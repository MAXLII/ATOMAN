"""Exercise SECTION TCP registrations without a BSP, FRAME, or discovery service."""
from pathlib import Path
import ctypes
import socket
import subprocess
import sys
import time

sys.dont_write_bytecode = True
from test_plecs_route_bridge import LoadedNode

ROOT = Path(__file__).resolve().parents[4]
OUT = ROOT / "build" / "sim_tcp_registration"
OUT.mkdir(parents=True, exist_ok=True)

# Ask the OS for unused loopback ports; avoid the user's running simulations.
reservations = [socket.socket() for _ in range(3)]
for reservation in reservations:
    reservation.bind(("127.0.0.1", 0))
ports = [reservation.getsockname()[1] for reservation in reservations]
for reservation in reservations:
    reservation.close()

source = r'''
#include "sim_tcp.h"
REG_SIM_TCP(p_a, "a", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
REG_SIM_TCP(p_b, "b", SIM_TCP_SERVER, "127.0.0.1", PORT_B)
REG_SIM_TCP(p_c, "c", SIM_TCP_SERVER, "127.0.0.1", PORT_C)
REG_SIM_TCP(p_client, "client", SIM_TCP_CLIENT, "127.0.0.1", PORT_C)
REG_SIM_TCP(p_invalid, "invalid", SIM_TCP_SERVER, "bad-ip", PORT_A)
REG_SIM_TCP(p_busy_a, "busy-a", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
REG_SIM_TCP(p_busy_b, "busy-b", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
REG_SIM_TCP(p_busy_c, "busy-c", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
REG_SIM_TCP(p_busy_d, "busy-d", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
REG_SIM_TCP(p_overflow, "overflow", SIM_TCP_SERVER, "127.0.0.1", PORT_A)
void sim_comm_stop(void);
uint32_t test_query(uint32_t index);
void test_client_send(void);
uint32_t test_client_read(void);
void sim_comm_stop(void) { sim_tcp_stop(); }
uint32_t test_query(uint32_t index)
{
    if (index == 0u) return sim_tcp_get_status("client")->connected;
    if (index == 1u) return sim_tcp_get_status("invalid")->last_error;
    if (index == 2u) return (p_invalid == NULL) ? 1u : 0u;
    if (index == 4u) return sim_tcp_get_status("overflow")->last_error;
    if (index == 5u) return (p_overflow == NULL) ? 1u : 0u;
    if (index == 6u) return sim_tcp_get_status("busy-a")->last_error;
    return ((p_a == NULL) && (p_b == NULL) && (p_c == NULL) && (p_client == NULL)) ? 1u : 0u;
}
void test_client_send(void) { sim_tcp_tx(p_client, "C", 1); }
uint32_t test_client_read(void)
{
    uint8_t byte = 0u;
    return (sim_tcp_rx_get_byte(p_client, &byte) != 0u) ? byte : 0u;
}
static void echo_task(void)
{
    struct sim_tcp *channels[] = {p_a, p_b, p_c};
    for (uint32_t i = 0u; i < 3u; ++i)
    {
        uint8_t byte = 0u;
        if (sim_tcp_rx_get_byte(channels[i], &byte) != 0u)
            sim_tcp_tx(channels[i], (const char *)&byte, 1);
    }
}
REG_TASK(1, echo_task)
'''
fixture = OUT / "fixture.c"
fixture.write_text(source, encoding="utf-8")
command = ["C:/mingw64/bin/gcc.exe", "-std=c11", "-shared", "-Wl,--export-all-symbols", "-DPLATFORM_PLECS",
           "-DTOOLCHAIN_GCC", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
           "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Wcast-align",
           "-Wmissing-prototypes", "-Wstrict-prototypes", "-Wundef"]
for name, port in zip(("A", "B", "C"), ports):
    command.append(f"-DPORT_{name}={port}")
for folder in ("code/sim/comm", "code/sim/debug/perf", "code/section/baremetal",
               "code/section", "code/lib", "platform/plecs/common",
               "platform/plecs/frame_route_bridge"):
    command += ["-I", str(ROOT / folder)]
command += [str(fixture), str(ROOT / "code/sim/comm/sim_tcp.c"),
            str(ROOT / "code/section/baremetal/section.c"),
            str(ROOT / "platform/plecs/common/plecs.c"), "-lws2_32",
            "-o", str(OUT / "fixture.dll")]
subprocess.run(command, check=True)
node = LoadedNode(OUT / "fixture.dll")
dll = ctypes.CDLL(str(OUT / "fixture.dll"))
dll.test_query.argtypes = [ctypes.c_uint32]
dll.test_query.restype = ctypes.c_uint32
dll.test_client_read.restype = ctypes.c_uint32

def wait_value(function, expected):
    deadline = time.monotonic() + 3
    while True:
        with node._lock:
            actual = function()
        if actual == expected:
            return
        if time.monotonic() > deadline:
            raise AssertionError(f"timeout waiting for {expected}")
        time.sleep(.01)

for run in range(2):
    node.start()
    try:
        assert dll.test_query(1) != 0 and dll.test_query(2) == 1
        assert dll.test_query(4) != 0 and dll.test_query(5) == 1
        assert dll.test_query(6) != 0
        for port in ports[:2]:
            with socket.create_connection(("127.0.0.1", port), timeout=3) as peer:
                peer.sendall(b"registration")
                received = b""
                while len(received) < len(b"registration"):
                    chunk = peer.recv(32)
                    assert chunk
                    received += chunk
                assert received == b"registration"
        wait_value(lambda: dll.test_query(0), 1)
        with node._lock:
            dll.test_client_send()
        wait_value(dll.test_client_read, ord("C"))
    finally:
        node.terminate()
        dll.sim_comm_stop()
    assert dll.test_query(3) == 1
    for port in ports:
        with socket.socket() as peer:
            assert peer.connect_ex(("127.0.0.1", port)) != 0
print("PASS three servers, one client, invalid registration, busy ports, capacity limit, repeat init and release")
