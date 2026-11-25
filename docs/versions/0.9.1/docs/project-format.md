# CartFoundry format 6 — CartFoundry 0.9.1

`.cartfoundry` files are self-contained UTF-8 JSON. Version 1–5 projects
still open; saving upgrades them to version 6. Unknown versions are rejected.
`target` is `gb` or `gbc`; changing it preserves the original colour assets.

## Prompts and textboxes

`prompts` have stable `id`, `title`, `text`, editor `position`, `choices` (text and
target prompt ID), and `textbox: [x, y, width, height]` in 8-pixel tile units.
Coordinates fit within the 20×18 tile screen. Minimum box size is 6×7; a box may
need to be taller to fit its choices and text. The title and body fit width−2;
choice labels fit width−3. The body fits height−6−choiceCount lines. Text is
word-wrapped; export rejects overflow, unsupported characters and broken links.
Printable ASCII and body newlines are supported. Zero choices is an ending.

The preview uses the same ASCII glyph data as the ROM. The shared C flow engine
handles choice selection and transitions. In an overworld, B closes a textbox;
in a screen scene, B returns to the authored screen.

## Raster assets

`assets` contains up to 32 entries with `id`, `name`, `kind` (`sprite`/`tileset`/`screen`),
`width`, `height`, four RGB555 `palette` values, and base64 `pixels` (one index
0–3 per pixel). Imported originals aren't referenced externally. Sprites reserve
index 0 for transparency; tilesets have four opaque colours. Dimensions are
multiples of 8. Sprite frames are 8–32 pixels per side; sprite-sheet images up to 256 per side and tilesets up to 128 per side.

The converter supports fit/crop/stretch, nearest/smooth scaling and optional
background colour removal. Weighted median-cut quantization runs in RGB555.
Game Boy export remaps each palette by brightness into four hardware shades.
GBC export uses the original palette. Image imports preserve these original
colour indices even when the project is temporarily in Game Boy mode.

## Areas and chapters

`world` is the starting area. `areas` contains up to seven additional areas.
Each has an `id`, human-readable `name`, and a `chapter` label for organisation.
The starting world's `enabled` flag selects overworld or conversation-only mode.

Each area defines a `tileset`, `player`, `width` (20–64 tiles), `height` (18–64),
`spawnX`/`spawnY` in tiles, row-major `tiles`, and base64 `collision` (0/1 bytes).
The player's hitbox is an 8×8 foot box. Characters are rendered bottom-centred
on it, using 8×8 hardware sprites. Motion, bounds and camera calculations are
shared C between preview and ROM. Scrolling streams only newly exposed map rows
and columns into the hardware's wrapping 32×32 tile map.

`interactions` entries have `cell` and `prompt`. `exits` entries have `cell` and
`area` (destination ID). Press A while facing or standing on a linked tile.
Exits take priority over prompt hotspots and use the destination's spawn point.
Use separate tiles if both behaviours are needed. Up to 64 links of each type
per area are supported. Collision and interaction layers are independent.

Tileset graphics are deduplicated to at most 128 unique tiles per area. Text uses
its own tile range and GBC palette 7; terrain and the player each use a palette.
ROMs use MBC5. Event bytecode and splash/screen bitmaps are allocated to 16 KiB
banks. Sprite frames also use resource banks (see Sprite resource banking below).
The gameplay interpreter lives in bank 3; its game data and public helper
functions remain resident in bank 0. World simulation/rendering uses bank 1;
larger interface drawing routines use bank 2 alongside optional sound/save code.
The builder checks section endpoints and rejects any fixed-bank overflow. Large
projects can still hit fixed gameplay-data limits; area/actor descriptors,
animation bindings and dialogue metadata remain resident. Map layers, collision,
terrain graphics, screen elements and screen text use resource banks. Optional battery-backed SRAM saves support up to three
journalled slots; see the menu/save authoring guide.

## Testing

The direct area preview needs no build. It uses our shared C movement and flow
code, but does not emulate the CPU. **Build & Play This Area** compiles a copy
with just that area, its reachable prompt graphs and referenced graphics. Its exits are
removed so the test stays isolated. The original project is unchanged.

