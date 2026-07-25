"""FRAME-backed dual-port acceptance helper for the Zynq-7020 PL UART DMA."""

from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path


FRAME_COUNT_DEFAULT = 11_000
PAYLOAD_SIZE = 497
FRAME_SIZE = PAYLOAD_SIZE + 15
STATUS_COMMAND = b"PL_UART_STATUS\r\n"
STATUS_PATTERN = re.compile(
    rb"pl_uart version=([0-9A-Fa-f]{8}) status=([0-9A-Fa-f]{8}) "
    rb"irq=([0-9A-Fa-f]{8})/(\d+)/([0-9A-Fa-f]{8}) "
    rb"rx=(\d+)/(\d+) tx=(\d+)/(\d+) "
    rb"uart_err=([0-9A-Fa-f]{8}) dma_err=([0-9A-Fa-f]{8}) "
    rb"stop=([0-9A-Fa-f]{8})"
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("role", choices=("monitor", "stress"))
    parser.add_argument("--frame-root", type=Path, required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=921_600)
    parser.add_argument("--frames", type=int, default=FRAME_COUNT_DEFAULT)
    parser.add_argument("--duration", type=float, default=300.0)
    parser.add_argument("--timeout", type=float, default=0.5)
    return parser.parse_args()


def load_frame(frame_root: Path) -> tuple[object, object, object]:
    sys.path.insert(0, str(frame_root))
    import serial  # type: ignore[import-not-found]
    from serial_debug_assistant.protocol import FrameParser, build_frame

    return serial, FrameParser, build_frame


def build_payload(sequence: int) -> bytes:
    body = bytearray(PAYLOAD_SIZE)
    body[0:4] = sequence.to_bytes(4, "little")
    for index in range(4, PAYLOAD_SIZE):
        body[index] = ((sequence * 17) + (index * 29) + (sequence >> 8)) & 0xFF
    return bytes(body)


def run_monitor(serial_module: object, port: str, baud: int, duration: float) -> int:
    deadline = time.monotonic() + duration
    next_query = 0.0
    receive_buffer = bytearray()
    status_count = 0

    with serial_module.Serial(port=port, baudrate=baud, timeout=0.05) as device:
        device.reset_input_buffer()
        while time.monotonic() < deadline:
            now = time.monotonic()
            if now >= next_query:
                device.write(STATUS_COMMAND)
                next_query = now + 2.0
            chunk = device.read(device.in_waiting or 1)
            if chunk:
                receive_buffer.extend(chunk)
                while b"\n" in receive_buffer:
                    line, _, receive_buffer = receive_buffer.partition(b"\n")
                    match = STATUS_PATTERN.search(line)
                    if match is not None:
                        status_count += 1
                        print(line.decode("ascii", errors="replace"), flush=True)
    print(f"FRAME_COM6_MONITOR result=PASS status_samples={status_count}", flush=True)
    return 0 if status_count > 0 else 1


def run_stress(
    serial_module: object,
    parser_type: object,
    build_frame: object,
    port: str,
    baud: int,
    frame_count: int,
    timeout: float,
) -> int:
    parser = parser_type()
    tx_bytes = 0
    rx_bytes = 0
    started = time.monotonic()

    with serial_module.Serial(port=port, baudrate=baud, timeout=0.01) as device:
        device.reset_input_buffer()
        for sequence in range(frame_count):
            payload = build_payload(sequence)
            request = build_frame(
                dst=2,
                d_dst=0,
                cmd_set=0x01,
                cmd_word=0x17,
                payload=payload,
            )
            if len(request) != FRAME_SIZE:
                raise RuntimeError(f"unexpected request size: {len(request)}")
            device.write(request)
            tx_bytes += len(request)

            response = None
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                chunk = device.read(device.in_waiting or 1)
                if not chunk:
                    continue
                for frame in parser.feed(chunk):
                    if (
                        frame.cmd_set == 0x01
                        and frame.cmd_word == 0x17
                        and frame.is_ack == 1
                    ):
                        response = frame
                        break
                if response is not None:
                    break

            if response is None:
                raise RuntimeError(f"ACK timeout at sequence {sequence}")
            if (
                response.src != 2
                or response.d_src != 0
                or response.dst != 1
                or response.d_dst != 0
            ):
                raise RuntimeError(f"address mismatch at sequence {sequence}")
            if response.payload != payload:
                raise RuntimeError(f"payload mismatch at sequence {sequence}")
            rx_bytes += len(response.payload) + 15

            if ((sequence + 1) % 1000) == 0:
                print(
                    "FRAME_COM7_PROGRESS "
                    f"frames={sequence + 1} tx_bytes={tx_bytes} rx_bytes={rx_bytes}",
                    flush=True,
                )

    elapsed = time.monotonic() - started
    total_bytes = tx_bytes + rx_bytes
    if frame_count < 1000 or total_bytes < (10 * 1024 * 1024):
        raise RuntimeError(
            f"acceptance volume too small: frames={frame_count} total={total_bytes}"
        )
    print(
        "FRAME_COM7_STRESS result=PASS "
        f"frames={frame_count} payload={PAYLOAD_SIZE} "
        f"tx_bytes={tx_bytes} rx_bytes={rx_bytes} total_bytes={total_bytes} "
        f"elapsed_s={elapsed:.3f}",
        flush=True,
    )
    return 0


def main() -> int:
    arguments = parse_arguments()
    serial_module, parser_type, build_frame = load_frame(arguments.frame_root)
    if arguments.role == "monitor":
        return run_monitor(
            serial_module,
            arguments.port,
            arguments.baud,
            arguments.duration,
        )
    return run_stress(
        serial_module,
        parser_type,
        build_frame,
        arguments.port,
        arguments.baud,
        arguments.frames,
        arguments.timeout,
    )


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"FRAME_PL_UART_DMA_ACCEPTANCE result=FAIL error={error}", file=sys.stderr)
        raise
