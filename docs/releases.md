# Desktop releases

The app version is the three-number `project(CartFoundry VERSION ...)` value in
CMakeLists.txt. Packages stamp the same version into installation metadata and
include the corresponding help-book edition. Keep older documentation snapshots.

`.github/workflows/release.yml` builds native Apple Silicon, Intel Mac, Linux
x86_64 and Windows x86_64 applications. Release branches (`codex/release-*`) and
main run the complete test suite and retain downloadable ZIP artifacts. Only a
`vVERSION` tag publishes a GitHub Release. All four jobs must pass first.

The jobs fetch the hash-pinned GBDK and mGBA source. macOS and Windows use Qt
6.12.1; Ubuntu 22.04 uses its Qt 6.2 packages to retain a glibc 2.35 baseline.
macOS packages require 14.4 because that is the Qt binary deployment target.
Linux packages include non-glibc shared dependencies, Qt plugins and unzip.
Windows packages use windeployqt, including the Visual C++ runtime. Qt, GBDK and
mGBA license notices and the unmodified mGBA source archive travel with the app.

Each runner executes CTest, including actual ROM tests using mGBA and native
updater replacement/relaunch/rollback tests. `scripts/verify-package.py` then
runs the packaged compiler with development library paths removed, exports an
example ROM and launches the packaged GUI (under Xvfb on Linux). These automated
checks do not imply testing on every desktop, GPU or OS revision.

The tag publisher validates archive sizes and SHA-256 hashes, publishes all four
ZIPs and checksum files, checks that their URLs respond successfully, and only
then commits `updates/stable.json` to main. The feed uses canonical GitHub Release
asset URLs. Never publish a platform entry before its archive is downloadable.
The updater additionally checks asset availability and the complete downloaded
checksum before installing. Release artifacts are stored on GitHub Releases,
rather than committing compiled applications into the source repository.

For local packaging, use `scripts/package-macos.sh`,
`scripts/package-windows.ps1`, or `scripts/package-linux.py` on the target OS.
The Mac script accepts QT_PREFIX, GBDK_ROOT and DIST_DIR environment variables;
provide architecture-matched GBDK tools for an Intel build on Apple Silicon.
Linux packaging requires patchelf, unzip and the Ubuntu Qt development packages.
These are developer requirements; installed users need no compiler setup.

To publish an already successful release-branch build, run **Publish verified
desktop packages** from the repository's Actions page, on `main`, and enter its
Desktop packages run ID. It checks the completed run, source repository, branch,
workflow, version and all four archive hashes before creating the release at that
build's exact source commit. This promotes the tested artifacts without rebuilding
them. The updater feed changes only after all download URLs respond successfully.
The tag is created with the workflow token, so this promotion does not start a
second tag build. Directly pushing a version tag still runs the full build pipeline.

Windows packages explicitly copy the Visual C++ redistributable DLLs beside the
application. `check-windows-runtime.py` scans PE imports before the package smoke
test, so the runner's preinstalled runtime cannot hide missing app-local DLLs.
The runtime refresh workflow can add these libraries to an existing tested
Windows artifact, rerun package checks and update the published checksum/feed
without recompiling unchanged application code.

## Friendly installation downloads

The same compiled app is distributed in two packaging formats. Users install the
Mac `.dmg` by dragging CartFoundry to its Applications shortcut, or run the Windows
`-setup.exe` and launch CartFoundry from Start. The updater continues to download
the existing checksum-verified `.zip` payload; older clients remain compatible.

`scripts/package-macos-dmg.py` uses dmgbuild 1.6.5 on the build machine. It writes
Finder layout metadata directly, without opening or controlling Finder. The
image contains the signed app and an Applications shortcut. Its verifier mounts
it read-only, copies the app into a temporary Applications folder, verifies its
signature, builds a ROM and launches that copy.

`scripts/package-windows-installer.ps1` uses Inno Setup 6 on the build machine.
It produces one setup executable containing all program files and runtime DLLs.
Installation is per-user under `%LOCALAPPDATA%\Programs\CartFoundry`, without an
administrator prompt, and creates a Start menu shortcut and uninstall entry.
Uninstaller files are kept in `%LOCALAPPDATA%\CartFoundry\Installer`, outside the
app folder that automatic updates replace. Keep this location and AppId stable.
Uninstall removes installed program files while preserving user-created projects.

The Windows installer check performs a real silent installation on a disposable
native runner, exports a ROM and launches the installed app, replaces its whole
app directory, checks shortcuts/uninstall support again, and uninstalls while
checking that a user-created project survives.

For an existing release, run **Publish friendly installers** on `main`. It fetches
the published ZIPs using the stable feed's sizes and SHA-256 hashes, creates and
tests both Mac images and the Windows installer, and uploads their own checksum
files to that release. It never changes the updater feed or its ZIP assets.
Future Desktop packages builds create and verify these installers automatically.
Signing status is unchanged: Mac apps are ad-hoc signed and Windows installers
are unsigned; these wrappers do not provide notarization or a publisher certificate.
