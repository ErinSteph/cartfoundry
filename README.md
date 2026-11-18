<img width="600" alt="cf_banner" src="https://github.com/user-attachments/assets/94da055e-818f-4c53-b810-1d4eaafa111d" />

# CartFoundry

A cross-platform Game Boy / Game Boy Color engine and C++/Qt desktop
editor. The application bundles a compiler
and mGBA emulator core.

<img width="600" alt="Screenshot 2026-11-18 at 18 27 55" src="https://github.com/user-attachments/assets/c65258d8-c840-479e-9c0d-6bdc1d44248c" />
<img width="600" alt="Screenshot 2026-11-18 at 18 27 13" src="https://github.com/user-attachments/assets/efd70e59-db69-4632-a9cb-8ca018c3aed3" />
<img width="600" alt="Screenshot 2026-11-18 at 18 24 32" src="https://github.com/user-attachments/assets/169428b2-e1e8-43d2-a7cb-4b18aab1a638" />
<img width="600" alt="Screenshot 2026-11-18 at 18 25 56" src="https://github.com/user-attachments/assets/71010156-80ab-426b-8ea4-84728e0babdd" />


## Download

The current packaged build is **0.8.1**, for **macOS Apple Silicon (arm64)**.
See [release notes](releases/README.md) and [SHA-256 checksums](releases/SHA256SUMS).
Windows is a source portability target, not a tested binary release.

## Try it

Download [CartFoundry 0.8.1 for macOS Apple Silicon](releases/CartFoundry-0.8.1-macOS-arm64.zip), unzip it, and open `CartFoundry.app`. **File → New Pokemon Example** opens Pocket Meadow,
a small adventure with grass encounters, battles, catching, healing and a party PC.
**Help → Pokemon example guide** contains the complete editor recipe, including
all variables, items, scripts, prompts and artwork. See [the guide](examples/pokemon/README.md).

For a simpler world, start with **File → New Overworld Example**, or open
`examples/overworld/game.cartfoundry`. It includes Foundry Fields and a Walled
Garden in two chapters. The original branching conversation is under
`examples/first-conversation/`.

- **File → Project Settings** chooses the Default Scene, target and platform splash
  duration. Startup is explicit: splash → default scene → its `onSceneEnter`.
- **Scenes & screen builder:** author menu screens from text, images, panels and
  buttons. Drag/resize on the tile grid, edit actions and conditions, then Preview
  or Test Scene. **File → New Scenes & Screens Example** demonstrates the flow;
  see [the scene guide](examples/scenes/README.md).
- **Scripts:** choose an element type, element and event; edit source with event
  forms and a reference browser. Double-click references to insert their IDs or
  commands. A second tab shows the live read-only project JSON.
- **Map selection and placement:** click and drag objects/sprites, edit their
  properties and script IDs in the map inspector, and stamp entries from a
  searchable palette of existing objects, sprites and NPCs. **+ New map** supports
  empty maps and existing-area templates. See [the map guide](docs/map-authoring.md).
- **Sprites, tilesets & overworld:** choose an area, import/replace images and
  set its tileset/player. Import lets you select dimensions, fit/crop/stretch,
  scaling filter and background removal, with a live four-colour preview.
- Choose a tile and paint the map. The tool selector also paints solid collision,
  spawn points, prompt interactions and area exits. Left drag paints, right drag
  erases. Collision is red while that layer is selected; exits are blue.
- Name areas and give them chapter labels. Add/duplicate areas and **Use as scene start**
  to choose the initial area within its parent world scene. Exits link to a destination's spawn point.
- **Conversations & textboxes:** wire prompt choices and edit their text. Adjust
  textbox X/Y/width/height in the Inspector; overflow is reported before export.
- Switch **Game Boy / Game Boy Color** with the toolbar or **Target** menu.
  Monochrome previews/exports use four shades; colour assets remain intact.
- **Play overworld** previews the selected area immediately with shared movement
  and collision code. Click the viewport: arrows/WASD move, Z/Enter interacts,
  X/Escape attacks or closes; Backspace opens inventory. This is an editor simulation, without a compiler step.
- **Build & Play This Area** compiles an isolated test ROM into the built-in
  emulator. Area exits are disabled for this test. **Build & Play** runs the
  complete project. Neither requires saving changes first.
