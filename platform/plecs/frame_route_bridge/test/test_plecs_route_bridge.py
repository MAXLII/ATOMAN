"""End-to-end TCP smoke test for the two PLECS route-bridge DLLs."""

from __future__ import annotations

import argparse
import ctypes
import socket
import struct
import sys
import time
from dataclasses import dataclass
from pathlib import Path


FRAME_HOST = "127.0.0.1"
FRAME_PORT = 5000
PC_ADDR = 0x01
NODE02_ADDR = 0x02
NODE03_ADDR = 0x03
LOOPBACK_CMD_SET = 0x30
LOOPBACK_CMD_WORD = 0x01
FRAME_SOP = 0xE8
FRAME_VERSION = 0x01
FRAME_EOP = b"\x0d\x0a"


class SimulationSizes(ctypes.Structure):
    """PLECS DLL size contract from DllHeader.h."""

    _pack_ = 4
    _fields_ = [
        ("numInputs", ctypes.c_int),
        ("numOutputs", ctypes.c_int),
        ("numStates", ctypes.c_int),
        ("numParameters", ctypes.c_int),
    ]


class SimulationState(ctypes.Structure):
    """PLECS DLL runtime contract from DllHeader.h."""

    _pack_ = 4
    _fields_ = [
        ("inputs", ctypes.POINTER(ctypes.c_double)),
        ("outputs", ctypes.POINTER(ctypes.c_double)),
        ("states", ctypes.POINTER(ctypes.c_double)),
        ("parameters", ctypes.POINTER(ctypes.c_double)),
        ("time", ctypes.c_double),
        ("errorMessage", ctypes.c_char_p),
        ("userData", ctypes.c_void_p),
    ]


@dataclass
class DecodedFrame:
    """Validated FRAME protocol fields used by the smoke assertions."""

    src: int
    d_src: int
    dst: int
    d_dst: int
    cmd_set: int
    cmd_word: int
    is_ack: int
    payload: bytes


class LoadedNode:
    """Loaded PLECS DLL and the state storage kept alive for its lifecycle."""

    def __init__(self, dll_path: Path) -> None:
        self.dll_path = dll_path
        self.dll = ctypes.CDLL(str(dll_path))
        self.dll.plecsSetSizes.argtypes = [ctypes.POINTER(SimulationSizes)]
        self.dll.plecsSetSizes.restype = None
        self.dll.plecsStart.argtypes = [ctypes.POINTER(SimulationState)]
        self.dll.plecsStart.restype = None
        self.dll.plecsOutput.argtypes = [ctypes.POINTER(SimulationState)]
        self.dll.plecsOutput.restype = None
        self.dll.plecsTerminate.argtypes = [ctypes.POINTER(SimulationState)]
        self.dll.plecsTerminate.restype = None

        sizes = SimulationSizes()
        self.dll.plecsSetSizes(ctypes.byref(sizes))
        actual_sizes = (sizes.numInputs, sizes.numOutputs, sizes.numStates, sizes.numParameters)
        if actual_sizes != (1, 2, 0, 0):
            raise AssertionError(f"unexpected DLL sizes for {dll_path.name}: {actual_sizes}")

        self.inputs = (ctypes.c_double * max(1, sizes.numInputs))()
        self.outputs = (ctypes.c_double * max(1, sizes.numOutputs))()
        self.states = (ctypes.c_double * max(1, sizes.numStates))()
        self.parameters = (ctypes.c_double * max(1, sizes.numParameters))()
        self.state = SimulationState(
            inputs=self.inputs,
            outputs=self.outputs,
            states=self.states,
            parameters=self.parameters,
            time=0.0,
            errorMessage=None,
            userData=None,
        )
        self.started = False

    def start(self) -> None:
        """Start or restart the simulated node."""

        if self.started:
            return
        self.dll.plecsStart(ctypes.byref(self.state))
        self.started = True

    def terminate(self) -> None:
        """Stop the simulated node if its lifecycle is active."""

        if not self.started:
            return
        self.dll.plecsTerminate(ctypes.byref(self.state))
        self.started = False


class FrameStream:
    """TCP stream decoder that preserves bytes between complete protocol frames."""

    def __init__(self, tcp_socket: socket.socket) -> None:
        self.tcp_socket = tcp_socket
        self.buffer = bytearray()

    def receive(self, timeout: float = 2.0) -> DecodedFrame:
        """Receive one validated frame before timeout."""

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            decoded = decode_one(self.buffer)
            if decoded is not None:
                return decoded
            self.tcp_socket.settimeout(max(0.01, deadline - time.monotonic()))
            try:
                chunk = self.tcp_socket.recv(65536)
            except socket.timeout:
                continue
            if not chunk:
                raise ConnectionError("FRAME TCP connection closed while waiting for a response")
            self.buffer.extend(chunk)
        raise TimeoutError("timed out waiting for a FRAME protocol response")


