#!/usr/bin/env python3
"""Check published human-friendly downloads independently of the updater feed."""
import json
from pathlib import Path
import urllib.request

source = Path(__file__).resolve().parents[1]
feed = json.loads((source/'updates/stable.json').read_text())
base = f"https://github.com/ErinSteph/cartfoundry/releases/download/v{feed['version']}/CartFoundry-{feed['version']}-"
for suffix in ('macos-arm64.dmg', 'macos-x86_64.dmg', 'windows-x86_64-setup.exe'):
    url = base+suffix
    for target in (url, url+'.sha256'):
        with urllib.request.urlopen(urllib.request.Request(target, method='HEAD'), timeout=60) as response:
            assert response.status == 200 and int(response.headers['Content-Length']) > 0
    print('Available:', url)
assert all(p['url'].endswith('.zip') for p in feed['platforms'].values())
print('Updater continues to use its existing verified ZIP payloads')
