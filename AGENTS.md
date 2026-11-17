# CartFoundry development

Work in this repository. CartFoundry owns its editor and game runtime; do not add
Godot or another game editor/engine to the development or end-user workflow.
Keep the desktop code portable to Windows and macOS. The current supporting UI
framework is Qt 6 Widgets (C++17); GBDK supplies the target compiler and hardware
support. Own the application-specific environment and formats wherever practical.

The editor preview and Game Boy ROM share runtime/cf_flow.c. Keep transitions in
that shared C code. Movement/camera/collision and actor behaviours are shared in
runtime/cf_world.c and runtime/cf_behavior.c; use them in previews and ROMs.
Variables, storage, random/terrain events, live text and event programs share runtime/cf_game.c; interaction/combat
rays share runtime/cf_raycast.c. Keep those implementations shared as well. The project model and builder are shared between the editor
and cf-build. End-user packages must contain their compiler tools and libraries;
no downloads, terminal setup or external compiler installations at runtime.

Build with CMake and run `ctest --test-dir build --output-on-failure` for changes
to the model, generator or runtime. Verify UI changes in the running app. ROM
integration tests need the pinned GBDK from scripts/fetch-gbdk.cmake. Keep generated
binaries in build/ or dist/ and retain supplied artwork under images/.

Windows portability is an implementation target, not yet a verified release.
Document platform verification honestly. Keep upstream toolchain license notices.

The project has named areas with chapter labels. Test Area builds an isolated
copy and disables area exits. mGBA is a supporting, unmodified open-source core
embedded for ROM tests; bootstrap it with scripts/fetch-mgba.cmake and retain its
MPL/source notices. Do not silently replace emulator tests with preview checks.

Sprite-sheet playback is shared in runtime/cf_animation.c. Atlas pixel data is
row-major, but generated sprite tile bytes are frame-major. Budget OAM/VRAM per
frame and ROM per entire sheet; preserve static assets with a 1x1 default grid.

Public event commands need an insertion form in editor/src/event_tools.cpp.
Keep the UI/function mapping in docs/editor-authoring.md current. Text layout is
shared in cf_flow.c; page rendering must keep the LCD enabled and only change
marker cells for cursor movement. Tests must read the active BG map and account
for CGB VRAM bank selection while rendering is in progress.
