"""Check actual ESP32 build inputs and ELF scheduler ownership after a full build."""
import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--nm', default='riscv32-esp-elf-nm')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    base = project.parents[1]
    assert project.name in ('esp32c5', 'esp32c5_srtos')
    srtos = project.name.endswith('_srtos')
    other = 'esp32c5' if srtos else 'esp32c5_srtos'
    forbidden = [(base / 'platform' / other).as_posix().lower() + '/',
                 (base / 'code/interface/demo' / other).as_posix().lower() + '/',
                 (base / 'code/business/demo' / other).as_posix().lower() + '/']
    interface = base / 'code/interface/demo' / project.name
    application = base / 'code/business/demo' / project.name
    for folder in (interface, application):
        for source in folder.iterdir():
            if source.suffix not in ('.c', '.h'):
                continue
            content = source.read_text(encoding='utf-8')
            includes = re.findall(r'#include[ \t]+[<"]([^>"]+)', content)
            assert not any(name.startswith(('esp_', 'freertos/')) for name in includes), source
            if folder == application:
                assert not any(name.startswith(('bsp_', 'wifi_config.h')) for name in includes), source
            else:
                assert not re.search(r'REG_SHELL_(?:CMD|VAR)\s*\(|DECLARE_COMM(?:_V1)?_CTX\s*\(', content), source
                assert not re.search(r'static\s+uint8_t\s+\w+\s*\[', content), source
    commands = json.loads((project / 'build/compile_commands.json').read_text(encoding='utf-8'))
    sources = []
    for row in commands:
        text = (row['file'] + '\n' + row['command']).replace('\\', '/').lower()
        assert not any(path in text for path in forbidden), row['file']
        sources.append(Path(row['file']).as_posix().lower())
    core = 'srtos_riscv' if srtos else 'baremetal'
    assert any('/code/section/' + core + '/section.c' in path for path in sources)
    assert not any('/code/section/' + ('baremetal' if srtos else 'srtos_riscv') + '/section.c'
                   in path for path in sources)
    link_inputs = (project / 'build/build.ninja').read_text(encoding='utf-8').replace('\\', '/').lower()
    assert not any(path in link_inputs for path in forbidden)
    for symbol in ('rtos_int_enter', 'rtos_int_exit'):
        assert ('--wrap=' + symbol in link_inputs) == srtos
    core_command = next(row['command'] for row in commands
                        if '/code/section/' + core + '/section.c' in row['file'].replace('\\', '/'))
    assert re.findall(r'-std=([^\s]+)', core_command)[-1] == 'c11'
    adapter_command = next(row['command'] for row in commands
                           if row['file'].replace('\\', '/').endswith('/bsp/bsp_wifi.c'))
    assert re.findall(r'-std=([^\s]+)', adapter_command)[-1] == 'gnu11'
    elf = project / 'build' / (project.name + '_demo.elf')
    output = subprocess.check_output([args.nm, '--defined-only', str(elf)], text=True)
    symbols = {parts[-1]: parts[-2] for line in output.splitlines() if len(parts := line.split()) >= 3}
    assert ('section_riscv_run' in symbols) == srtos
    assert ('__wrap_rtos_int_enter' in symbols) == srtos
    assert symbols['bsp_system_runtime_enter'].upper() == ('T' if srtos else 'W')
    assert symbols['bsp_system_runtime_exit'].upper() == ('T' if srtos else 'W')
    print(json.dumps(dict(platform=project.name, compilation_units=len(commands),
                          scheduler=core, cross_platform_references=0,
                          runtime_guard='scheduler' if srtos else 'no-op', passed=True)))


if __name__ == '__main__':
    main()
