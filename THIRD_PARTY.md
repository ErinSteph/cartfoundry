# Third-party components

CartFoundry owns its editor, project format, code generation, preview and C runtime.
It currently uses these supporting components:

- Qt 6: Core, Gui, Widgets, Concurrent, Network, Multimedia (and Test for developer tests), dynamically
  linked. Qt is available under LGPLv3/GPLv3 or commercial terms. The package uses
  the open-source distribution; keep libraries replaceable and retain the applicable
  license notices. Official sources: https://code.qt.io/qt/qtbase.git and
  https://code.qt.io/qt/qtmultimedia.git
- GBDK-2020 4.5.0, including SDCC and lcc: bundled compiler/assembler/linker,
  Game Boy hardware support and IBM font. Upstream notices are retained under
  `tools/gbdk/licenses` and `tools/gbdk/README`. The compiler tools and target
  libraries have different licenses; the target libraries carry linking exceptions.
  Source and release: https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0

GBDK archives are pinned by SHA-256 in scripts/fetch-gbdk.cmake. No tools are
fetched at application runtime. Qt is bundled by the platform deployment tools.

Packages retain Qt license texts under `resources/licenses/qt` (inside
`Contents/Resources` on macOS). Libraries remain dynamically linked and replaceable.
Release builds use Qt 6.12.1 on macOS, Qt 6.8.3 on Windows, and Ubuntu 22.04's Qt
6.2.4 on Linux. Matching Qt sources are available from the official repositories
above under the corresponding version tags, or Ubuntu's qt6-base/qt6-multimedia
source packages. Linux packages also include distribution copyright notices for
bundled shared libraries and Info-ZIP unzip under `resources/licenses`.
Mac applications are ad-hoc signed; releases are not Apple-notarized or signed
with a Windows publisher certificate.

## Embedded emulator

mGBA 0.10.5 is linked statically, with its upstream UI disabled. Its unmodified
source files are under the Mozilla Public License 2.0 and include additional
upstream permissive notices. The exact source archive is available from:
https://github.com/mgba-emu/mgba/archive/refs/tags/0.10.5.tar.gz
SHA-256: 91d6fbd32abcbdf030d58d3f562de25ebbc9d56040d513ff8e5c19bee9dacf14

The pinned bootstrap is scripts/fetch-mgba.cmake. MPL text is included in
licenses/mgba/MPL-2.0.txt; upstream source and additional notices accompany local
packages under licenses/mgba/source-0.10.5.tar.gz. The CartFoundry integration
uses the public core API and leaves the emulator source unmodified.

runtime/cf_font.h contains the ASCII glyph subset of GBDK 4.5.0's f_ibm_sh.s.
It is used by our renderer and previews; GBDK's upstream notices and applicable
library license/linking exception remain in the bundled toolchain.

Windows packages also contain Microsoft's application-local Visual C++ runtime
DLLs from the Visual Studio redistribution directory; notices and exact file
versions are under `resources/licenses/msvc`. Qt's Windows multimedia backend
bundles FFmpeg 7.1 shared libraries under LGPL 2.1 or later; the license,
corresponding source link and original build configuration are retained under
`resources/licenses/ffmpeg`.