**Build & Play** compiles all enabled areas and launches the actual ROM in the
embedded mGBA core. The emulator also opens GB, GBC and GBA ROM files. It supports
video, buttons, pause/resume and reset. Audio is drained but muted; save-state,
link cable, peripheral UI and audio output are not exposed yet. Loading a ROM
uses no external application, installed emulator or external BIOS file.

Project saves and successful ROM exports replace their destinations atomically.
Failed validation/compilation leaves any previous ROM intact. The compiler runs
with structured arguments; project text never becomes a shell command.

## Controls and behaviour scripts

`controls` maps `interact`, `close`, `inventory` and `restart` to distinct hardware buttons:
A=16, B=32, Select=64, Start=128. Directional movement remains on the D-pad.
Keyboard keys in the emulator represent hardware buttons: Z=A, X=B, Enter=Start,
Backspace=Select. A physical control remap therefore works in emulation too.

Each area has a `playerScript` and up to four `actors`. Actors have stable `id`,
`name`, a `sprite` asset ID, tile spawn `x`/`y`, and `script` text. The first
behaviour language supports two-word commands, one per line, with `#` comments:

```text
behavior patrol
axis x
range 32
speed 1
every 2
on_touch quest
```

Behaviours are `top_down` (player only), `idle`, `patrol`, or `chase`. `speed` is
1–2 pixels per update; `every` is 1–8 frames per update. Patrol `axis` is x/y and
`range` is 1–255 pixels from its starting point. Chasers use Manhattan `sight`
distance (1–255 pixels). `on_touch` takes a prompt ID or `none`. Contact opens
that conversation with a 60-frame cooldown; B/close returns to the world.

Scripts compile into compact behaviour configurations interpreted by shared C.
Invalid commands, duplicate commands, broken prompt links, solid/out-of-bounds
actor spawns and excess sprite allocation are rejected before ROM export. Player
and actors together may allocate at most 40 8×8 hardware sprites per area. The
hardware's separate 10-sprites-per-scanline limit still applies when they overlap.
Actors collide with map solids; actor-versus-actor blocking and pathfinding are
not implemented. Dialogues freeze movement and hide sprites until closed.

## Sprite sheets and animation clips

Sprite assets optionally add `columns`, `rows` (default 1), and `animations`
(default empty). Atlas dimensions must divide evenly into frames of 8–32 pixels
per side in multiples of 8. The grid supports 1–8 columns and rows, at most 64
frames. Empty animation lists render static frame 0 for backward compatibility.

Each clip has a unique identifier `name`, inclusive `first`/`last` frame indices,
`duration` in nominal 60 Hz ticks (whole range, not per-frame), and boolean `loop`.
There can be 32 clips per sprite. The duration ranges from the number of selected
frames to 3600 ticks (60 seconds). Timing distributes ticks across the range
without dropping frames. One-shots stop and hold their final frame; explicit play
restarts them. Playback logic is shared in runtime/cf_animation.c.

The converter scales source cells separately and quantizes the whole atlas to
one palette. Frame numbering is row-major; generated 2bpp sprite data is
**frame-major**, with each frame's hardware tiles contiguous. Runtime updates
stream changed frames into each character's fixed VRAM allocation. Hardware/OAM
budgeting uses the per-frame size; ROM budgeting includes every sheet frame.

Behaviour scripts may add `animation`, `on_move`, `on_idle`, and `on_interact`
commands with clip names. Missing clip references reject export. Base/idle/move
bindings switch automatically. Interaction-bound one-shots run to completion;
looping actions are interrupted by movement. Animations pause during dialogue.

Old projects and static assets migrate with a 1×1 grid and no named clips.
The loader allows up to 4 MiB of JSON; embedded indexed image pixels are capped
at 512 KiB across the project. Invalid grid and clip edits are not applied.

## Variables, event scripts and libraries (version 3)

`systems` contains arrays `variables`, `items`, `storages`, `scripts`, `npcs`,
`scenes` and `maps`. Library IDs use letters, digits and underscores, starting
with a letter/underscore, at most 32 characters. Keep references valid when
renaming IDs; the editor reports broken references before export.

- Variables: `{id, initial}`; up to 31 signed 16-bit values. Add saturates.
  `result` is reserved/read-only and reports the most recent operation's success.