def crc16_ccitt(data: bytes) -> int:
    """Calculate the repository's CRC-16-CCITT value."""

    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def encode_frame(
    dst: int,
    payload: bytes,
    *,
    cmd_set: int = LOOPBACK_CMD_SET,
    cmd_word: int = LOOPBACK_CMD_WORD,
    corrupt_crc: bool = False,
) -> bytes:
    """Encode one PC-originated protocol request."""

    header = bytes(
        [
            FRAME_SOP,
            FRAME_VERSION,
            PC_ADDR,
            0x00,
            dst,
            0x00,
            cmd_set,
            cmd_word,
            0x00,
        ]
    ) + struct.pack("<H", len(payload))
    crc = crc16_ccitt(header + payload)
    if corrupt_crc:
        crc ^= 0x0001
    return header + payload + struct.pack("<H", crc) + FRAME_EOP


def decode_one(buffer: bytearray) -> DecodedFrame | None:
    """Remove and decode one complete valid frame from a TCP byte buffer."""

    while buffer and buffer[0] != FRAME_SOP:
        del buffer[0]
    if len(buffer) < 15:
        return None

    payload_length = struct.unpack_from("<H", buffer, 9)[0]
    frame_length = 15 + payload_length
    if len(buffer) < frame_length:
        return None

    raw = bytes(buffer[:frame_length])
    del buffer[:frame_length]
    if raw[-2:] != FRAME_EOP:
        raise AssertionError(f"invalid EOP in response: {raw[-2:].hex()}")
    expected_crc = struct.unpack_from("<H", raw, 11 + payload_length)[0]
    actual_crc = crc16_ccitt(raw[: 11 + payload_length])
    if expected_crc != actual_crc:
        raise AssertionError(f"invalid response CRC: expected {expected_crc:04x}, got {actual_crc:04x}")

    return DecodedFrame(
        src=raw[2],
        d_src=raw[3],
        dst=raw[4],
        d_dst=raw[5],
        cmd_set=raw[6],
        cmd_word=raw[7],
        is_ack=raw[8],
        payload=raw[11 : 11 + payload_length],
    )


def assert_ack(frame: DecodedFrame, expected_src: int, expected_payload: bytes) -> None:
    """Validate direct ACK identity, command, address, and echoed payload."""

    expected = (
        expected_src,
        0x00,
        PC_ADDR,
        0x00,
        LOOPBACK_CMD_SET,
        LOOPBACK_CMD_WORD,
        0x01,
        expected_payload,
    )
    actual = (
        frame.src,
        frame.d_src,
        frame.dst,
        frame.d_dst,
        frame.cmd_set,
        frame.cmd_word,
        frame.is_ack,
        frame.payload,
    )
    if actual != expected:
        raise AssertionError(f"unexpected ACK: actual={actual!r}, expected={expected!r}")


def connect_frame(timeout: float = 5.0) -> socket.socket:
    """Connect to node 0x02 after its worker starts listening."""

    deadline = time.monotonic() + timeout
    last_error: OSError | None = None
    while time.monotonic() < deadline:
        try:
            tcp_socket = socket.create_connection((FRAME_HOST, FRAME_PORT), timeout=0.5)
            tcp_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            return tcp_socket
        except OSError as exc:
            last_error = exc
            time.sleep(0.05)
    raise ConnectionError(f"could not connect to {FRAME_HOST}:{FRAME_PORT}: {last_error}")


def wait_for_routed_ack(tcp_socket: socket.socket, stream: FrameStream, payload: bytes, timeout: float = 5.0) -> None:
    """Retry routed requests until the internal link is established."""

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        tcp_socket.sendall(encode_frame(NODE03_ADDR, payload))
        try:
            frame = stream.receive(timeout=0.3)
        except TimeoutError:
            continue
        if frame.payload == payload:
            assert_ack(frame, NODE03_ADDR, payload)
            return
    raise TimeoutError("node 0x02 did not restore the routed connection to node 0x03")


