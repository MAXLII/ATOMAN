"""Run the workspace CHB switching model through PLECS RPC and FRAME.

Entity: the saved model and freshly built DLLs are the test subjects.
Prior: successful RPC completion alone does not prove closed-loop behavior.
Time: simulation, connections and requests have explicit deadlines.

Requires PLECS Standalone RPC at localhost:1080 and compile.bat run first.
Results and the sampled PWM trace are saved under build/simulation-test/.
The model, controller settings and solver tolerances are left unchanged.
"""

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import socket
import struct
import time
from concurrent.futures import ThreadPoolExecutor
import xmlrpc.client


PROJECT = Path(__file__).resolve().parents[1]
MODEL = PROJECT / "chb.plecs"
BUILD = PROJECT / "build"
LOG = BUILD / "bin/plecs_log.txt"
RESULTS = BUILD / "simulation-test"
RPC = "http://127.0.0.1:1080/RPC2"
STOP_TIME = 4.7
WALL_TIMEOUT = 600
CELL_COUNT = 5
STAMP = re.compile(r"\[\s*([\d.]+)\]")
PWM = re.compile(
    r"\[\s*([\d.]+)\] PWM vg=(\S+) il=(\S+) ib=(\S+) th=(\S+) "
    r"bus=(\S+) vcmd=(\S+) duty=(\S+) vpwm=(\S+)"
)


class BoundedTransport(xmlrpc.client.Transport):
    def make_connection(self, host):
        connection = super().make_connection(host)
        connection.timeout = WALL_TIMEOUT + 10
        return connection


def crc16(data):
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc & 0x8000 else 0)) & 0xFFFF
    return crc


class FrameClient:
    def __init__(self, connection, address=2):
        self.connection = connection
        self.buffer = b""
        self.address = address

    def request(self, word, payload):
        header = struct.pack("<9BH", 0xE8, 1, 1, 0, self.address, 0, 1, word, 0, len(payload))
        data = header + payload
        self.connection.sendall(data + struct.pack("<H", crc16(data)) + b"\r\n")
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            if len(self.buffer) >= 11:
                size = 15 + struct.unpack_from("<H", self.buffer, 9)[0]
                if size > 4096:
                    raise AssertionError("FRAME response exceeds the test buffer budget")
                if len(self.buffer) >= size:
                    packet, self.buffer = self.buffer[:size], self.buffer[size:]
                    assert packet[:2] == b"\xe8\x01" and packet[7:9] == bytes((word, 1))
                    assert packet[-2:] == b"\r\n"
                    assert crc16(packet[:-4]) == struct.unpack_from("<H", packet, size - 4)[0]
                    return packet[11:-4]
            try:
                chunk = self.connection.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                raise ConnectionError("FRAME disconnected before its ACK")
            self.buffer += chunk
        raise TimeoutError(f"FRAME command {word} did not acknowledge")

    def read(self, name):
        key = name.encode("ascii")
        result = self.request(2, bytes((len(key),)) + key)
        assert result[0] == len(key) and result[-len(key):] == key
        return struct.unpack_from("<f" if result[1] == 6 else "<I", result, 2)[0]

    def write(self, name, value, floating=False):
        key = name.encode("ascii")
        values = struct.pack("<fff", value, 1e6, 1.0) if floating else struct.pack("<III", value, 1, 0)
        result = self.request(3, bytes((len(key),)) + values + key)
        assert result[0] == len(key) and result[-len(key):] == key
        if name != "LOAD_APPLY":
            actual = struct.unpack_from("<f", result, 2)[0] if floating else result[2]
            assert actual == value, f"{name}: requested {value}, acknowledged {actual}"

    def load(self, resistances):
        for cell, resistance in enumerate(resistances, 1):
            self.write(f"LOAD_R{cell}", resistance, floating=True)
        self.write("LOAD_APPLY", 0)


