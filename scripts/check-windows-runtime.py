#!/usr/bin/env python3
"""Check PE imports so a preinstalled C++ runtime cannot mask missing DLLs."""
from pathlib import Path
import re
import struct
import sys

def imports(path):
    data = path.read_bytes()
    if data[:2] != b'MZ':
        return []
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0':
        raise ValueError('Invalid PE executable: ' + str(path))
    count = struct.unpack_from('<H', data, pe+6)[0]
    optional_size = struct.unpack_from('<H', data, pe+20)[0]
    optional = pe+24
    magic = struct.unpack_from('<H', data, optional)[0]
    directories = optional + (112 if magic == 0x20b else 96)
    import_rva = struct.unpack_from('<I', data, directories+8)[0]
    if not import_rva:
        return []
    sections = optional+optional_size
    def offset(rva):
        for i in range(count):
            size, address, raw_size, raw = struct.unpack_from('<IIII', data, sections+i*40+8)
            if address <= rva < address+max(size, raw_size):
                return raw+rva-address
        raise ValueError('Invalid PE import RVA: ' + str(path))
    position = offset(import_rva)
    names = []
    while any(struct.unpack_from('<IIIII', data, position)):
        name = offset(struct.unpack_from('<I', data, position+12)[0])
        names.append(data[name:data.index(b'\0', name)].decode('ascii'))
        position += 20
    return names

root = Path(sys.argv[1]).resolve()
files = {p.name.lower(): p for p in root.iterdir() if p.is_file()}
required = set()
for path in root.rglob('*'):
    if path.suffix.lower() not in ('.exe', '.dll'):
        continue
    for name in imports(path):
        if re.match(r'(?i)^(msvcp|vcruntime|concrt)\d+.*\.dll$', name):
            required.add(name.lower())
            if name.lower() not in files and not (path.parent/name).is_file():
                raise SystemExit(f'{path.name} requires missing app-local {name}')
assert required, 'No C++ runtime dependencies found in Windows package'
print('Verified app-local runtime DLLs:', ', '.join(sorted(required)))