def run_smoke_test(dll_dir: Path) -> None:
    """Execute all required direct, routed, parser, and reconnect scenarios."""

    node02_path = dll_dir / "plecs_node02" / "plecs_node02.dll"
    node03_path = dll_dir / "plecs_node03" / "plecs_node03.dll"
    for dll_path in (node02_path, node03_path):
        if not dll_path.is_file():
            raise FileNotFoundError(f"missing built DLL: {dll_path}")

    node02 = LoadedNode(node02_path)
    node03 = LoadedNode(node03_path)
    tcp_socket: socket.socket | None = None
    try:
        node03.start()
        node02.start()
        tcp_socket = connect_frame()
        stream = FrameStream(tcp_socket)

        bad_frame = encode_frame(NODE02_ADDR, b"bad-crc", corrupt_crc=True)
        fragmented_frame = encode_frame(NODE02_ADDR, b"direct-fragmented")
        tcp_socket.sendall(bad_frame + fragmented_frame[:4])
        time.sleep(0.02)
        tcp_socket.sendall(fragmented_frame[4:11])
        time.sleep(0.02)
        tcp_socket.sendall(fragmented_frame[11:])
        assert_ack(stream.receive(), NODE02_ADDR, b"direct-fragmented")
        print("PASS direct routing, invalid CRC rejection, and fragmented input")

        direct_payload = b"direct-batch"
        routed_payload = b"routed-batch"
        tcp_socket.sendall(
            encode_frame(NODE02_ADDR, direct_payload) + encode_frame(NODE03_ADDR, routed_payload)
        )
        replies = [stream.receive(), stream.receive()]
        replies_by_payload = {reply.payload: reply for reply in replies}
        assert_ack(replies_by_payload[direct_payload], NODE02_ADDR, direct_payload)
        assert_ack(replies_by_payload[routed_payload], NODE03_ADDR, routed_payload)
        print("PASS consecutive direct and routed frames")

        for target_addr, expected_count in ((NODE02_ADDR, 8), (NODE03_ADDR, 7)):
            tcp_socket.sendall(encode_frame(target_addr, b"", cmd_set=0x01, cmd_word=0x01))
            count_ack = stream.receive()
            list_batch = stream.receive()
            if (
                count_ack.src != target_addr
                or count_ack.dst != PC_ADDR
                or count_ack.cmd_set != 0x01
                or count_ack.cmd_word != 0x01
                or count_ack.is_ack != 0x01
                or struct.unpack("<I", count_ack.payload)[0] != expected_count
            ):
                raise AssertionError(f"invalid parameter-count ACK from node 0x{target_addr:02x}: {count_ack}")
            if (
                list_batch.src != target_addr
                or list_batch.dst != PC_ADDR
                or list_batch.cmd_set != 0x01
                or list_batch.cmd_word != 0x3F
                or list_batch.is_ack != 0x00
                or b"NODE_ADDR" not in list_batch.payload
                or b"NODE_VALUE" not in list_batch.payload
            ):
                raise AssertionError(f"invalid parameter-list batch from node 0x{target_addr:02x}: {list_batch}")
        print("PASS direct and routed parameter-list services")

        parameter_name = b"NODE_VALUE"
        for target_addr, value, node in (
            (NODE02_ADDR, 202, node02),
            (NODE03_ADDR, 303, node03),
        ):
            write_payload = bytes([len(parameter_name)]) + struct.pack("<III", value, 1_000_000, 0) + parameter_name
            tcp_socket.sendall(
                encode_frame(target_addr, write_payload, cmd_set=0x01, cmd_word=0x03)
            )
            write_ack = stream.receive()
            expected_ack_payload = (
                bytes([len(parameter_name), 0x05])
                + struct.pack("<III", value, 1_000_000, 0)
                + parameter_name
            )
            if (
                write_ack.src != target_addr
                or write_ack.dst != PC_ADDR
                or write_ack.cmd_set != 0x01
                or write_ack.cmd_word != 0x03
                or write_ack.is_ack != 0x01
                or write_ack.payload != expected_ack_payload
            ):
                raise AssertionError(f"invalid NODE_VALUE write ACK from node 0x{target_addr:02x}: {write_ack}")
            node.state.time += 0.0001
            node.dll.plecsOutput(ctypes.byref(node.state))
            if node.outputs[0] != float(value):
                raise AssertionError(
                    f"node 0x{target_addr:02x} PLECS output did not follow NODE_VALUE: {node.outputs[0]}"
                )
        print("PASS direct and routed NODE_VALUE writes with PLECS output updates")

        node03.terminate()
        time.sleep(0.3)
        tcp_socket.sendall(encode_frame(NODE03_ADDR, b"offline"))
        try:
            offline_reply = stream.receive(timeout=0.3)
        except TimeoutError:
            offline_reply = None
        if offline_reply is not None:
            raise AssertionError(f"received routed response while node 0x03 was stopped: {offline_reply}")

        node03.start()
        wait_for_routed_ack(tcp_socket, stream, b"after-reconnect")
        print("PASS node 0x03 restart and node 0x02 automatic reconnect")
    finally:
        if tcp_socket is not None:
            tcp_socket.close()
        node02.terminate()
        node03.terminate()


def parse_args() -> argparse.Namespace:
    """Parse an optional build output directory."""

    repository_root = Path(__file__).resolve().parents[4]
    default_dll_dir = repository_root / "platform" / "plecs" / "frame_route_bridge" / "build" / "bin"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll-dir", type=Path, default=default_dll_dir)
    return parser.parse_args()


def main() -> int:
    """Run the smoke test and return a process status."""

    args = parse_args()
    try:
        run_smoke_test(args.dll_dir.resolve())
    except Exception as exc:  # noqa: BLE001 - command-line test must report every failure.
        print(f"FAIL {type(exc).__name__}: {exc}", file=sys.stderr)
        return 1
    print("PASS all PLECS route-bridge TCP smoke tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
