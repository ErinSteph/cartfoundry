# Handbook — CartFoundry 0.9.0

Use the edition matching the version shown in About. Older builds may lack
some commands or controls described by newer editions.

This book combines practical recipes with the full authoring, map-building,
project-format and update references. Choose a topic on the left, or search for
words such as **pause menu**, **on_press**, **save slot**, or **animation list**.
Search checks the whole book, including example scripts. Multiple words narrow
the results. Matching words are highlighted; Previous match and Next match step
through the first search word in the displayed topic. Ctrl/Cmd+F focuses search.
Back and Forward revisit topics. Clear search to browse every chapter.

Scripts use IDs, not display names. Replace the example IDs with entries that
exist in your project. The event insertion forms offer your project's current IDs.

## Getting started: scenes, areas and events

A project contains scenes. A **world scene** owns maps and areas for navigation;
a **screen scene** contains panels, text, images and buttons for menus or title
screens. File → Project Settings → Default Scene chooses where a full game starts
after its splash. The scene inspector's onSceneEnter event runs when it opens.
For a world scene, its initial area's start event runs before onSceneEnter.

Use **Scenes & menu builder** to create scenes and menu presets. Use **Sprites,
tilesets & overworld** to edit areas, collision and characters. **Scripts** brings
editable event sources and references together. Event editors have **+ Event…**,
**+ If…** and **Check script** controls. These insert the same commands you can type.

A quick starting point is File → New Menus & Saves Example. To learn the world
editor, use File → New Overworld Example or New Pokemon Example, and browse the
Pokémon walkthrough chapter in this book.

## Pause menu: open a scene and return to gameplay

Create a screen with **+ Menu preset… → Pause menu**. The generated pause screen
normally has ID `pause_menu`; its Save and Load buttons open their own menus.
The preset also sets up the project's normal Start pause binding.

To open that screen from an event, use:

```text
menu pause_menu
```

This remembers the calling scene and pauses the calling script. A **Resume**
button should contain:

```text
return
```

Put `return` in the pause scene's **onBack** event as well, so B closes it.
Return restores the caller, including the player's position, rather than always
jumping to the project's Default Scene. To explicitly go to a particular scene,
use `scene its_scene_id` instead.

For a custom Start binding, register this in the area's start event or the world
scene's onSceneEnter event:

```text
on_press start
  menu pause_menu
end
```

Choose **+ Event… → Input / On button press** to insert the handler, then add
**Menus / Open menu** inside it. Handlers run during world gameplay; dialogue,
menus and keyboard entry suspend them.

## Why return does nothing: menu versus scene

These two commands have different purposes:

| Command | Behaviour | How to leave |
| --- | --- | --- |
| `menu pause_menu` | Opens a screen while remembering its caller | `return` restores that caller |
| `scene title_screen` | Changes to another scene | Use another explicit `scene` command |

If you enter a pause screen with `scene pause_menu`, there is no menu caller for
`return` to restore. Likewise, testing the pause screen directly starts there;
it does not create a world underneath it. Test the world scene or use Build & Play,
then open the pause screen through `menu` or the configured Start binding.

If a menu closes and immediately changes scene again, inspect the event that
opened it: execution resumes after its `menu` command when it returns.

## Nested menus: pause, save and back

A screen can open another screen as a menu. In the pause screen's Save button:

```text
menu save_menu
```

In the save screen's Back button and onBack event:

```text
return
```

The route is world → pause → save. One return goes back to pause; another goes
back to the world. Up to four menu levels can be open. World movement is paused
while menus are open. Use menu calls throughout this route to retain the callers.
A regular scene change starts scene navigation instead of preserving this route.

## Button-press scripts with conditions

Handlers register when their containing event runs. Their bodies execute on a
new press, not while the button is merely held. Supported button names are `a`,
`b`, `start`, `select`, `up`, `down`, `left` and `right`.

For example, in the Overworld Example, `coins` is a declared variable. To allow a
custom pause menu only when it has a particular value:

```text
on_press start
  if coins >= 0
    menu pause_menu
  end
end
```

Create the pause menu first. In your own game, declare any variables you reference
in Project & game data, or choose an existing variable with the condition form.
You can also place normal `call`, item, dialogue and other event commands inside
a handler. One handler per button remains registered until replaced or the game
resets. A custom handler consumes that button's normal action, even when a
condition in its body is false. Handlers cannot be nested inside other handlers.

Preview controls: Z=A, X=B, Enter=Start, Backspace=Select, arrows/WASD=directions.
The ROM uses Game Boy buttons; an emulator maps its keyboard controls separately.

## Save and load: slot actions and successful return

Use **Text, keyboard & save builders…** to enable saves and choose saved fields.
Menu presets generate screen scenes and editable save/load scripts for you.
Slots are numbered from 1; the slot must exist in your save configuration.

A button action can save to Slot 1 and close the current menu only on success:

