# Names, menus, keyboards and save slots

Open `game.cartfoundry`. Build & Play starts a naming keyboard, followed by the
world. Arrows select keys, A types, `<` deletes, `!` accepts, and `X` cancels.
Cancelling the opening keyboard uses the default name "Player".

Start opens the custom pause menu. Rename your character or save label, open the
save menu and choose a slot. Move the player, then load the slot to restore its
position, text, variables, storage, timers and random state. The emulator writes a
`.sav` beside the ROM; keep that file to retain saves across launches.

In **Scenes & menu builder**, choose a scene to customize its layout and actions.
**Text, keyboard & save builders…** edits text capacities, character sets, keyboard
grids and save fields. The save menu label is an in-game name; it does not rename
the emulator's `.sav` file. Preview saves are separate from ROM saves.

Actor movement, actor health and audio are transient. Use saved numeric variables
for permanent story outcomes. Changing the saved data schema rejects older slots.
