#!/usr/bin/env python3
"""Mount a disk image, check its layout, copy the app and test the installed copy."""
import argparse
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('image', type=Path)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='cf-dmg-', dir='/tmp') as work:
    mount = Path(work)/'mount'
    result = subprocess.check_output(['/usr/bin/hdiutil', 'attach', '-readonly', '-nobrowse', '-mountpoint', str(mount), '-plist', str(args.image.resolve())])
    try:
        assert (mount/'Applications').is_symlink()
        assert (mount/'Applications').readlink() == Path('/Applications')
        assert (mount/'.DS_Store').is_file()
        assert (mount/'CartFoundry.app').is_dir()
        installed = Path(work)/'Applications/CartFoundry.app'
        installed.parent.mkdir()
        subprocess.run(['/usr/bin/ditto', str(mount/'CartFoundry.app'), str(installed)], check=True)
    finally:
        subprocess.run(['/usr/bin/hdiutil', 'detach', str(mount)], check=True)
    subprocess.run(['/usr/bin/codesign', '--verify', '--deep', '--strict', str(installed)], check=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name('verify-package.py')), str(installed)], check=True)
print('Verified drag-to-Applications disk image:', args.image)
