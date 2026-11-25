# Authoring controls and engine coverage — CartFoundry 0.9.0

CartFoundry exposes the current gameplay commands through editor forms as
well as source text. New primitives should gain a model field, editor control,
shared preview/runtime behavior, validation and meaningful verification together.

## Current coverage

| Function | Editor controls |
|---|---|
| Game startup / target / splash duration | File → Project Settings → Default Scene, Target, Platform splash duration |
| Scene entry and scene changes | Scenes & screen builder / hierarchy scene inspector → onSceneEnter; + Event → Scenes / Load scene |
| Menu screens | Screen builder: text, panel, image and button elements; drag/resize, alignment, order and enabled conditions |
| Scene testing | Screen builder Preview (shared native runtime) and Test Scene (real ROM); Build & Play uses Default Scene |
| Script workspace / references | Scripts tab: element-type list, event selector, live edits, double-click reference browser; live read-only JSON tab |
| Explicit area loads / object and NPC event calls | + Event → Areas / Load area, Objects / Call event, NPCs / Call event |
| Project, scene, map, area and object ownership | Project & game data tree; parent selectors; Open map editor |
| Direct sprite import and placement | Map toolbar Import & place sprite; import automatically selects the brush; double-click existing sprite assets to place |
| Direct object / sprite selection | Map Select / move tool; click visible sprites or object tiles; instance list; drag with one-step Undo |
| Reuse existing content | Place / reuse palette: sprite assets, NPC library and all existing instances; search and stamp; Duplicate / Ctrl-Cmd+D |
| Name and target map instances | Selected / instances inspector: display name, stable target ID, reference field; renames update direct event calls |
| Create maps from templates | + New map / + Area dialogs: parent scene/map, empty or existing terrain, optional contents, resources and dimensions |
| Container setup on the map | New container brush creates separate storage; copied containers retain references; Create separate storage button |
| Areas, exits and spawn points | World tab area selector, size/spawn/link painting and isolated area test |
| Tile and collision painting | Pencil, rectangle fill and flood fill; right-drag erases; Alt-click picks a tile |
| Zoom and navigation | Map scale / Ctrl-Cmd wheel, sheet Fit/percent/scrollbars and thumbnail scale |
| Indexed graphics and palettes | Pixel art canvas, blank assets, four brush colours, palette edit, PNG import/export |
| Sprite sheets and animation ranges | Animation grid, inclusive range, duration, loop and playback preview |
| Reuse animation graphics | Reverse playback, Mirror horizontally/vertically, Duplicate clip (shared frames) |
| Character behavior | Controls & behaviours → Behavior & animation fields; equivalent fields on NPC/actor scripts |
| NPC and player visuals | Sprite selector, clip bindings, horizontal/vertical flips and automatic left-facing mirroring |
| NPC combat and interaction | Health, position, behavior, On interact/On hit/On defeat in the hierarchy inspector |
| Objects and storage | Object kind/graphic/solid/position/storage, interaction/hit script forms |
| Raycasts | Area interaction/combat distance controls; map solids block the rays |
| Variables | Variable library with ID and initial value; + Event forms for set/add/subtract |
| Random numbers | Project root Initial RNG seed; + Event → RNG random range / Set seed |
| Item counts and transfers | Item/storage libraries, contents/capacity fields; Give/Take/Transfer event forms |
| Inventory and collection views | Open / Inspect storage events; items may run On use/inspection scripts |
| Branching and calls | + If forms, comparisons, Else/End and Call event forms; Check script diagnostics |
| Choice gating | Condition… form: always, variable comparison or item count |
| Dialogue actions and entry events | + Event / + If / Check script on prompt, choice, actor, object and terrain events |
| Dialogue appearance | Box bounds, border/title/footer, padding, alignment, choice spacing/markers, custom footer, paper/ink colours |
| Dialogue portraits and interaction policy | Portrait sprite selector and Modal dialogue checkbox |
| Dynamic dialogue values | Body `{variable}` fields; shared formatting/wrapping and overflow validation |
| Timed actions | Timer library: seconds/ticks, one-shot/repeat, session autostart, dialogue pause, timeout event |
| Starting/stopping timers | + Event → Timing → start, stop or reset a named timer |
| Terrain triggers | Area terrain events: tile, matching-entry interval, probability and event script |
| Return to play | + Event → Dialogue → Return to world |
| Build and testing | Target selector, previews, area/conversation builds and embedded ROM emulator |

