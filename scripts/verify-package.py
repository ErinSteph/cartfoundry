#!/usr/bin/env python3
"""Exercise installed binaries without development Qt/compiler search paths."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('root', type=Path)
args = parser.parse_args()
root = args.root.resolve()
mac = root.suffix == '.app'
windows = os.name == 'nt'
resources = root / ('Contents/Resources' if mac else 'resources')
binaries = root / 'Contents/MacOS' if mac else root
suffix = '.exe' if windows else ''
metadata = json.loads((resources / 'app-version.json').read_text())
assert (resources / 'docs/versions' / metadata['version'] / 'book.json').is_file(), 'Missing versioned help book'
for name in ('runtime/main.c', 'THIRD_PARTY.md', 'licenses/mgba/MPL-2.0.txt'):
    assert (resources / name).is_file(), name
assert (resources / 'tools/gbdk/bin' / ('lcc' + suffix)).is_file()
assert (binaries / ('cf-update-helper' + suffix)).is_file()
env = {key: value for key, value in os.environ.items() if not key.startswith(('QT_', 'QML', 'DYLD_', 'LD_'))}
if windows:
    env['PATH'] = str(root) + os.pathsep + str(Path(os.environ['SystemRoot']) / 'System32') + os.pathsep + os.environ['SystemRoot']
else:
    env['PATH'] = '/usr/bin:/bin'
cli = str(binaries / ('cf-build' + suffix))
version = subprocess.check_output([cli, '--version'], env=env, text=True).strip()
assert version == 'CartFoundry ' + metadata['version'], version
with tempfile.TemporaryDirectory(prefix='cartfoundry-package-') as work:
    project, rom = str(Path(work) / 'example.cartfoundry'), Path(work) / 'example.gbc'
    subprocess.run([cli, '--example-overworld', project], env=env, cwd=work, check=True)
    subprocess.run([cli, project, str(rom)], env=env, cwd=work, check=True)
    assert rom.stat().st_size >= 32768
# Start the actual packaged window using the native platform plugin. Xvfb is
# supplied by the Linux release runner; no offscreen plugin masks X11 failures.
process = subprocess.Popen([str(binaries / ('CartFoundry' + suffix))], cwd=root.parent, env=env)
try:
    time.sleep(4)
    assert process.poll() is None, 'Packaged GUI exited during startup'
finally:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
print('Verified bundled compiler, help book, ROM export and GUI:', metadata['platform'], metadata['version'])
