"""Produce a binary carrying its source ELF identity without modifying that ELF."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--elf', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--objcopy', default='arm-none-eabi-objcopy')
parser.add_argument('--nm', default='arm-none-eabi-nm')
args = parser.parse_args()
symbols = subprocess.check_output([args.nm, '--defined-only', str(args.elf)], text=True)
address = next(int(row.split()[0], 16) for row in symbols.splitlines()
               if row.split()[-1] == '__firmware_elf_sha256')
subprocess.run([args.objcopy, '-O', 'binary', '-S', str(args.elf), str(args.output)], check=True)
image = bytearray(args.output.read_bytes())
offset = address - 0x08000000
if offset < 0 or offset + 32 > len(image) or image[offset:offset + 32] != bytes(32):
    raise ValueError('Firmware identity placeholder is missing or outside the load image')
digest = hashlib.sha256(args.elf.read_bytes()).digest()
image[offset:offset + 32] = digest
args.output.write_bytes(image)
args.output.with_suffix('.identity.json').write_text(json.dumps({
    'elf_sha256': digest.hex(), 'binary_sha256': hashlib.sha256(image).hexdigest(),
    'identity_address': address, 'identity_offset': offset, 'binary_bytes': len(image)
}, indent=2) + '\n', encoding='utf-8')
