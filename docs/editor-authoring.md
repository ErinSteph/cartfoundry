# Authoring controls and engine coverage

CartFoundry 0.8 exposes the current gameplay commands through editor forms as
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
The GB/GBC splash uses the artwork shipped in `images/splash_gb.png` and
`images/splash_gbc.png`, quantized to the target's four-colour display. Default
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
