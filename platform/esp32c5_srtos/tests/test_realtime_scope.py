"""Compile and test the actual portable ISR core; emit an interop EA fixture."""
from pathlib import Path
import subprocess

platform = Path(__file__).resolve().parents[1]
base = platform.parents[1]
out = platform / "build/host-realtime"
out.mkdir(parents=True, exist_ok=True)
subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Wcast-align",
                "-Wstrict-prototypes", "-Wmissing-prototypes", "-Wundef",
                "-I" + str(base / "code/dbg"), str(base / "code/dbg/realtime_scope.c"),
                str(base / "tests/host/realtime_scope/test_realtime_scope.c"), "-o", str(out / "test.exe")], check=True)
subprocess.run([str(out / "test.exe"), str(out / "ea.bin")], check=True)
