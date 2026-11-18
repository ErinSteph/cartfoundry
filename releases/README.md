# CartFoundry 0.8.1

Download `CartFoundry-0.8.1-macOS-arm64.zip`, extract it, and open CartFoundry.app.
The package contains the editor, embedded mGBA core, GBDK compiler, runtime,
examples, documentation and third-party notices; no toolchain download is needed
at application runtime. The archive preserves the signed application bundle.

This is a preview build for macOS Apple Silicon, ad-hoc signed and not notarized.
A verified Windows binary is not yet available. Source-build instructions are in
the repository README. No upstream toolchain or emulator sources were modified.

## Changes

- Import & place sprite directly from the map toolbar; import selects the brush.
- Double-click existing sprite assets to place them; pointer previews the sprite.
- Select, move, duplicate and stamp map objects, NPCs and sprites.
- Readable target IDs, automatic event-reference updates and map templates.
- Explicit default scenes, scene entry scripts and a menu-screen builder.
- Dedicated script workspace, GB/GBC splash art, timers and configurable text.

## Verification

The 0.8 baseline passed all 11 then-existing suites, including actual GB/GBC ROM
execution. The added canvas interaction suite and map-authoring suite pass, and
the 0.8.1 import-to-placement flow was verified in the running editor. The macOS
application passed deep strict code-signature verification before archiving.

Generated build directories, downloaded dependencies and loose ROM exports are
excluded from source control. Bootstrap scripts retrieve pinned developer tools.
The packaged build is kept here as a versioned release artifact. Future binaries
can be attached to GitHub Releases instead of accumulating them in Git history.
