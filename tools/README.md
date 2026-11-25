# Supporting build tools

GBDK-2020 **4.5.0** is the pinned compiler/assembler/linker and Game Boy hardware
library. Fetch it with `cmake -P scripts/fetch-gbdk.cmake`; this selects the native
macOS, Windows or Linux archive and verifies its SHA-256. The downloaded directory
is excluded from version control and copied into application bundles at packaging.
The application never runs a package manager or downloads build tools.

Official upstream release: https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0

`gbdk/licenses/`, `gbdk/README`, the library linking exceptions and SDCC notices
must travel with the bundled toolchain. Our C runtime is under ../runtime/.