def check_trace(text):
    rows = []
    for match in PWM.finditer(text):
        stamp, grid, current, beta, theta, buses, command, duties, voltages = match.groups()
        bus = list(map(float, buses.split(",")))
        duty = list(map(float, duties.split(",")))
        voltage = list(map(float, voltages.split(",")))
        values = list(map(float, (stamp, grid, current, beta, theta, command))) + bus + duty + voltage
        assert all(math.isfinite(value) for value in values), "non-finite PWM telemetry"
        assert len(bus) == len(duty) == len(voltage) == CELL_COUNT
        assert all(2700 <= value <= 3600 for value in bus), "RUN bus outside the test voltage envelope"
        assert all(0 <= value <= 1 for value in duty), "invalid duty"
        assert all(abs(v) <= 0.98 * b + 0.1 for v, b in zip(voltage, bus)), "bridge modulation limit"
        rows.append((float(stamp), float(current), bus, float(grid)))
    assert rows and rows[-1][0] >= 4.49, "missing PWM trace before the requested stop"
    assert "CHB_FSM_STATE_FAULT" not in text, "FSM entered FAULT"
    states = ["CHB_FSM_STATE_SOFT_START", "CHB_FSM_STATE_MAIN_WAIT", "CHB_FSM_STATE_RUN"]
    assert all(state in text for state in states), "missing startup state"
    loads = []
    for line in text.splitlines():
        if "LOAD r=" in line:
            loads.append((float(STAMP.search(line)[1]), list(map(float, line.split("r=")[1].split()[0].split(",")))))
    expected = [[512.0] * CELL_COUNT, [482.0, 512.0, 542.0, 512.0, 512.0], [542.0] * CELL_COUNT]
    assert [values for _, values in loads] == expected, "load application does not match commands"
    summaries = []
    for index, (applied, resistances) in enumerate(loads):
        end = loads[index + 1][0] if index + 1 < len(loads) else 4.5
        steady = [row for row in rows if end - 0.12 <= row[0] < end - 0.02]
        assert len(steady) >= 150 and steady[0][0] > applied + 0.5, "insufficient settled samples"
        means = [sum(row[2][cell] for row in steady) / len(steady) for cell in range(CELL_COUNT)]
        powers = [sum(row[2][cell] ** 2 / resistances[cell] for row in steady) / len(steady)
                  for cell in range(CELL_COUNT)]
        targets = [min(3200.0, math.sqrt(20000.0 * resistance)) for resistance in resistances]
        assert all(abs(mean - target) < 32 for mean, target in zip(means, targets)), ("bus regulation", means, targets)
        assert all(power <= 20200 for power in powers), ("steady load power exceeds 1% margin", powers)
        summaries.append({"applied_at": applied, "resistances": resistances, "samples": len(steady),
                          "bus_mean": means, "load_power_mean": powers})
    peak = max(abs(row[1]) for row in rows)
    assert peak < 50, ("sampled input current margin", peak)
    final_grid = [row[3] for row in rows if 4.3 <= row[0] < 4.4]
    grid_rms = math.sqrt(sum(value * value for value in final_grid) / len(final_grid))
    assert abs(grid_rms - 10000) < 10, ("model grid RMS", grid_rms)
    return {"pwm_samples": len(rows), "sampled_current_peak": peak, "grid_rms": grid_rms, "stages": summaries}