```text
save write 1
if result == 1
  return
end
```

Other slot commands are:

```text
save check 1
```

```text
save read 1
```

```text
save erase 1
```

Check tests whether the slot is valid. Read restores selected saved fields;
erase removes that slot's saved record. `result` is read-only and reports the
operation's success. A position-restoring load can restore gameplay directly;
it is not simply another menu return. The generated presets handle the normal
save/load route. Enable position saving if you want a load to restore the scene,
area, player position and facing.

Native preview saves and emulator/ROM saves are separate. For an accurate ROM
check, use Build & Play and test the cartridge, including saving and reloading.

## Continue a script after a menu closes

Opening a menu suspends its caller. Returning resumes at the next command:

```text
menu pause_menu
add coins 1
```

With a declared `coins` variable, this adds one coin after the menu returns,
not as soon as it opens. This is useful for post-menu checks, but leave the
second line out if opening a pause menu should have no other effect.

Use `return` for menu callers. `resume` is a dialogue command for returning to
the world; it is not the menu-stack return command.

## Dialogue, variables and reusable event scripts

Create variables, items, storage and named scripts in **Project & game data**.
Event commands refer to their IDs. A dialogue command also uses a prompt's ID:

```text
dialogue intro
```

For a declared `coins` variable, normal branching looks like:

```text
if coins >= 5
  sub coins 5
else
  add coins 1
end
```

To reuse an event, create a named script and insert **Scripts / Call** into the
calling event. For example, if its ID is `reward`:

```text
call reward
```

Live dialogue text can contain `{coins}`. Text fields use the same brace syntax,
and `{save_status}` shows save feedback. **Check script** catches unknown IDs,
invalid commands and unmatched blocks before a ROM build.

## Reuse animation lists on matching sprite sheets

In **Sprite animations**, choose a sheet and use **Export animation list…**.
This writes a `.cfanimations` file containing names, inclusive frame ranges,
total duration, loop, reverse and mirror settings. It contains no pixels or
palette data.

Choose another sheet with the same columns, rows, frame width and frame height,
then use **Import animation list…**. Import replaces its clip list after
confirmation and preserves its image, colours, name and ID. Normal project Undo
can reverse the edit. Incompatible layouts and invalid ranges are rejected.

Frames are numbered from zero, left to right and then top to bottom. Animation
duration covers the whole range. A one-shot clip holds its final frame; a loop
repeats until replaced. Character behaviour binds the names you create:

```text
behavior top_down
speed 1
every 1
animation idle
on_move walk
on_idle idle
```

This is a **player behaviour script**, not an event script. Create clips named
`idle` and `walk` on that player's sheet before using it. Directional bindings
and interaction animations are covered in the Authoring reference chapter.

## Test the right thing: preview, area, scene and full game

**Preview** uses the shared native runtime for quick authoring checks. **Test
Scene**, **Build & Play This Area**, and **Build & Play** compile real ROMs and
run them with the embedded mGBA core. Native preview is not a substitute for ROM
verification.

Testing an area creates an isolated copy and disables area exits. Testing a
menu scene directly gives it no menu caller. To check nested returns, saves,
scene transitions or full startup, build the full game from its Default Scene.

A useful pause-menu test is: enter the world, move away from spawn, press Start,
open Save, return to Pause, then Resume. The player should still be where you
left them. Test save/load separately with the saved fields you enabled.

## Troubleshooting ROM size and bank overflow

A cartridge has several ROM banks; fitting the total cartridge size does not
ensure every individual bank fits. A fixed-bank overflow can occur even with
small image data. CartFoundry banks dialogue records, text, event bytecode and
resources, and compresses/deduplicates graphics losslessly.

Use a current application build and check the complete build log. Warnings about
conditional flow changed by the optimizer or unreachable code are not themselves
a bank-overflow error. Multiple-write or bank-overflow messages indicate a
failed build. Keep the project that reproduces the issue so its resource and
runtime budgets can be inspected.

Sprite hardware budgets apply per animation frame; cartridge storage covers the
whole sheet. Removing unused frames or sharing clips can help, but adding a save
menu should not require reducing image quality. Consult the authoring and project
format references for the current asset and scene limits.

## Application version, updates and further help

On macOS, **CartFoundry → About CartFoundry** shows the version and the
[EmeraldSoftlab website](https://eslab.click). On Windows, find About in Help.

**Help → Check for Updates…** checks manually. **Ask about updates on launch**
controls automatic offers. An update is offered only when a newer numeric version
has an available package for your platform. Approval downloads and installs the
whole application; the installation keeps a rollback copy. Save your project
before replacing an application manually. Availability depends on published
release packages and metadata.

For a full control-to-command mapping, see the Authoring reference chapter.
Map building explains painting, placing and reusing characters and objects.
The Pokémon walkthrough provides a longer worked game example. The Project
format chapter documents saved project structures for advanced use.
