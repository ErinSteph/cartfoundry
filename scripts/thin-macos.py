#!/usr/bin/env python3
"""Keep only the release architecture in every bundled Mach-O, including GBDK."""
import os
from pathlib import Path
import subprocess
import sys
root, architecture = Path(sys.argv[1]), sys.argv[2]
magic = {bytes.fromhex(x) for x in ('feedface','feedfacf','cefaedfe','cffaedfe','cafebabe','bebafeca','cafebabf','bfbafeca')}
for path in root.rglob('*'):
    if path.is_symlink() or not path.is_file():
        continue
    with path.open('rb') as stream:
        if stream.read(4) not in magic:
            continue
    arches = subprocess.check_output(['lipo','-archs',str(path)],text=True).split()
    if architecture not in arches:
        raise SystemExit(f'{path}: {arches} does not contain {architecture}')
    if len(arches)>1:
        temporary = str(path)+'.thin'
        subprocess.run(['lipo',str(path),'-thin',architecture,'-output',temporary],check=True)
        os.chmod(temporary,path.stat().st_mode)
        os.replace(temporary,path)