- Items: `{id, name, description, icon, stackLimit, consumable, useScript}`;
  up to 16, stack limits 1–99. ROM item names use 1–14 printable ASCII characters.
  The initial inventory/container UI is textual; icons/descriptions are library metadata.
- Storage: `{id, name, slots, contents: {itemId: quantity}}`; up to 8, each
  with 1–16 stack slots. The first is `inventory`. Transfers are atomic if the
  destination is full or the source lacks items. Shared storage IDs mean shared contents.
- Scripts: `{id, name, source}`; up to 32 reusable programs, expanded at build time.
- NPCs: `{id, name, actor}`; up to 16 templates, copied when placed. `libraryId`
  records origin; instances are independent, without live inheritance.
- Scenes: `{id, name}` and maps: `{id, name, scene}`; up to 32 each.
  Every area references its owning `mapId`. These nodes are organisational.

Prompts add `onEnter`; choices add `condition` and `actions`. Areas add
`onStart`, `interactionRange` (8–64 px), `attackRange` (8–128 px), and up to eight
`objects`. Objects have `{id,name,kind,storage,onInteract,onHit,x,y,tile,solid}`.
Kinds are `storage` or `script`. Objects overlay their tile and optional collision
onto the base map at build/preview time. An empty container interaction opens its
storage; a nonempty event replaces that default. Actors add `health` (1–255),
`onInteract`, `onHit`, `onDefeat`, and `libraryId`. Defeat hides an actor until area
entry; each attack deals one damage. Rays use shared Bresenham traversal and stop
at the first object/actor/wall; interaction rays also test exits and prompt tiles.

Event commands (one per line, `#` comments):

```text
set coins 5
add coins -1
give inventory key 1
take inventory key 1
move chest inventory key 1
open chest
dialogue quest
call reward
if coins >= 5
  give inventory key 1
else
  add coins 1
end
```

Conditions support `== != < <= > >=`; `has storage item >= quantity` tests
contents. Operands can be signed integers or declared variables (`coins`/`$coins`).
Programs allow 96 expanded instructions, eight nested conditions and four nested
calls; recursive calls and general loops are rejected. Requests to open dialogue
or storage apply after the event finishes. Prompt-entry request chains are bounded.
Item mutations can fail; test `result` before subsequent conditional actions.
Events are sequential, not multi-operation transactions. Consumables are removed
only after the complete use event succeeds.

All state is global to a play session and survives area transitions; it is not
saved to cartridge SRAM. Position reset retains variables/storage in the ROM.
Starting a new preview/emulation session resets them to project initial values.
Area/conversation tests retain the prompt library when events can reference it.
Earlier project versions migrate with empty event fields, default scene/map,
player inventory and the first unused inventory control button.

## Terrain events, live text and modal prompts (version 4)

Areas add `terrainEvents`, at most eight `{tile,every,chance,script}` entries.
`tile` is the original tileset index, `every` is 1–255 matching tile entries and
`chance` is 0–100 percent. Foot-box centre cell changes count as entries; standing
still never rolls. Entries on other terrain do not count. Rules run in order;
the first successful probability roll executes its script and stops that step's
search. Counter state resets on area entry. The generator remaps tile IDs after
deduplicating graphics, so authors always use the original editor tile numbers.
The shared step evaluator and random generator live in `cf_game.c`.

`random variable minimum maximum` accepts inclusive literal bounds 0–32767.
`sub variable value` accepts a literal or variable operand and saturates like add.
`resume` requests a return to the world after the current event completes.
Random state belongs to the play session; reset starts the deterministic sequence
again, which keeps test runs reproducible.

Prompt bodies support `{variable}` (including `{result}`). Compilation resolves
names into compact byte tokens, and shared C formatting expands signed decimal
values and performs word wrapping. Validation reserves space for signed 16-bit
values, reports unknown fields, and rejects body overflow. Titles and choice
labels remain plain ASCII. Both preview and ROM use the same formatter.

Prompts add `modal` (default false) and `portrait` (optional sprite asset ID).
Modal prompts disable Back, Bag and position reset, leaving script/choice actions
to determine the exit. Portraits render frame 0 above the textbox, aligned right
with an 8-pixel margin. The textbox Y must leave room for the portrait plus one
tile. Portraits use background tile IDs 96–111 and GBC palette 6, separate from
font/terrain and actor sprite allocations. Missing assets reject export.

