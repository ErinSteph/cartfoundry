#!/usr/bin/env python3
"""Combine all four verified platform artifacts into one stable updater feed."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('artifacts', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--tag', required=True)
parser.add_argument('--verify-urls', action='store_true')
args = parser.parse_args()
version = args.tag.removeprefix('v')
platforms = ('macos-arm64', 'macos-x86_64', 'linux-x86_64', 'windows-x86_64')
feed = {'schema': 1, 'product': 'CartFoundry', 'version': version,
        'notes': 'Includes ROM size improvements, reusable animations, button events, pause menus, versioned help, MIDI import/export and automatic updates.', 'platforms': {}}
args.output.mkdir(parents=True, exist_ok=True)
for platform in platforms:
    directory = args.artifacts / platform
    metadata = json.loads((directory / 'stable.json').read_text())
    assert metadata['product'] == 'CartFoundry' and metadata['version'] == version
    item = metadata['platforms'][platform]
    name = f'CartFoundry-{version}-{platform}.zip'
    archive = directory / name
    assert item['url'] == f'https://github.com/ErinSteph/cartfoundry/releases/download/{args.tag}/{name}'
    assert item['size'] == archive.stat().st_size
    assert item['sha256'] == sha256(archive)
    if args.verify_urls:
        with urllib.request.urlopen(urllib.request.Request(item['url'], method='HEAD'), timeout=60) as response:
            assert response.status == 200
    feed['platforms'][platform] = item
    for file in (archive, directory / (name + '.sha256')):
        shutil.copy2(file, args.output / file.name)
# Human-facing installers are additional assets; the updater schema stays ZIP-only.
for platform, suffix in (('macos-arm64', '.dmg'), ('macos-x86_64', '.dmg'), ('windows-x86_64', '-setup.exe')):
    installer = args.artifacts/platform/f'CartFoundry-{version}-{platform}{suffix}'
    if installer.exists():
        checksum = installer.with_name(installer.name+'.sha256')
        assert checksum.read_text().split()[0] == sha256(installer)
        shutil.copy2(installer, args.output/installer.name)
        shutil.copy2(checksum, args.output/checksum.name)
(args.output / 'stable.json').write_text(json.dumps(feed, indent=2) + '\n')
print('Verified four platform archives for', args.tag)
