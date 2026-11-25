# Application updates (0.9.0 onward) — CartFoundry 0.9.1

CartFoundry checks this public HTTPS feed after the editor opens:

`https://raw.githubusercontent.com/ErinSteph/cartfoundry/main/updates/stable.json`

Older applications without the updater cannot check this feed. Install 0.9.0
manually once; subsequent packaged installations have update support ready.
Development builds without bundled metadata/helper do not check automatically.

## Version and platform rules

CMake's project version is the source of truth. Versions contain exactly three
nonnegative integers: `version.subversion.revision`. Comparison is numeric, from
left to right: `0.7.3 > 0.7.2`, `0.10.0 > 0.9.9`, and `1.0.0 > 0.99.99`.
Equal and older releases are never offered. Prerelease labels are not supported.
The version is embedded in the application, macOS plist, CLI and package metadata.

The manifest has this shape (hash/size below are illustrative):

```json
{
  "schema": 1,
  "product": "CartFoundry",
  "version": "0.9.1",
  "notes": "Changes in this release.",
  "platforms": {
    "macos-arm64": {
      "url": "https://github.com/ErinSteph/cartfoundry/releases/download/v0.9.1/CartFoundry-0.9.1-macos-arm64.zip",
      "size": 123456,
      "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
    }
  }
}
```

Supported keys are `macos-arm64`, `macos-x86_64`, `windows-x86_64`, and
`windows-arm64`, `linux-x86_64` and `linux-arm64`. Publish only platforms with an actual, tested package. Missing
platform entries do not produce an update offer. The client also checks that the
package URL responds successfully before asking. It falls back to a one-byte
range GET if HEAD is unsupported. Network/TLS/feed failures are quiet during
automatic checks; manual checks explain the failure.

## Prompts and preferences

- **Update** downloads and prepares that release. The editor's normal unsaved-work
  and build-in-progress checks still run before closing. Cancel keeps the editor.
- **Later**, including closing the prompt, allows another offer next launch.
- **Don't ask automatically** disables launch prompts persistently. **Help → Check
  for Updates…** still works. **Help → Ask about updates on launch** re-enables them.
- Starting a successfully installed newer version resets the suppression flag.
  A failed update or a repeated launch of the same version does not reset it.

## Installation and recovery

Updates replace the entire application, including Qt, runtime sources, compiler,
artwork and license notices. Projects and application-data saves stay outside the
installation and are not replaced.

The client downloads to its per-user cache, enforces the advertised size, and
checks SHA-256 before extracting. ZIP local/directory paths, types and symbolic
links are validated before an OS extractor runs. Archives cannot write outside
the package; standard relative Qt framework links are supported. Compressed or
escaping symlinks, entries below symlinks, split/ZIP64 archives, encrypted entries,
and oversized payloads are rejected. Only this repository's HTTPS GitHub release
ZIP URLs are accepted; HTTPS redirects are allowed without downgrading to HTTP.
macOS additionally verifies the bundle's code signature. Checksums are integrity
checks authenticated by the HTTPS GitHub feed, not an independent release-signing
key. Protect the repository/release account accordingly. Current local macOS
packages use ad-hoc signing; Developer ID/notarization remains a release concern.

A separate helper runs from a private copy outside the app being replaced, so its
own executable and Qt libraries do not lock the destination. It waits for the
editor process, checks for other running copies, prepares an incoming directory
beside the installation, and switches directories with a rollback copy retained.
The new app confirms startup through a private receipt. A failed launch restores
and relaunches the previous app. If the new app is running but cannot confirm, it
is not forcibly killed; the rollback copy is retained for recovery.

One prior installation is kept in `.CartFoundry.previous-update.app` on macOS or
`.CartFoundry.previous-update` on Windows and Linux, beside the app. The next update replaces
that backup only after checking its product identity/version and preparing the
new installation. Completed cache jobs are removed on later launches after a day.
The helper never forces an editor shutdown or silently elevates privileges.
Automatic installation requires a writable installation folder; otherwise the
old app remains and a manual installation is needed. macOS bundles must be named
`CartFoundry.app`; Windows and Linux portable installs must be in a `CartFoundry` folder.

macOS uses its built-in `ditto`; Windows uses the built-in System32 `tar.exe`.
Linux includes an `unzip` executable and its license in the package.
Installed clients require no Python, terminal setup or external compiler downloads.
The release workflow tests replacement, relaunch, running-process protection and
rollback on native macOS, Windows and Linux runners before publishing packages.
Runner verification does not cover every desktop environment or hardware model.

## Publishing

1. Increase `project(CartFoundry VERSION ...)` in `CMakeLists.txt`.
2. Run `scripts/package-macos.sh` , `scripts/package-windows.ps1` or `scripts/package-linux.py`. Packaging stamps
   `app-version.json`, includes the helper/TLS backend, and generates a ZIP,
   checksum and `dist/stable.json`. Use a single architecture per package.
   Release tooling requires Python 3 on the build machine only. On Windows pass
   `-Architecture x86_64` or `-Architecture arm64`; PE architecture is checked.
3. Verify the app opens and builds a ROM with its bundled compiler. Exercise an
   update from the previous release and its rollback path on that platform.
4. Upload the ZIP(s) to GitHub Release `v<version>`. `make-update.py archive` merges
   packages of the same version into the local `dist/stable.json`; a new version
   starts with no old platform entries. Do not mix packages from different versions.
5. Add release notes, then copy the generated feed to `updates/stable.json` and
   commit/push that file to `main` **after** uploading the release assets.

The scripts do not upload releases or push source. A local/generated feed is not
live until published to the above GitHub path. The client skips missing assets,
so publishing metadata accidentally before an asset does not offer a broken link.

The ZIP tool preserves permissions and stores symlink targets uncompressed.
Use the generated archives for updates rather than arbitrary third-party ZIPs.
`cf-update-helper --verify-archive <zip>` performs a read-only archive check.