The Pokémon example is entirely project data using these general facilities.
Its C++ template factory supplies the same model fields exposed in the editor;
there is no dedicated battle/catching implementation in the cartridge runtime.

`view storage_id` opens an inspection menu. Selecting an item runs its On use event
without transferring or consuming it, so a collection can show entries through
ordinary dialogue scripts. The menu disables side swapping. `open storage_id`
retains the normal inventory/container transfer behaviour. The Pokémon PC uses
`view party`; no special party UI exists in the engine.

## Styles, transforms and timers (version 5)

Prompts add `textStyle`: border/showTitle/showFooter booleans; padding 0–3;
choiceSpacing 1–3; align 0/1/2 for left/centre/right; one-character cursor and
disabledMarker; optional plain footer and an empty/default or four-colour RGB555
palette. Palette entries 0 and 3 are paper and ink. Defaults preserve prior
layouts. Shared `cf_text_layout` drives both renderers.

Animation clips add reverse/flipH/flipV booleans. First/last remain ascending;
reversal is a playback flag. Pixel data stays frame-major and unchanged.
Behavior source adds `flip_h yes/no`, `flip_v yes/no`, `mirror_facing yes/no`.
Character and clip transforms combine with XOR.

Behavior source also accepts `movement_sprites single|directional` (default
`single`, so existing projects are unchanged). In `directional` mode the
commands `on_move_up|down|left|right` and `on_idle_up|down|left|right` each
name a clip. A direction left as `none` falls back to `on_move`/`on_idle`, then
to `animation`. The direction is the last facing direction, so an idle
character keeps facing the way it last moved. The directional commands are an
error unless `movement_sprites directional` is set. `on_interact` stays a single
clip. To reuse one side-view clip for both sides, set `mirror_facing yes`, point
`on_move_left`/`on_idle_left` at the same clip as the right-facing slot.

`systems` adds rngSeed (0–65535, default 44257) and up to eight timers with
`id,name,source,interval,repeat,autostart,pauseDialogue`. Intervals are 1–65535
nominal 60 Hz ticks. Timer callbacks compile as ordinary bounded events.
`seed value` resets RNG state; `timer start/stop/reset id` controls timers.
Stop disables callbacks; reset restores the interval while stopped. Timer state
persists across areas and resets with the play session. Timer updates that change
visible state redraw the active text/menu page.

Text pages alternate BG maps 0x9800/0x9c00 while the LCD remains on. Cursor moves
write marker cells only. Portrait tiles now alternate 96–111 / 112–127 and GBC
palettes 6/5; text uses palettes 4/7, terrain palette 0. World restoration stages
a fresh hidden map. Do not assume the visible BG map is always 0x9800 in tests.
Identical generated byte arrays are shared across references. Older projects
load with default styles, transforms, no timers and the existing default seed.

## Scene startup and screen elements (format 6)

Root `settings` has `defaultScene` (stable scene ID) and `splashTicks` (1–600,
default 120 video frames). `entry` remains a conversation-preview preference and
has no implicit game-start behavior. A project may have zero prompts.

Each scene has `kind` (`world` or `screen`), `startArea` (owned area ID for world
scenes), `onSceneEnter`, `onBack`, `background` (RGB555) and `elements`. Elements
have stable ID/name, type (`text`, `image`, `panel`, `button`), tile `rect`, `text`,
`image` asset ID, `align` (0/1/2), `condition` and `actions`. A screen supports 16
elements; images share one four-colour palette and up to 128 deduplicated tiles.

Boot: initialize game state → supplied platform splash → Default Scene. World
entry: load Initial Area → onAreaEnter → onSceneEnter. Same-scene area changes
run only the area event. Cross-scene area changes also run the destination scene
event, keeping the selected destination area. Scene re-entry preserves game state.
`scene` and `area` commands end the current event; immediate entry chains are
bounded. Versions 1–5 migrate their legacy start area/dialogue into these fields.

The legacy `world.enabled` flag is retained for old file compatibility; format 6
builds use scene kinds and Default Scene. Screen scenes may retain draft map data
without compiling it as an overworld.