All current public event commands have insertion forms. The source remains
editable for ordering, comments, branches and repeated operations. Internal VM
jumps, renderer buffers and scheduler counters are implementation details, not
a second project-authoring API.

## Event editing

Choose **+ Event…**, select a category/action, then choose current project IDs and
values. Operands that accept variables have editable value selectors. **+ If…**
inserts a validated if/end block and puts the cursor inside it. Use Condition… for
a choice's enabled condition. **Check script** reports errors and expanded
instruction usage. These controls appear beside scripts throughout the editor.

Names used in scripts are stable IDs, not display names. Create an entry before
referencing it. The editor reports deleted or renamed references before export;
it does not silently guess which entry was intended.

The behavior form writes the same behavior commands as source editing. It
restricts the player to the current top-down behavior, and NPCs to idle, patrol
or chase. Animation choices must exist on the assigned sprite. Flip properties
combine with clip flips using XOR, so two horizontal flips cancel.

## Animation reuse

A clip still references an inclusive ascending first/last range. Reverse changes
the playback order, including the start/end held by a one-shot. It does not
rearrange or copy sheet pixels. Clip mirroring and character mirroring use Game
Boy hardware sprite flips. Multi-tile frames also reverse tile positions, so
16×16 and 32×32 characters mirror as a complete image. Facing mirroring flips a
right-facing base when horizontal facing is left.

Directional characters: in Behavior & animation fields, set Movement sprites to
Directional to bind separate moving and idle clips to up, down, left and right.
Single mode (the default) uses On moving / On idle as before. These fields write
the `movement_sprites` and `on_move_<dir>` / `on_idle_<dir>` commands.

Duplicate a clip to make a left-facing or reversed variant of the same frame
range. The generator also shares identical image/map/collision byte arrays across
references rather than emitting another copy for each actor/area.

## Text rendering and styles

Cursor movement updates choice markers only. A full dialogue/menu page is built
in the hidden background map and shown at a vertical blank. Portrait graphics
and palettes alternate too. Returning to the world regenerates its hidden map
before switching back. The LCD stays enabled throughout these interactions.

Text style controls are per prompt. Defaults retain existing layouts. Body
alignment is left, centre or right. Padding and choice spacing are tile units;
bordered boxes need at least one tile of padding. Paper and ink are GBC colours;
GB mode keeps its readable monochrome shades. The custom footer is plain text;
leave it empty for the current control hints. Body variables expand before word
wrapping. Text overflow is validated using signed 16-bit values.

These settings do not add alternate fonts, typewriter reveal, text pagination,
rich text or proportional glyphs. The current renderer uses fixed 8×8 ASCII
characters. Those remain distinct future engine features rather than controls
that pretend to do something the runtime cannot execute.

## Timing and randomness

Timers run at nominal 60 Hz using video-frame ticks in the ROM. They do not busy
wait or freeze character movement. The editor uses its elapsed-time scheduler.
Intervals are 1–65535 ticks; the inspector shows both seconds and ticks. Start
schedules a full interval; stop disables execution; reset restores the interval
and leaves it stopped. Repeating timers reschedule before their callback, so a
callback can stop/restart itself. Timers are global to a session and survive
area changes. Their pause checkbox controls dialogue and inventory menus. For a
conversation-only game, turn pause off if it should continue while text is open.

Timer callbacks that change variables or storage refresh the visible dialogue
or menu without requiring an extra Show dialogue command. If multiple callbacks
request a new screen in the same update, the last request wins. Preview sessions
pause when their viewport/window is inactive. Restart play to apply edited
initial values and the project's initial RNG seed.

The RNG produces inclusive random ranges. An initial seed or Set seed event
makes a run reproducible. Seed literals are 0–65535; zero selects the default
nonzero seed. Variable seeds use the low 16 bits. Random-result variables remain
signed 16-bit, so random ranges are bounded to 0–32767. Probability tables and
weighted pools can currently be authored with random rolls and If branches;
there is no separate weighted-table editor yet.

