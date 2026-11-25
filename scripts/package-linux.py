#!/usr/bin/env python3
"""Bundle a native Linux build, Qt libraries and the pinned Game Boy compiler.
Run on the oldest supported Linux distribution (Ubuntu 22.04 / glibc 2.35).
"""
import argparse
from collections import deque
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tarfile

SOURCE = Path(__file__).resolve().parents[1]

def run(*args):
    return subprocess.check_output([str(arg) for arg in args], text=True)

def elf(path):
    return path.is_file() and path.open('rb').read(4) == b'\x7fELF'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=Path('build'))
    parser.add_argument('--output', type=Path, default=Path('dist'))
    parser.add_argument('--gbdk', type=Path, default=SOURCE / 'tools/gbdk')
    args = parser.parse_args()
    root = args.output.resolve() / 'CartFoundry'
    if root.exists():
        shutil.rmtree(root)
    resources = root / 'resources'
    resources.mkdir(parents=True)
    for name in ('runtime', 'images', 'docs', 'examples', 'licenses'):
        shutil.copytree(SOURCE / name, resources / name, symlinks=True)
    shutil.copytree(args.build / 'documentation', resources / 'docs/versions', dirs_exist_ok=True)
    shutil.copytree(args.gbdk, resources / 'tools/gbdk', symlinks=True)
    shutil.copy2(SOURCE / 'THIRD_PARTY.md', resources)
    with tarfile.open(resources / 'licenses/mgba/source-0.10.5.tar.gz', 'w:gz') as archive:
        archive.add(SOURCE / 'third_party/mgba', arcname='mgba')
    for name in ('CartFoundry', 'cf-build', 'cf-update-helper'):
        shutil.copy2(args.build / name, root / name)
    shutil.copy2('/usr/bin/unzip', resources / 'tools/unzip')
    shutil.copy2('/usr/share/doc/unzip/copyright', resources / 'licenses/unzip-copyright.txt')
    qmake = shutil.which('qmake6') or shutil.which('qmake')
    plugins = Path(run(qmake, '-query', 'QT_INSTALL_PLUGINS').strip())
    for category in ('platforms', 'imageformats', 'multimedia', 'audio', 'tls', 'xcbglintegrations', 'platforminputcontexts'):
        if (plugins / category).exists():
            destination = root / 'plugins' / category
            destination.mkdir(parents=True)
            for plugin in (plugins / category).glob('*.so'):
                if category == 'platforms' and plugin.name not in ('libqxcb.so', 'libqoffscreen.so'):
                    continue
                shutil.copy2(plugin, destination / plugin.name)
    (root / 'qt.conf').write_text('[Paths]\nPlugins=plugins\n')
    # Keep glibc and the dynamic loader supplied by the host. Ship all other
    # dependencies, including those pulled in by Qt plugins and compiler tools.
    system = re.compile(r'^(ld-linux|lib(c|m|pthread|dl|rt|util|resolv|nss_[^.]+)\.so)')
    libdir = root / 'lib'
    libdir.mkdir()
    # Qt Network loads OpenSSL at runtime, so ldd alone does not discover it.
    for name in ('libssl.so.3', 'libcrypto.so.3'):
        shutil.copy2(Path('/usr/lib/x86_64-linux-gnu') / name, libdir / name)
    queue = deque(path for path in root.rglob('*') if not path.is_symlink() and elf(path))
    checked = set()
    dependency_sources = {Path('/usr/lib/x86_64-linux-gnu') / name for name in ('libssl.so.3', 'libcrypto.so.3')}
    while queue:
        binary = queue.popleft()
        if binary in checked:
            continue
        checked.add(binary)
        result = subprocess.run(['ldd', str(binary)], text=True, capture_output=True)
        if 'not found' in result.stdout:
            raise RuntimeError(f'Missing dependency of {binary}: {result.stdout}')
        for line in result.stdout.splitlines():
            match = re.match(r'\s*(\S+) => (/\S+)', line)
            if not match or system.match(match[1]):
                continue
            destination = libdir / match[1]
            if not destination.exists():
                dependency_sources.add(Path(match[2]).resolve())
                shutil.copy2(Path(match[2]).resolve(), destination)
                queue.append(destination)
    # Carry distribution copyright/license notices for every bundled dependency.
    notices = resources / 'licenses/linux-dependencies'
    notices.mkdir()
    for dependency in sorted(dependency_sources):
        owners = subprocess.run(['dpkg-query', '-S', str(dependency)], text=True, capture_output=True)
        if owners.returncode:
            owners = subprocess.run(['dpkg-query', '-S', '*' + dependency.name], text=True, capture_output=True)
        for line in owners.stdout.splitlines():
            package = line.split(': ', 1)[0].split(':', 1)[0]
            copyright_file = Path('/usr/share/doc') / package / 'copyright'
            if copyright_file.is_file():
                shutil.copy2(copyright_file, notices / (package + '.txt'))
    # Each executable/library can resolve the bundle without environment setup.
    for binary in checked:
        relative = os.path.relpath(libdir, binary.parent)
        rpath = '$ORIGIN' if relative == '.' else '$ORIGIN/' + relative
        subprocess.run(['patchelf', '--set-rpath', rpath, str(binary)], check=True)
    shutil.copy2(SOURCE / 'images/cf_icon.png', root / 'CartFoundry.png')
    (root / 'README.txt').write_text('CartFoundry for Linux x86_64\n\nExtract the complete folder and open CartFoundry.\nRequires Linux with glibc 2.35 or newer (Ubuntu 22.04 or newer), X11 or XWayland.\nQt, compiler tools and documentation are included. Keep the folder writable for automatic updates.\n')
    subprocess.run([sys.executable, str(SOURCE / 'scripts/make-update.py'), 'stamp', '--root', str(root), '--platform', 'linux-x86_64'], check=True)
    subprocess.run([sys.executable, str(SOURCE / 'scripts/make-update.py'), 'archive', '--root', str(root), '--output', str(args.output)], check=True)

if __name__ == '__main__':
    main()