- The **ROM emulator** tab uses mGBA 0.10.5. Click its screen: arrows move, Z=A,
  X=B, Enter=Start, Backspace=Select. It has on-screen controls, pause and reset,
  and can load existing GB/GBC/GBA ROMs. Audio output is currently muted.
- **Save** stores a self-contained `.cartfoundry` file. **Build ROM…** exports
  `.gb` or `.gbc`. On a handheld, A interacts, B attacks/closes, Select opens inventory and Start resets position.

## Develop

Install a C++17 compiler, CMake 3.21+ and Qt 6 Core/Gui/Widgets/Concurrent/Test/Multimedia.
Install `qtbase` and `qtmultimedia` on Homebrew. Bootstrap the pinned third-party tools:

```sh
cmake -P scripts/fetch-gbdk.cmake
cmake -P scripts/fetch-mgba.cmake
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

macOS Qt prefix: `/opt/homebrew/opt/qtbase`. On Windows, use a Qt MSVC kit and
matching Visual Studio compiler. mGBA is compiled into the application, with its
upstream UI disabled. Source archives and compiler downloads are checksum-pinned.

```sh
build/cf-build examples/overworld/game.cartfoundry build/overworld.gbc
```

Package a local Mac app with `scripts/package-macos.sh`. Windows packaging is
provided by `scripts/package-windows.ps1`, with `windeployqt` on PATH. Windows
and physical hardware have not been validated here yet.

## Code layout and current limits

- `editor/src/`: our asset pipeline, project model, graph/map editors and build UI.
- `runtime/`: shared C movement/flow and hardware renderer/area transitions.
- `tests/`: model/converter/build tests, C runtime tests and actual ROM execution
  in the embedded emulator (collision, scrolling, dialogue, exits and DMG mode).
- `tools/gbdk/`, `third_party/mgba/`: pinned supporting components, not tracked.
- `docs/project-format.md`: schema, hardware budgets and testing semantics.

This milestone supports animated sprite sheets and one four-colour tileset per
area. The **Sound & music** tab composes four-channel songs and effects, with
a piano roll, pulse envelopes/sweep, editable wave RAM, noise drums and stereo controls. Scripted sound loops and one-shots play in the preview and ROM
emulator (`sound loop pulse`, `sound play chime`, `sound stop`); use **+ Event → Sound**.
Imported music, game saves, text pagination
remain future work. Eight named areas are supported with MBC5 banked event and
screen resources; runtime/gameplay tables still have fixed-bank limits. See THIRD_PARTY.md for supporting licenses.

## Controls and scripts

The **Controls & behaviours** tab configures hardware buttons and character
scripts per area. Select Player or add an enemy/character, choose its sprite and
spawn, then edit its script or apply a template. The templates are top-down
movement, patrol, chase and idle. Their movement and collision code is shared
with the ROM. Add `on_touch <prompt ID>` to an enemy script to trigger a
conversation on contact; the tab lists the available prompt IDs.

The overworld example includes a purple patrolling field keeper and a chasing
garden guardian. Scripts currently provide fixed behaviour commands rather than
a general programming language. Combat uses forward rays, with configurable ranges, NPC health, and hit/defeat events. Pathfinding remains future work.

## Sprite-sheet animations

Use **Sprite animations → Import sprite sheet…**. Set the sheet's width and height
in frames and the output pixel size of each frame. Source cells are scaled
individually and share one palette, preserving frame boundaries and transparency.

Select the sheet and add named animations. Pick inclusive first/last frame
numbers, the **total animation duration** in seconds, and whether to loop. The
sheet is numbered from zero, left-to-right then top-to-bottom. Click a frame to
set the start or Shift-click to set the end. **Play / Fire** replays from the
start; **Stop** freezes playback. A non-looping clip holds its final frame.

Bind clips by name in character scripts:

```text
behavior top_down
speed 1
every 1
animation idle
on_move walk
on_idle idle
on_interact wave
```

`animation` selects the default clip; move/idle bindings switch automatically.
`on_interact` fires when the configured Interact button is pressed. One-shot
actions finish before returning to the normal clip. Looping actions can be
interrupted by movement. Dialogue pauses animation along with world simulation.
Existing sprites remain single-frame assets until you add a grid/animations.

Each frame is 8–32 pixels per side in multiples of eight. Sheets support up to
8×8 frames and 32 named clips. Duration is quantized to 60 Hz ticks, with at least
one tick per selected frame; GB/GBC hardware runs at approximately 59.7 Hz.
Only the current frame occupies sprite VRAM; all frame graphics count toward the
ROM budget. The example now contains idle, walk and one-shot wave animations.

## Game data, scripts and storage

**Project & game data** provides a Project → Scene → Map → Area → Object browser.
Select a node to edit it, use **+ Add** for new nodes/library entries, and choose
**Open map editor** on an area to paint it. Areas can move between maps; maps can
move between scenes. This grouping organises the project; area exits control travel.

The same tab edits variables, reusable scripts, items, storage and NPC templates.
Items have stack limits, optional icons, descriptions and use scripts. Storage
has a capacity in stack slots and editable initial counts. The first storage is
always `inventory`. Add a container object to an area, choose its storage, graphic
tile, position and solidity. Interacting opens it; choose moves one item, and
Select switches between container and inventory. Nonempty interaction scripts
replace the default open action; use `open storage_id` to open explicitly.

NPC templates contain sprites, behaviours, health and event scripts. **Place NPC**
copies a template into the selected area; placed instances remain independently
editable in the hierarchy. B attacks the first target along the facing ray and
deals one damage. Walls block rays; solid interactive objects can still be hit.
Health resets on area entry. Variables and storage survive area transitions but
are currently session state, without cartridge save persistence.

Dialogue choices have an **Enabled when** condition and **On choose** script;
prompts and areas have entry scripts, and NPCs have interact/hit/defeat scripts.
Locked choices display `x` and are skipped. For example:

```text
if coins >= 5
  add coins -5
  give inventory key 1