## Sound patterns and four-channel songs

`systems.sounds` is optional (up to 16 entries); older projects retain their
single-channel patterns. Every entry has `id`, `name`, `bpm` (30–300), legacy
`duty`, `volume`, `notes`, optional `effect`, `masterLeft`/`masterRight` (0–7), and
optional `channels`. When present, channels is exactly four entries in hardware
order: Pulse 1, Pulse 2, Wave, Noise. Each lane has 1–64 notes/rests and instrument
settings: enabled/DAC flags, duty, volume, pan bitmask (left=1/right=2), envelope
pace/up, sweep pace/shift/down, hardware length, wave level and 32 four-bit wave
samples, noise clock shift/divider/narrow and drum-kit flag. Inapplicable hardware
controls are stored for safe round trips and hidden by the channel inspector.

Notes store MIDI `note` (36–107 or 0 rest), `ticks` (1–255), optional `volume`
(-1 inherits, 0–15 override), and optional `noise` (-1 inherits kit/instrument,
0–255 raw NR43 override). Wave does not use the velocity field. Old note objects
without overrides still load. Song duration is the maximum lane duration;
shorter lanes wait at the end until the common loop. Effect entries replace only
their enabled channels. IDs obey the normal unique identifier rule, with the
five built-in sound names reserved.

The generator banks note arrays and instrument descriptors (including packed
wave RAM); compact clip descriptors stay fixed. The shared `cf_sound.c` loads
instrument data safely across ROM banks, sequences all four lanes, and handles
looping/gating/modulation state. End-user playback does not load external media
or tools. Preview synthesis and actual mGBA audio are separate output backends.

### Sprite resource banking

ROM sprite frames live in resource banks, separate from the main renderer and
world code. Identical frame bytes are shared across player/actor sheets and areas;
frame references preserve authored frame numbers. Each reference stores a bank
and address. Only the current frame is copied through a bounded 256-byte buffer
into sprite VRAM. Sheet graphics count toward total ROM capacity; resident frame
references count toward the fixed gameplay-data budget. Per-frame OAM limits are
unchanged. Tilesets emit only tiles used by the effective map or terrain rules,
with identical tile patterns shared within each area.

Interface drawing and optional sound/save code share bank 2; event/game code uses bank 3. Resource banks
start at bank 4. This leaves more room for event and keyboard runtime code without
putting graphics back into the runtime code bank. Fixed metadata and main-code
bank limits still apply.

### Lossless resources and feature selection

Maps, terrain graphics, sprite frames, portraits, platform splashes, screen
bitmaps and screen text use immutable `CFResource` descriptors. A resource is
stored raw or as independent 64-byte PackBits blocks, whichever uses fewer bytes
including the block-offset directory. Each packet encodes either a repeated byte
or a literal sequence. Random access decodes at most one block into a shared
64-byte cache; the ROM never allocates a full decompressed map. Collision cells
are first packed into bits (eight cells per byte), then use the same resource
codec. Movement, actor behaviour and raycasting still run their shared C logic;
only the collision accessor differs between preview byte arrays and ROM resources.

Screen/menu element records live in resource banks and are copied one record at
a time. Encoded screen text has a 511-byte source limit and a bounded decode
buffer. Cursor movement still updates only marker cells in the active BG map.
Cross-bank element actions declare their event symbols explicitly. Resource
reads and decoding execute in resident code and restore the caller's bank before
returning; interface drawing uses banked calls to bank 2.

The ROM builder omits sound code and voice state when there are neither sound assets nor
compiled sound commands (built-in sounds count as sound use),
omits save serialization when cartridge saves are disabled, and omits keyboard
input/render code when no keyboard layouts are declared. Projects with no world
scenes also omit movement, actor, animation and raycast code. Native previews retain
all features. Build logs report stored resource bytes and included/omitted runtime
features. These optimisations preserve project files, artwork and the explicit
save-record format; they do not relax per-frame OAM or VRAM limits.

Animation lists (`.cfanimations`) are separate reusable files with
`format: "CartFoundry animations"`, `version: 1`, `columns`, `rows`, `frameWidth`,
`frameHeight`, and the asset-format `animations` array. They contain no image or
palette data. All four geometry values must match the destination sprite sheet.