class QualificationScenario:
    """Attack the grid/Shell AND gate using actual source and app measurements."""

    TIMES = (0.01, 0.10, 0.35, 0.40, 0.65, 0.70, 0.75, 0.80, 0.85,
             1.25, 1.95, 2.00, 2.30, 2.35, 2.80)

    def __init__(self):
        self.index = 0
        self.events = []

    @staticmethod
    def idle(client):
        assert client.read("RUN_REQUEST") == 0, "invalid/stale Shell request survived"
        assert client.read("RUN_STATE") == 1, "grid/Shell AND gate allowed an unexpected start"
        assert all(client.read(f"DUTY_{cell}") == 0 for cell in range(1, CELL_COUNT + 1))

    def advance(self, stamp, client, grid):
        if self.index == len(self.TIMES) or stamp < self.TIMES[self.index]:
            return
        assert stamp < self.TIMES[self.index] + 0.04, "missed qualification stimulus timing"
        stage = self.index
        if stage in (0, 7):
            client.write("RUN_REQUEST", 1)
            label = "request before 200 ms qualification"
        elif stage in (1, 2, 5, 8, 14):
            self.idle(client)
            label = "IDLE, request cleared, PWM disabled"
            if stage in (2, 14):
                rms, hz = client.read("GRID_RMS_V"), client.read("GRID_HZ")
                assert abs(rms - 10000) < 20 and abs(hz - 50) < 0.05, (rms, hz)
            if stage == 14:
                assert abs(client.read("I_L")) < 0.01, "physical current remained after grid stop"
        elif stage == 3:
            grid.write("GRID_SOURCE_RMS_V", 8000.0, floating=True)
            label = "source voltage lowered to 8 kV"
        elif stage == 4:
            rms = client.read("GRID_RMS_V")
            assert abs(rms - 8000) < 30, ("app RMS did not follow the source", rms)
            client.write("RUN_REQUEST", 1)
            label = f"measured {rms:.2f} V, invalid voltage start requested"
        elif stage == 6:
            grid.write("GRID_SOURCE_RMS_V", 10000.0, floating=True)
            label = "source voltage restored; old request must stay cleared"
        elif stage == 9:
            self.idle(client)
            client.write("RUN_REQUEST", 1)
            label = "qualified grid explicitly started"
        elif stage == 10:
            assert client.read("RUN_STATE") == 4 and client.read("RUN_REQUEST") == 1
            label = "RUN reached after explicit request"
        elif stage == 11:
            grid.write("GRID_SOURCE_HZ", 52.0, floating=True)
            label = "running source frequency raised to 52 Hz"
        elif stage == 12:
            hz = client.read("GRID_HZ")
            assert abs(hz - 52) < 0.05, ("PLL frequency did not follow the source", hz)
            self.idle(client)
            label = f"measured {hz:.4f} Hz, RUN withdrawn and Shell request cleared"
        else:
            grid.write("GRID_SOURCE_HZ", 50.0, floating=True)
            label = "frequency restored; no automatic restart allowed"
        self.events.append({"time": stamp, "stage": stage, "result": label})
        print(f"{stamp:.4f}s: {label}", flush=True)
        self.index += 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qualification", action="store_true", help="test dynamic RMS/PLL and the Shell startup gate")
    args = parser.parse_args()
    qualification = QualificationScenario() if args.qualification else None
    stop_time = 2.9 if qualification else STOP_TIME
    RESULTS.mkdir(parents=True, exist_ok=True)
    proxy = xmlrpc.client.ServerProxy(RPC, transport=BoundedTransport())
    proxy.plecs.load(MODEL.as_posix())
    assert "plecs_chb.dll" in proxy.plecs.get("chb/DLL", "Filename")
    assert "plecs_grid_source.dll" in proxy.plecs.get("chb/DLL1", "Filename")
    assert proxy.plecs.get("chb/S1", "Type") == proxy.plecs.get("chb/S2", "Type") == "Breaker"
    metadata = {"model": str(MODEL), "stop_time": stop_time,
                "scenario": "qualification" if qualification else "closed_loop",
                "sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in (MODEL, BUILD / "bin/plecs_chb.dll", BUILD / "bin/plecs_grid_source.dll")}}
    if LOG.exists():
        shutil.copyfile(LOG, RESULTS / "previous-plecs-log.txt")
    phases = [(1.5, "balanced", [512.0] * CELL_COUNT),
              (2.5, "unbalanced", [482.0, 512.0, 542.0, 512.0, 512.0]),
              (3.5, "recovered", [542.0] * CELL_COUNT), (4.5, "stop", None)]
    connection = None
    grid_connection = None
    client = None
    with ThreadPoolExecutor(max_workers=1) as pool:
        future = pool.submit(proxy.plecs.simulate, "chb", {"SolverOpts": {"TimeSpan": stop_time, "Timeout": WALL_TIMEOUT}})
        try:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline and not future.done():
                try:
                    connection = socket.create_connection(("127.0.0.1", 5004), timeout=0.2)
                    break
                except OSError:
                    time.sleep(0.1)
            if connection is None:
                if future.done():
                    future.result()
                raise TimeoutError("CHB FRAME endpoint did not open")
            connection.settimeout(0.2)
            client = FrameClient(connection)
            if qualification:
                grid_connection = socket.create_connection(("127.0.0.1", 5005), timeout=0.2)
                grid_connection.settimeout(0.2)
                grid = FrameClient(grid_connection, address=3)
            start_requested = False
            phase = 0
            stopped = False
            current_zero = False
            wall_deadline = time.monotonic() + WALL_TIMEOUT + 10
            next_status = 0.0
            while not future.done():
                assert time.monotonic() < wall_deadline, "simulation wall deadline exceeded"
                stamp = client.read("SIM_TICK_100US") * 0.0001
                if qualification:
                    qualification.advance(stamp, client, grid)
                    if qualification.index == len(qualification.TIMES):
                        break  # All FRAME assertions consumed; await solver completion without another request.
                    if time.monotonic() >= next_status:
                        print(f"PLECS qualification: {stamp:.4f}s / {stop_time}s", flush=True)
                        next_status = time.monotonic() + 15
                    time.sleep(0.1)
                    continue
                if not start_requested and stamp >= 0.50:
                    assert client.read("RUN_STATE") == 1, "qualified grid alone started the FSM"
                    assert client.read("RUN_REQUEST") == 0
                    rms, hz = client.read("GRID_RMS_V"), client.read("GRID_HZ")
                    assert abs(rms - 10000) < 20 and abs(hz - 50) < 0.05, ("app grid measurement", rms, hz)
                    metadata["app_grid_before_start"] = {"time": stamp, "rms": rms, "hz": hz}
                    client.write("RUN_REQUEST", 1)
                    start_requested = True
                    print(f"{stamp:.4f}s: qualified grid, explicit Shell start requested", flush=True)
                if phase < len(phases) and stamp >= phases[phase][0]:
                    _, name, resistances = phases[phase]
                    assert client.read("RUN_STATE") == 4, f"not in RUN before {name}"
                    if resistances is None:
                        client.write("RUN_REQUEST", 0)
                    else:
                        client.load(resistances)
                    print(f"{stamp:.4f}s: {name} requested", flush=True)
                    phase += 1
                if phase == len(phases) and not stopped:
                    stopped = client.read("RUN_STATE") == 1
                    if stopped:
                        assert client.read("RUN_REQUEST") == 0
                        assert all(client.read(f"DUTY_{cell}") == 0 for cell in range(1, CELL_COUNT + 1))
                        print("PASS: normal stop reached IDLE with all PWM commands disabled", flush=True)
                if stopped and not current_zero:
                    current = client.read("I_L")
                    current_zero = abs(current) < 0.01
                    if current_zero:
                        metadata["stopped_input_current"] = current
                        print("PASS: physical input current decayed to zero after stop", flush=True)
                if phase == len(phases) and stopped and current_zero:
                    break  # The DLL closes FRAME at simulation end, before RPC may return.
                if time.monotonic() >= next_status:
                    print(f"PLECS running: {stamp:.4f}s / {STOP_TIME}s", flush=True)
                    next_status = time.monotonic() + 15
                time.sleep(0.2)
            future.result(timeout=max(0.001, wall_deadline - time.monotonic()))
            text = LOG.read_text(encoding="utf-8", errors="replace")
            if qualification:
                assert qualification.index == len(qualification.TIMES), "qualification scenario incomplete"
                assert "CHB_FSM_STATE_FAULT" not in text, "qualification run entered FAULT"
                metadata.update(events=qualification.events, status="PASS")
            else:
                assert phase == len(phases) and stopped and current_zero, "simulation ended before the complete scenario"
                metadata.update(check_trace(text), status="PASS")
        except Exception as error:
            metadata.update(status="FAIL", error=str(error))
            print(f"FAIL: {error}", flush=True)
            if client is not None and not future.done():
                try:
                    client.write("RUN_REQUEST", 0)
                except (OSError, TimeoutError, AssertionError):
                    pass  # Preserve the original failure; the solver still has its wall deadline.
            raise
        finally:
            if connection is not None:
                connection.close()
            if grid_connection is not None:
                grid_connection.close()
            if qualification:
                metadata["events"] = qualification.events
            prefix = "qualification-" if qualification else ""
            if LOG.exists():
                shutil.copyfile(LOG, RESULTS / f"{prefix}plecs-log.txt")
            (RESULTS / f"{prefix}result.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(json.dumps(metadata, indent=2), flush=True)


if __name__ == "__main__":
    main()