else
  dialogue quest
end
```

Commands include `set`, `add`, `give`, `take`, `move`, `open`, `dialogue`, and
`call script_id`. Conditions compare variables (`coins >= 5`) or item counts
(`has inventory key >= 1`). Numeric operands can reference another variable,
with an optional `$` prefix. `result` reports operation success. Validation
reports unknown references, missing `end`, cycles and exceeded limits.
The preview and ROM use the same bounded C interpreter and storage/raycast code.

Spritesheets and tilesheets have **Fit**, percentage, plus/minus and Ctrl/Cmd-wheel
zoom with scrollbars. Map and tile-thumbnail scale controls are on the world tab.
The overworld example includes a chest, items, a variable-gated choice and NPC rewards.

## Encounters, live text and pixel painting

The area inspector in **Project & game data** has terrain step events: choose
an original tileset tile number, an interval in tile entries, a probability and
an ordinary event script. Standing still does not retrigger a tile. `random roll
1 100` provides inclusive random rolls; `sub hp damage` subtracts another variable;
`resume` returns from dialogue to the world; `view party` browses item entries without transfers. Prompt body text supports `{hp}` and
other declared variable names. **Modal dialogue** prevents Back, Bag and position
reset during an encounter. A prompt may select a portrait sprite; keep its
textbox below the portrait. These features use shared C code in previews and ROMs.

**Pixel art** creates blank sprites/tilesets, paints all indexed image pixels,
edits palettes and exports PNGs. It supports mouse strokes, keyboard arrows/Space,
exact X/Y pixel edits, erasing, grid and zoom. Sample assets can be modified here,
or imported from the PNGs supplied in the Pokémon example's assets folder.

## Versatile authoring controls (0.6)

Dialogue cursor movement no longer blanks the screen. Page changes render into a
hidden background map and swap at a vertical blank. Animation clips can reverse
or mirror without duplicating pixels, and the builder shares identical graphics.
The animation inspector and behavior forms expose the new controls.

Text styles now configure borders, title/footer visibility, padding, body
alignment, choice spacing/markers, custom footers and colours. Event scripts
throughout the editor have **+ Event**, **+ If** and **Check script** tools; choices
have a **Condition** form. The project root sets an initial RNG seed. A timer
library provides nonblocking one-shot/repeating events, seconds/ticks and pause
policy; Start/Stop/Reset and Seed events are available in the event chooser.
Maps gain rectangle/flood filling and Alt-click tile picking.

See [authoring controls and engine coverage](docs/editor-authoring.md) for the full
UI/function mapping, semantics, references and remaining engine limits.

The **Sound & music → + Demo song** button loads *Foundry After Hours*: an eight-bar
150 BPM song using both pulse channels, wave bass, and noise percussion. The
[sound studio example](examples/sound-studio/README.md) includes a playable project.
