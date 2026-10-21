"""Compile real Wi-Fi/USB BSP sources against a fault-injecting SDK shim."""
from pathlib import Path
import argparse
import json
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HEADERS = ["esp_event.h", "esp_netif.h", "esp_timer.h", "esp_wifi.h",
           "driver/usb_serial_jtag.h", "driver/usb_serial_jtag_vfs.h", "freertos/FreeRTOS.h"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    out = ROOT / "build/host-drivers"
    out.mkdir(parents=True, exist_ok=True)
    for name in HEADERS:
        path = out / "sdk" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "fake_sdk.h"\n', encoding="utf-8")
    command = [args.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-Wconversion",
               "-Wsign-conversion", "-Wshadow", "-Wcast-align", "-Wstrict-prototypes",
               "-Wmissing-prototypes", "-Wundef", "-I" + str(ROOT / "bsp"),
               "-I" + str(ROOT / "tests"), "-I" + str(out / "sdk"),
               str(ROOT / "bsp/bsp_wifi.c"), str(ROOT / "bsp/bsp_usart.c"),
               str(ROOT / "tests/test_drivers.c"), "-o", str(out / "test_drivers.exe")]
    subprocess.run(command, check=True)
    run = subprocess.run([str(out / "test_drivers.exe")], check=True, capture_output=True, text=True)
    print(run.stdout, end="")
    base = ROOT.parents[1]
    directory_command = [args.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
        "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Wcast-align", "-Wstrict-prototypes",
        "-Wmissing-prototypes", "-Wundef", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "-DPLATFORM_TESTBENCH", "-DTOOLCHAIN_GCC", "-I" + str(base / "code/section/baremetal"),
        "-I" + str(base / "code/section"), "-I" + str(base / "code/dbg"),
        "-I" + str(base / "code/data_source/comm"), "-I" + str(base / "code/lib"),
        str(ROOT / "tests/test_shell_directory.c"), "-o", str(out / "test_shell_directory.exe")]
    subprocess.run(directory_command, check=True)
    directory_run = subprocess.run([str(out / "test_shell_directory.exe")], check=True, capture_output=True, text=True)
    print(directory_run.stdout, end="")
    (out / "result.json").write_text(json.dumps({"passed": True, "hardware_tested": False,
        "command": command, "output": run.stdout, "directory_command": directory_command,
        "directory_output": directory_run.stdout}, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
