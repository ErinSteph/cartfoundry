# Building maps from existing objects — CartFoundry 0.9.0

Open **Sprites, tilesets & overworld**. Map editing now starts in **Select / move**
mode. The toolbar above the canvas switches between selection, stamping and tile
painting; the full tool dropdown still offers collision, spawn, dialogue and exit
layers.

## Import a sprite and place it immediately

1. Click **Import & place sprite…** on the toolbar directly above the map.
2. Choose an image, adjust its size if needed, then click **Import & place**.
3. Click the map to place it. Keep clicking for additional copies; press **Escape**
   when finished.

The imported sprite becomes the active brush automatically. A transparent sprite
preview follows the pointer, and the toolbar names what you are placing. You do
not need to find it in another tab or switch tools. The sidebar's sprite import
button follows this same flow. To reuse a sprite already imported, double-click
its thumbnail in the asset list below the import buttons.

## Select and move

Click a sprite's visible image, an object's tile, or the player to select it. Drag
it to another tile. The **Selected / instances** tab also lists every placed object,
character and player spawn, including instances hidden behind another sprite.
The inspector edits name, script target ID, position, graphic, collision, storage,
behaviour and event scripts without leaving the map workspace. Graphic tiles have
image thumbnails. Dragging commits one project Undo step.

**Delete / Backspace** removes a selected object or character when the map has
focus. **Ctrl/Cmd+D** duplicates it into a nearby free tile; the inspector's buttons
do the same. The player spawn can be moved but cannot be deleted or duplicated.
Placement rejects occupied tiles, blocked character positions and hardware limits.
Deleting an instance does not erase scripts that refer to it; validation identifies
those references before a build, and project Undo restores the instance.

## Place and reuse

The right-hand **Place / reuse** palette contains:

- New script objects and containers, using the current tile brush.
- Every sprite asset, placed as an idle character.
- NPC library entries, with their authored behaviour and events.
- Existing objects and characters from every area, with thumbnails and area names.

Search the palette, choose an entry, then click the map to stamp copies. The
highlighted tile previews the placement location. **Escape** returns to Select /
move. Choose a tile from the tilesheet/thumbnail list to paint terrain; while a
new-object/container brush is active, the tile choice supplies that object's
image instead.

Copies are independent instances: editing one does not edit its source. Sprite
pixels remain shared assets. Direct self-event references in a copied instance
are retargeted to its new ID. Existing object graphics require the same tileset
as their source area; the editor reports a mismatch rather than choosing another
graphic. **Save as NPC library entry** in a character's inspector makes a reusable
character available in the palette; the existing library inspector can edit it.

A **new container** creates separate empty storage automatically. A **reused
container** retains its source storage reference, which is useful for multiple
access points to the same inventory or collection. Use **Create separate storage**
to give a copied container its own contents. Storage capacity and initial items
remain editable in **Project & game data**.

## Names and script targets

**Name** is the displayed editor label. **Script target ID** is its stable script
reference, such as `treasure_chest` or `village_guard`. New instances receive
readable unique IDs rather than UUIDs. Use letters, numbers and underscores,
starting with a letter or underscore (up to 32 characters).

Changing a target ID in the map inspector updates direct object/NPC event calls
throughout the project, including scene events, dialogue choices, reusable scripts,
timers, item actions, terrain events and NPC templates. Merely changing the display
name leaves existing references intact. IDs need to be unique within their area
and object/character type.

The read-only **Event call** field provides a complete reference to copy:

```
object area_start treasure_chest interact
npc area_start village_guard hit
```

These call authored events; an NPC `hit` event call does not itself deal damage.
The **Scripts** reference browser also inserts references to the current instances.

## Create a map or area

**+ New map** asks for its name, parent world scene, template, dimensions, tileset
and player sprite. Select **Create a new world scene** when needed. A map group
and its first area are created together. A new world scene uses that area as its
initial area; the project's Default Scene is unchanged.

**Build from → Empty terrain** starts with clear terrain/collision and no copied
objects, characters or events. An existing area supplies its terrain and collision;
**Copy placed objects, characters and map events** is optional. Area exits and
area-entry scripts are cleared for new maps. Direct references between copied
instances are retargeted to the new area. Other external script/storage references
remain shared. The editor rejects a smaller size that would crop copied instances.

**+ Area** adds an area to an existing world map. If there are no world scenes yet,
it opens the new-map workflow. **Duplicate** beside the area selector opens the
same dialog with the current area's template and contents selected.

Use **Use as scene start** to change the parent world's initial area, and **File →
Project Settings → Default Scene** to choose which scene starts the game. Test the
selected area immediately in **Play overworld**, or compile it using **Build & Play
This Area**. Save format 6 is unchanged. Hardware limits remain eight areas, eight
objects and four sprite characters per area, with up to 40 hardware sprites total.
Windows remains an implementation target; this workflow is verified on macOS.