## Practical limits

Exports use MBC5 with banked event bytecode and splash/screen graphics. There
are no SRAM saves yet. Fixed runtime/gameplay-data bank limits are checked at
export; graphics sharing and code-size optimization save space. There are up to eight areas, four NPC instances per
area, eight objects per area, eight timers, 31 user variables, 16 item types and
eight storages. Event programs allow 96 expanded instructions, eight nested
conditions and four call levels. The sprite hardware limits still apply.

The built-in movement models are top-down, patrol, chase and idle. Platformer
physics, arbitrary entity components, navigation/pathfinding, audio output,
general user-defined functions and asynchronous multi-thread script execution
are not implemented by this release. Timers provide delayed/repeating actions,
not a resumable Wait inside a script. These are engine primitives to add and
expose deliberately as broader genre support develops.

## Design references

- [Godot AnimatedSprite2D](https://docs.godotengine.org/en/stable/classes/class_animatedsprite2d.html): adjacent flip properties and forward/backward playback controls informed the animation inspector.
- [GB Studio scripting](https://www.gbstudio.dev/docs/scripting/): choosing categorized events and editing their parameters informed the insertion forms.
- [GB Studio timer events](https://www.gbstudio.dev/docs/scripting/script-glossary/timer/): named timer actions and seconds/frame controls informed the timer inspector.
- [GameMaker sequences](https://manual.gamemaker.io/monthly/en/The_Asset_Editors/Sequences.htm): explicit timed events and playback controls informed the timing audit; a timeline editor is not included here.

These are interface references. CartFoundry continues to use its own Qt editor,
project format, compiler pipeline and shared C runtime.

## Scenes and startup (0.7)

See [Scenes & Screens](../examples/scenes/README.md) for the complete editor recipe
and entry semantics. The conversation preview start no longer controls game boot.
The GB/GBC splash uses the artwork shipped in `images/splash_2_gb.png` and
`images/splash_2_gbc.png`, quantized to the target's four-colour display. Default
splash duration is 120 video frames (about two seconds).

Public additions: `scene scene_id`, `area area_id`,
`object area_id object_id interact|hit`, and
`npc area_id actor_id interact|hit|defeat`. Object/NPC calls execute the selected
actor's authored event; they do not deal damage, change animation or mutate health.
They are expanded like reusable script calls and share the recursion/instruction
limits. Scene/area loads end the current event. The compiler moves event bytecode
and screen graphics to MBC5 banks; fixed runtime/data sections are checked for
overflow. Save format 6 adds project startup settings and authored scene contents.

## Map authoring (0.8)

The full recipe is in [Building maps from existing objects](map-authoring.md),
also available through Help → Map building & reusable objects. Existing project
format 6 files require no migration. The runtime and cartridge limits are unchanged.

### Sound events

Use **+ Event → Sound** in any event script (including named scripts, timers,
terrain steps, dialogue choices and NPC/object events). Built-in Game Boy pulse
sounds are `blip`, `hit`, `chime`, `pulse` and `alarm`.

```text
sound loop pulse
sound play chime
# Later, when leaving or finishing:
sound stop
```

Play retriggers the one-shot channel; loop replaces the independent repeating
channel. Stop silences both. Loops persist across area/scene changes until stopped
or the session resets. The shared runtime advances sound at game tick speed,
including during dialogue. Preview uses square-wave synthesis; Build & Play uses
the actual emulated Game Boy audio hardware. The Sound & music tab also authors
custom patterns. Qt Multimedia
provides desktop output; Windows audio has not yet been verified.

| Function | Insertion form |
| --- | --- |
| One-shot sound | + Event → Sound / Play one-shot |
| Repeating sound | + Event → Sound / Start loop |
| Silence | + Event → Sound / Stop all |

### Sound & music editor

Open **Sound & music** and choose **+ Song**, **+ Effect**, or **+ Demo song**.
Songs have four simultaneous lanes: Pulse 1, Pulse 2, Wave and Noise. Select a
channel tab to edit its piano roll and instrument. Channel enabled is saved in
the project; Solo is an audition-only aid. Play and Loop start all enabled lanes
together. The longest lane sets the song length; shorter lanes rest until the
common loop boundary, so channels cannot drift independently.

Click the piano roll to draw eighth notes (C2–B7), drag to move, drag the right
edge to resize, and right-click/Delete to erase. Gaps become rests. Overlap
replaces that portion of the same lane. Other lanes remain independent. Each
lane holds up to 64 note/rest entries, each lasting 1–255 ticks at 60 Hz. Tempo
(30–300 BPM) scales every lane together. Sixteen song/effect library entries are
available per project. Selected-note controls edit exact duration, pulse/noise
velocity and an optional raw noise-byte override. Wave output uses its hardware
level instead of velocity. Moving/resizing a note preserves its overrides.

| Channel | Instrument controls |
| --- | --- |
| Pulse 1 | Duty (12.5/25/50/75%), initial volume, envelope pace/direction, period sweep pace/direction/shift |
| Pulse 2 | Duty, initial volume, envelope pace/direction |
| Wave | 32 editable 4-bit samples; triangle/sine/square/saw presets; mute/full/half/quarter output |
| Noise | Envelope; clock shift/divider; 15-bit/7-bit noise; drum kit by note or fixed noise settings |
| Every channel | Enabled, DAC, left/right/center/muted stereo routing, optional hardware length timer |
| Song output | Independent 0–7 left/right master gain (0 is quiet, not mute) |

Envelope pace is in 64 Hz units, period sweep in 128 Hz units. Pace 0 disables
iterations. The optional hardware length is in 1/256-second units: up to 64 for
pulse/noise and 256 for wave. Zero leaves gating to piano-roll note duration.
The DAC and waveform/sample controls map to Game Boy sound hardware. External
VIN input is not used. The shared C sequencer runs the song in preview and ROM;
the preview synthesizer approximates hardware sound, while Build & Play uses
the actual emulated APU, including envelope/sweep timing and hardware quirks.
Hardware register reference: [Pan Docs](https://gbdev.io/pandocs/Audio_Registers.html).

Noise kit pitches: C2 kick, D2 snare, F#2 closed hat, A2 tom, A#2 open hat and
C#3 crash. Other pitches use the track's noise settings. Disable Drum kit by
pitch to use fixed settings for every note, or set a selected note's Noise byte.

Use `sound loop song_id`, `sound play effect_id`, and `sound stop` in events.
Starting a song replaces current playback. Effects only replace their enabled
hardware channels; the default effect uses Pulse 2. An interrupted song channel
returns at the next song-loop boundary. The original five built-ins remain
available and use the original single-channel script behaviour.

**+ Demo song** loads *Foundry After Hours*, an eight-bar, 150 BPM composition
using all four channels. The packaged `examples/sound-studio` project starts it
on scene entry. It combines a pulse lead, a second pulse arpeggio, triangle-wave
bass, and kick/snare/hat percussion, with stereo separation and note velocities.

| Function | Editor control |
| --- | --- |
| Compose all channels | Sound & music → + Song → channel tabs |
| Make an effect | + Effect (Pulse 2 by default) |
| Load the composed example | + Demo song |
| Edit pitch/duration | Piano roll → drag / resize right edge |
| Edit a channel instrument | Channel tab → instrument inspector |
| Hear one channel | Solo this channel → Play / Loop |
| Trigger in game | + Event → Sound → select the library entry |

Image import defaults to **Keep original pixels (pad only)** and the source size,
rounded up to an 8-pixel boundary. It never resamples in this mode; any padding is
added at the right and bottom. Choose a scale mode explicitly to resize or crop.
For a sprite sheet, choose the frame grid; each frame keeps its source pixels.
Current frame limits remain 32×32 for sprites and 128×128 for tilesets. A source
larger than those limits needs a sheet grid or an explicit resize before import.

Colour counts use colours actually present in visible pixels at GBC RGB555
precision, ignoring unused indexed-palette entries and transparent sprite pixels
(alpha below 128). Existing colours are preserved when they fit the shared asset
palette: three visible colours plus transparency for sprites, four for tilesets.
Images exceeding that budget show **Reduce visible colours** with a live preview;
click the palette swatches to tune the reduction or reset to the automatic palette.
GBC colour precision and binary sprite transparency still apply. Background removal
and resampling remain explicit choices. Screen-image insertion uses its existing
160×144 screen conversion.

Remove a sprite asset with **Sprite animations → Remove sprite sheet…**, or select
an asset under **Sprites, tilesets / overworld → Remove selected asset…**. This
removes the entire asset and its animation clips; **Delete animation** only removes
one clip. Removal supports **Edit → Undo project change**. Assets still assigned
to players, characters, NPC library entries, portraits, item icons, tilesets, or
scene images cannot be removed until those uses are changed or removed; the dialog
lists the remaining uses. To delete only a placed character, select it on the map
and use **Selected / instances → Delete** or the Delete key.

## Menus, keyboards and saves

**Scenes & menu builder → + Menu preset…** installs editable screen scenes for
pause menus and save/load slots. The pause preset also creates a naming keyboard,
`player_name` and `save_name` text fields, and save event scripts. It assigns the
Start button to the pause menu. Existing scenes with the preset IDs are retained;
customize their panels, text, buttons, positions, colors and events in the same
scene inspector. Use **Text, keyboard & save builders…** to manage input and saves.
**File → New Menus & Saves Example** demonstrates the complete flow; the project is
also in `examples/menu-saves`.

| UI / function | Event command / behavior |
| --- | --- |
| + Event → Menus / Open menu | `menu scene_id` opens a screen and pauses the calling event |
| + Event → Menus / Return | `return` restores the calling menu/world/dialogue and continues its event |
| + Event → Input / Keyboard | `keyboard layout_id text_field maximum_chars` pauses until OK or Cancel |
| + Event → Saves / Write slot | `save write 1` writes a slot; `result` is 1 on success, 0 on failure |
| + Event → Saves / Load slot | `save read 1` restores selected data; with position enabled, resumes the saved scene/area |
| + Event → Saves / Check slot | `save check 1` sets `result` to whether a valid compatible slot exists |
| + Event → Saves / Erase slot | `save erase 1` invalidates both recovery copies of that slot |
| + Event → Text / Clear field | `text clear player_name` clears a text field |
| + Event → Text / Copy field | `text copy save_name player_name` copies, bounded by the destination capacity |
| Text fields tab | Declare up to eight fields, each with an initial value and a capacity of 1–20 characters |
| Keyboard builder tab | Name/digits presets, editable character order, title, columns and grid origin, with a live preview |
| Save builder tab | Automatically detects variables, text fields and storages; check fields to include, and choose timers, RNG and player position |

Text fields use `{field_id}` in dialogue bodies and scene text, just like numeric
variables. A keyboard returns its text into the chosen field only when accepted;
cancelling preserves the previous value and returns `result = 0`. The event
continues after the keyboard, so it can test `result`, copy input or open another
menu. A dialogue choice waits for input before following its target. Keyboards
can also be called from a prompt's onEnter event. Use printable ASCII characters
from the built-in font. Each layout supports up to 80 unique characters, arranged
in a grid of up to ten columns; `<` deletes, `!` accepts and `X` cancels. Space is
a supported character key. Arrows move the selection; A activates a key and B
cancels. Editor previews also accept Escape to cancel and Backspace to delete.
Menu calls nest up to four levels; input/events use the same bounded wait stack.
Timers and world movement pause while these modal interfaces are open.

The save builder includes newly added fields automatically unless excluded. Its
optional generated scripts are `save_write_1`, `save_read_1`, `save_check_1` and
`save_erase_1` (and corresponding scripts for other slots). It adds missing scripts
without overwriting authored scripts. `{save_status}` displays messages such as
`Saved`, `Loaded`, `Save failed` or `No valid save`. `result` and `save_status` are
reserved names. Save slots are 1-based in scripts. Save names are ordinary saved
text fields: they label the game data and do not rename the host emulator's file.

ROM saves use battery-backed MBC5 RAM (8 KiB) and up to three slots. Each slot has
two 768-byte journal records, with a checksum and a last-written commit marker;
a damaged or incomplete newest copy falls back to the previous valid copy. The
format is explicit little-endian bytes, independent of C struct layout. A project
identity and schema fingerprint prevent loading data from unrelated or incompatible
layouts. Changing field IDs, capacities, order, selections or scene/area structure
can invalidate previous saves; no automatic migration is attempted.

Position saving includes scene, area, player coordinates and facing. Loading a
position does not run scene/area entry events again. Variables, chosen text fields,
chosen storages, timers and RNG state are saved according to the builder selections.
Actor motion/health, sound playback and open menus are transient. Represent lasting
story outcomes in variables. ROM emulators retain battery RAM in a `.sav` file;
the embedded mGBA wrapper attaches a `.sav` alongside battery-backed ROMs. Keep it
alongside the ROM between launches. The editor's native preview uses a separate
file per save identity and schema under its application-data `preview-saves`
directory; the builder shows the path. Isolated area tests therefore use separate
preview saves from the complete project. Preview and ROM saves are independent.

## Import frame detection

Sprite import guesses an initial sheet grid without resampling the image. A
filename size hint such as `hero_16x16.png` takes priority; a grid hint such as
`hero_4x2.png` is also supported. Otherwise the dimensions are used to choose
8-pixel frame sizes and an evenly dividing grid, preferring common 16×16 frames.
Small images without a hint retain a static 1×1 grid. **Guess frames again** resets
the controls to that guess. Frame size and row/column counts remain editable.
Current sprite sheets support up to 8×8 frames, with 8–32 pixel frame dimensions.
If no valid conversion is available, the full source remains visible in the
preview while import stays disabled until the dimensions/grid are corrected.

**Pixel art → Import sprite / sheet…** and **Import tileset…** use the same importer.
Newly imported pixel-art assets are selected automatically and large sheets start
at a zoom that makes them visible. Images that exceed the existing asset limits
need a supported grid or an explicitly selected resize mode; guessing does not
silently resize or crop them.

## Button press events

In an area's **On start** event (or a world scene's enter event), register a button
handler directly. For example, with a screen scene whose ID is `pause`:

```text
on_press start
  menu pause
end
```

Use **Insert event → Input / On button press** to insert the block. Buttons are
`a`, `b`, `start`, `select`, `up`, `down`, `left`, and `right`. Put normal event
commands, including `if`/`else` conditions and `call`, inside the block. Registration
does not execute its body. Each handler fires once on a new press during world
play; holding a button does not repeat it. Its button consumes the normal action,
including the default Start pause binding. Handlers pause during dialogue,
menus, and keyboard entry. Opening a menu suspends the handler until return.

One handler per button remains registered until replaced or the game resets.
Re-register it in another area's start event to change its behaviour. Handler
registrations are not saved in save slots; register them during scene/area setup.
Nested button handlers are rejected. The editor preview and ROM use the same
registration and dispatch code. Preview keys: Z=A, X=B, Enter=Start,
Backspace=Select, arrows/WASD=directions.

## Reusable animation lists and About

**Sprite animations → Export animation list…** writes a `.cfanimations` JSON file
containing grid/frame dimensions and named clip ranges, duration, loop, reverse,
and mirror flags. **Import animation list…** applies it to a selected sheet with
identical grid and frame dimensions; pixels, colours, and asset identity are
preserved. Replacing a list asks for confirmation and supports normal project
undo. Import rejects incompatible or invalid lists without changing the sheet.

**Help → About CartFoundry…** (in the **CartFoundry** application menu on macOS) shows the application version and a clickable
[EmeraldSoftlab website](https://eslab.click).

## Searchable documentation book

**Help → Documentation book…** (F1) provides offline examples and the existing
reference chapters. Search matches all entered words across topic titles and
contents; Ctrl/Cmd+F focuses search. Matching words are highlighted, and Previous
match / Next match step through the first word within the current topic. Back
and Forward revisit topics. The handbook covers menu versus scene, nested
returns, button events, saving/loading, animation lists and ROM troubleshooting.
