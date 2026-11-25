#!/usr/bin/env python3
"""Create CartFoundry installation metadata and a reproducible update ZIP/feed.
Development/release tool only; Python is never required by installed clients.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import zipfile

PLATFORMS = {"macos-arm64", "macos-x86_64", "windows-arm64", "windows-x86_64", "linux-x86_64", "linux-arm64"}

def version_from_cmake():
    text = (Path(__file__).resolve().parents[1] / "CMakeLists.txt").read_text()
    return re.search(r"project\(CartFoundry VERSION ([0-9]+\.[0-9]+\.[0-9]+)", text).group(1)

def metadata_path(root):
    return root / ("Contents/Resources/app-version.json" if root.suffix == ".app" else "resources/app-version.json")

def stamp(root, platform, version):
    if platform not in PLATFORMS or not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise ValueError("Invalid platform or version")
    expected = "CartFoundry.app" if platform.startswith("macos-") else "CartFoundry"
    if root.name != expected:
        raise ValueError("Package root must be named " + expected)
    if platform.startswith("windows-"):
        with (root / "CartFoundry.exe").open("rb") as stream:
            stream.seek(0x3c)
            pe = int.from_bytes(stream.read(4), "little")
            stream.seek(pe)
            if stream.read(4) != b"PE\0\0":
                raise ValueError("Invalid Windows executable")
            machine = int.from_bytes(stream.read(2), "little")
        if machine != {"windows-x86_64": 0x8664, "windows-arm64": 0xaa64}[platform]:
            raise ValueError("Windows executable architecture does not match --platform")
    if platform.startswith("linux-"):
        header = (root / "CartFoundry").read_bytes()[:20]
        if header[:4] != b"\x7fELF" or header[4:6] != b"\x02\x01" or int.from_bytes(header[18:20], "little") != {"linux-x86_64": 62, "linux-arm64": 183}[platform]:
            raise ValueError("Linux executable architecture does not match --platform")
    path = metadata_path(root)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"product": "CartFoundry", "version": version, "platform": platform}, indent=2) + "\n")

def make_archive(root, output):
    paths = [root]
    for directory, dirs, files in os.walk(root, followlinks=False):
        paths.extend(Path(directory) / name for name in sorted(dirs + files))
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=False) as archive:
        for path in paths:
            name = path.relative_to(root.parent).as_posix()
            if path.is_symlink():
                target = os.readlink(path)
                if os.path.isabs(target) or ".." in target.split("/"):
                    raise ValueError("Update symlinks must be relative and descend within the bundle: " + str(path))
                info = zipfile.ZipInfo(name)
                info.create_system = 3
                info.external_attr = (stat.S_IFLNK | 0o777) << 16
                info.compress_type = zipfile.ZIP_STORED
                archive.writestr(info, target.encode("utf-8"))
            else:
                archive.write(path, name)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["stamp", "archive"])
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--platform", choices=sorted(PLATFORMS))
    parser.add_argument("--version", default=version_from_cmake())
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.action == "stamp":
        if not args.platform:
            parser.error("stamp needs --platform")
        stamp(args.root, args.platform, args.version)
        return
    metadata = json.loads(metadata_path(args.root).read_text())
    if metadata["version"] != args.version or metadata["platform"] not in PLATFORMS or metadata["product"] != "CartFoundry":
        parser.error("Package metadata does not match the release")
    output = args.output or Path("dist")
    output.mkdir(parents=True, exist_ok=True)
    version, platform = metadata["version"], metadata["platform"]
    name = f"CartFoundry-{version}-{platform}.zip"
    archive = output / name
    make_archive(args.root, archive)
    digest = hashlib.sha256()
    with archive.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    item = {"url": f"https://github.com/ErinSteph/cartfoundry/releases/download/v{version}/{name}", "size": archive.stat().st_size, "sha256": digest.hexdigest()}
    feed_path = output / "stable.json"
    feed = json.loads(feed_path.read_text()) if feed_path.exists() else {}
    if feed.get("version") != version:
        feed = {"schema": 1, "product": "CartFoundry", "version": version, "notes": "", "platforms": {}}
    feed["platforms"][platform] = item
    feed_path.write_text(json.dumps(feed, indent=2) + "\n")
    (output / (name + ".sha256")).write_text(digest.hexdigest() + "  " + name + "\n")
    print(archive)
    print("Publish the ZIP to GitHub release v" + version + "; publish " + str(feed_path) + " as updates/stable.json only after uploading packages.")

if __name__ == "__main__":
    main()
