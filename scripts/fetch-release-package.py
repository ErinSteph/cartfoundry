#!/usr/bin/env python3
"""Download a published updater payload and check its recorded size and SHA-256."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('platform')
parser.add_argument('--output', type=Path, default=Path('build/installer-payload'))
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
feed = json.loads((source/'updates/stable.json').read_text())
item = feed['platforms'][args.platform]
expected = f"https://github.com/ErinSteph/cartfoundry/releases/download/v{feed['version']}/CartFoundry-{feed['version']}-{args.platform}.zip"
assert item['url'] == expected
args.output.mkdir(parents=True, exist_ok=True)
archive = args.output / expected.rsplit('/', 1)[1]
hash_value = hashlib.sha256()
with urllib.request.urlopen(urllib.request.Request(expected, headers={'Cache-Control': 'no-cache'}), timeout=60) as response, archive.open('wb') as stream:
    for block in iter(lambda: response.read(1024*1024), b''):
        hash_value.update(block)
        stream.write(block)
assert archive.stat().st_size == item['size'] and hash_value.hexdigest() == item['sha256'], 'Published payload checksum mismatch'
if args.platform.startswith('macos-'):
    subprocess.run(['/usr/bin/ditto', '-x', '-k', str(archive), str(args.output)], check=True)
else:
    import zipfile
    with zipfile.ZipFile(archive) as bundle:
        bundle.extractall(args.output)
print('Verified released payload:', args.platform, feed['version'])

if os.environ.get('GITHUB_OUTPUT'):
    with open(os.environ['GITHUB_OUTPUT'], 'a') as output:
        output.write('version='+feed['version']+'\n')
