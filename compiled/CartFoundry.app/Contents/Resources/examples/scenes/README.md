# Scenes & Screens

Choose **File → New Scenes & Screens Example**. Everything in this example is
editable by hand; the file uses the same scene, element and event fields as the UI.

1. Open **File → Project Settings**. Default Scene is `scene_title`; the platform
   splash lasts 2 seconds. Its supplied GB/GBC artwork comes from `images/` in the
   engine package.
2. Open **Scenes & screen builder** and select Title menu. Click empty canvas or
   deselect an element to inspect the scene. It is a Screen scene. Its entry script
   is empty, so no conversation starts automatically.
3. The title screen contains a panel, centred title text and two buttons. Select
   Play: its On activate script is `scene scene_main`. Select About: its action
   loads `scene_about`. Drag elements to move them; drag the bottom-right handle
   to resize. Numeric fields use the Game Boy's 20×18 tile grid (8×8 pixels/tile).
4. Select About screen. Its `onBack` script is `scene scene_title`, so B returns to
   the title. Text can include `{variable}` values. Image elements import a PNG,
   JPEG, BMP or GIF and scale it to their rectangle using four indexed colours.
5. Select Main Scene. It is a World scene; its Initial Area is Starting Area.
   Its `onSceneEnter` script is `dialogue welcome`. That explicit command starts
   the workshop conversation after the area loads. The conversation's third
   choice returns to the title using a scene event.
6. In **Scripts**, choose Scenes from Element type, select Main Scene, then
   `onSceneEnter`. Delete the dialogue command to start directly in the world.
   Choose Dialogues in the reference browser and double-click THE WORKSHOP to
   insert the same correctly referenced command again. Edits update the project
   immediately and use project Undo. The second tab shows live project JSON.
7. **Preview** runs the selected scene immediately using the shared game logic.
   **Test Scene** builds a ROM starting at that scene; **Build & Play** starts at
   the project's Default Scene. These tests use unsaved edits too.

Screen controls: Up/Down selects an enabled button, A activates it, B runs onBack.
World controls: arrows move, A interacts, B attacks/closes dialogue, Select opens
inventory and Start resets position. In the embedded emulator Z=A and X=B.

## Entry semantics

Boot initializes variables, storage, RNG and timers once, displays the platform
splash, then loads Default Scene. Loading a world scene loads its Initial Area,
runs that area's `onAreaEnter`, then runs `onSceneEnter`. Re-entering the scene runs
its entry script again without resetting game variables or inventory. A same-scene
area change only runs `onAreaEnter`; crossing to a different scene also runs the
destination's `onSceneEnter`. A `scene` or `area` command transfers control and
ends the current event. Immediate entry chains are capped at eight loads per
request pass, so a mistaken entry loop cannot hang the game.

The conversation preview start is only for the Conversations tab. It never selects
the game's starting scene or implicitly starts a dialogue in a ROM. Version 1–5
projects migrate their previous startup into explicit scene entry fields on load.

## Current screen limits

Up to 32 scenes and 16 elements per scene; text, panels, images and buttons use a
20×18 grid. Text uses the existing ASCII font and four default text colours.
Imported images share one four-colour background palette per screen and must fit
128 unique 8×8 tiles after deduplication. The editor reports overflow before build.
Buttons can be disabled by a variable/item condition, and run any public event
command. Element order determines drawing and button-navigation order. Image
layers form the background; text, panels and buttons overlay it. These are menu
screens, not a general layout or animated HUD system.

ROM exports use MBC5 with banked event bytecode and splash/screen images. Runtime
code, gameplay tables and maps still have fixed-bank limits checked at export.
The Windows code path is retained but this release is verified on macOS only.
