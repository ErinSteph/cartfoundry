#!/usr/bin/env python3
"""Wrap a verified app in a drag-to-Applications disk image. Build tool only."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import dmgbuild

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('app', type=Path)
parser.add_argument('--output', type=Path, default=Path('dist'))
args = parser.parse_args()
app = args.app.resolve()
metadata = json.loads((app/'Contents/Resources/app-version.json').read_text())
assert app.name == 'CartFoundry.app' and metadata['product'] == 'CartFoundry'
assert metadata['platform'] in ('macos-arm64', 'macos-x86_64')
subprocess.run(['/usr/bin/codesign', '--verify', '--deep', '--strict', str(app)], check=True)
args.output.mkdir(parents=True, exist_ok=True)
name = f"CartFoundry-{metadata['version']}-{metadata['platform']}.dmg"
output = args.output.resolve()/name
settings = {
    'format': 'UDZO', 'filesystem': 'HFS+',
    'files': [str(app)], 'symlinks': {'Applications': '/Applications'},
    'background': 'builtin-arrow',
    'window_rect': ((160, 160), (640, 280)),
    'default_view': 'icon-view', 'include_icon_view_settings': True,
    'icon_size': 112, 'text_size': 15,
    'icon_locations': {'CartFoundry.app': (140, 120), 'Applications': (500, 120)},
    'show_toolbar': False, 'show_sidebar': False, 'show_status_bar': False,
    'badge_icon': str(app/'Contents/Resources/CartFoundry.icns'),
}
dmgbuild.build_dmg(str(output), 'CartFoundry — Drag to Applications', settings=settings)
subprocess.run(['/usr/bin/hdiutil', 'verify', str(output)], check=True)
digest = hashlib.sha256()
with output.open('rb') as stream:
    for chunk in iter(lambda: stream.read(1024*1024), b''):
        digest.update(chunk)
(output.parent/(name+'.sha256')).write_text(digest.hexdigest()+'  '+name+'\n')
print(output)
