# Pocket Meadow — an editor-built miniature Pokémon adventure

Open **File → New Pokemon Example**, or open `game.cartfoundry` beside this guide.
Save a copy before experimenting. **Build & Play** runs the real GB/GBC ROM.
There are no project-specific runtime functions: encounters, attacks, catching,
healing and the completion check are ordinary editable events and scripts.

You start with Pikachu, eight Poke Balls and three Potions. Walk east into long
grass. Choose Fight to lower wild HP, Catch to throw a ball, or Run. Bulbasaur,
Charmander and Squirtle can appear. Catching has a 35% chance at high HP and is
guaranteed at 4 HP or less. Each attack deals 2–4; wild retaliation deals 1–3.
Catch all three distinct species and ask Nurse to check your collection.

The nurse stands at tile (6,5), north of the starting path. She restores 20 HP
and gives five balls when fewer than five remain. The PC at (3,5) shows your
party; selecting a Pokémon shows its portrait. Potions in the Bag restore HP.
Arrows move, Z=A, X=B, Backspace=Select, Enter=Start. In a battle choose Run to
leave; B, Bag and Start are deliberately disabled by the prompt's Modal checkbox.

This is a small fan example with newly drawn pixel icons. Pikachu is the fixed
battling starter. The three wild species share basic stats: there are no type
matchups, party switching, levels, evolution, trading or cartridge saves. State
persists between encounters during one play session. Reset the emulator to begin
again.

## Make the project by hand

The template is the fastest starting point. The complete recipe below also works
from **File → New**, using only editor fields, image import/pixel painting and
scripts. No terminal, JSON editing, external engine or hidden game code is needed.
You may freely change names, map layout and artwork without changing the rules.

### 1. Artwork

In **Pixel art**, use **+ Blank sprite** or **+ Blank tileset**. Set dimensions,
choose one of four brush colours and paint with the left mouse button. Right drag
erases; arrow keys plus Space also paint. The X/Y fields and **Paint selected
pixel** support exact edits. Each stroke is one Undo step. **Edit colour…** changes
a palette colour. Colour 0 is transparent for sprites. Zoom and Grid help at pixel
scale. All sample artwork is editable this way.

Alternatively, in **Sprites, tilesets & overworld**, import the PNG files in
`assets/`. Use nearest scaling, the dimensions below, and a four-colour palette.
The converter reorders colours by quantization, so match the preview to the PNG;
colour order itself does not matter. Import trainer.png as a 2×2 sprite sheet
with 16×16-pixel frames. All other sprites are static 16×16. Import the tileset
at its original 64×8 pixels, without resizing. Names, rather than generated asset
IDs, are used in the dropdowns below.

| Asset | Kind | Image size | Frame grid |
|---|---|---|---|
| Meadow tiles | tileset | 64×8 | 1×1 |
| Trainer | sprite | 32×32 | 2×2 |
| Pikachu | sprite | 16×16 | 1×1 |
| Bulbasaur | sprite | 16×16 | 1×1 |
| Charmander | sprite | 16×16 | 1×1 |
| Squirtle | sprite | 16×16 | 1×1 |

For the trainer, create these clips in **Sprite animations** before adding the
animation bindings below: idle = frame 0, 0.5 s, loop; walk = frames 0–3, 0.4 s,
loop; wave = frames 1–3, 0.5 s, once. A static trainer also works: omit the animation
lines in its behaviour. NPCs can use the same trainer image.

The 8×8 tiles run left to right: 0 meadow, 1 hedge, 2 path, 3 sign/PC, 4 long grass,
5 water, 6 roof, 7 doorway/wall. You can draw alternatives as long as grass remains
tile 4 or you change the terrain event to match its new tile number.

### 2. Map, objects and NPC

In the world tab enable **Start game in overworld**, choose Meadow tiles and
Trainer, and resize to **32×24 tiles**. Coordinates start at (0,0).

1. Fill the map with tile 0. Paint tile 1 around x=0, x=31, y=0 and y=23; paint
   the same border on the collision layer.
2. Paint grass (tile 4) across x=12…28, y=6…19. Leave it walkable.
3. Paint path (tile 2) across x=2…11, y=8. Put the player spawn at **(4,8)**.
4. Draw the nurse building at x=3…7, y=2…4: roof tile 6 on rows 2 and 3, wall tile
   7 on row 4. Make those tiles solid. Draw a water pond at x=20…27, y=2…4 using
   tile 5 and mark it solid.
5. Put a sign (tile 3, solid) at **(9,7)**. Once the prompts exist, use **Prompt
   interactions** to connect this tile to the `sign` prompt.
6. In **Project & game data**, select the area and use **+ Add → object**. Name it
   Party PC, set Kind=Container, tile=3, X=3, Y=5, Solid=true. Choose `party` as its
   storage after creating it below. Set **On interact** to `view party`, so choosing
   a Pokémon inspects its entry instead of transferring it. The auto-created unused container storage
   can be removed once this reference has been changed.
7. Add an NPC template called Nurse, choose Trainer, set behaviour `behavior idle`
   (optionally add `animation idle`), and set **On interact** to `dialogue lab`.
   Place the NPC in this area, then select the placed instance in the tree and
   set X=6, Y=5. Templates are copied when placed, so edit the instance too if
   you change the template later. Health can stay at 3; Nurse has no hit/defeat event.
8. Under **Controls & behaviours**, set the player's script to:

```text
behavior top_down
speed 1
every 1
animation idle
on_move walk
on_idle idle
```

### 3. Variables, items and storage

Use **Project & game data → + Add → variable**. Set each ID and initial value:

| ID | Initial value |
|---|---:|
| `hp` | 20 |
| `wild_hp` | 10 |
| `species` | 1 |
| `damage` | 0 |
| `roll` | 0 |
| `wins` | 0 |
| `caught` | 0 |

Use **+ Add → item**. Set its ID before writing scripts that reference it.
Select the matching portrait as Icon for each Pokémon. Descriptions are optional
metadata. Paste the On use event exactly as listed. Blank events stay blank.

| ID | Name | Stack limit | Consumable | On use |
|---|---|---:|---|---|
| `pokeball` | Poke Ball | 20 | no | `dialogue ball_help` |
| `potion` | Potion | 10 | yes | `set hp 20 ; dialogue potion_used` |
| `pikachu` | Pikachu | 1 | no | `dialogue dex_pikachu` |
| `bulbasaur` | Bulbasaur | 1 | no | `dialogue dex_bulbasaur` |
| `charmander` | Charmander | 1 | no | `dialogue dex_charmander` |
| `squirtle` | Squirtle | 1 | no | `dialogue dex_squirtle` |

Semicolons in the table indicate separate lines: use actual newlines in the editor.
Select the existing `inventory`, name it **Bag**, set 8 stack slots, and set initial
Poke Ball count to 8 and Potion count to 3. Other item counts are zero. Add another
storage with ID **party**, name **Pokemon party**, capacity **4**, and initial
Pikachu count **1**. All wild Pokémon counts start at zero. The PC and Nurse's
View party choice both refer to this same storage.

### 4. Create the prompts before writing scripts

In **Conversations & textboxes**, add all prompt IDs from the prompt reference
below. Give each its displayed title. Set **Prompt ID** before wiring or pasting
scripts; click away to commit it. IDs are the stable names used by `dialogue`.
Delete unused prompts from the initial conversation template when finished.
Set the starting prompt to `intro`.

Every example prompt has textbox X=0, Y=5, Width=20, Height=13. Its portrait appears
above it. Toggle **Modal dialogue** on the entries marked modal. This protects
battles from Back/Bag/Start shortcuts. Enter body text, add choice rows, choose
their destination prompts, and paste any conditions/actions from the reference.
`{hp}` and other `{variable}` fields display live values in body text.

### 5. Reusable scripts

Use **+ Add → script**, enter its ID/name, then paste its Source. All nine scripts
are listed below. Indentation is optional; one command goes on each line. `if`,
`else` and `end` form branches. `sub` subtracts a literal or another variable.
`random variable min max` rolls an inclusive range. `result` is read immediately
after a storage operation to check success. `resume` returns to the overworld.
The game uses only these ordinary commands; its rules are not compiled into the
engine as a special Pokémon battle system.

#### `show_battle` — Show the wild Pokemon

```text
if species == 1
 dialogue battle_bulbasaur
else
 if species == 2
  dialogue battle_charmander
 else
  dialogue battle_squirtle
 end
end
```

#### `wild_encounter` — Start a wild encounter

```text
if hp > 0
 set wild_hp 10
 random species 1 3
 call show_battle
end
```

#### `hurt_player` — Wild Pokemon attacks

```text
random damage 1 3
sub hp damage
if hp < 0
 set hp 0
end
```

#### `attack_turn` — Pikachu attacks

```text
random damage 2 4
sub wild_hp damage
if wild_hp <= 0
 set wild_hp 0
 add wins 1
 dialogue victory
else
 call hurt_player
 if hp == 0
  dialogue fainted
 else
  call show_battle
 end
end
```

#### `store_capture` — Store a caught Pokemon

```text
if species == 1
if has party bulbasaur >= 1
 dialogue already_owned
else
 give party bulbasaur 1
 if result == 1
  add caught 1
  dialogue caught_bulbasaur
 else
  dialogue party_full
 end
end
else
if species == 2
if has party charmander >= 1
 dialogue already_owned
else
 give party charmander 1
 if result == 1
  add caught 1
  dialogue caught_charmander
 else
  dialogue party_full
 end
end
else
if has party squirtle >= 1
 dialogue already_owned
else
 give party squirtle 1
 if result == 1
  add caught 1
  dialogue caught_squirtle
 else
  dialogue party_full
 end
end
end
end

```

#### `throw_ball` — Try to catch the wild Pokemon

```text
take inventory pokeball 1
if result == 1
 random roll 1 100
 if wild_hp <= 4
  set roll 1
 end
 if roll <= 35
  call store_capture
 else
  call hurt_player
  if hp == 0
   dialogue fainted
  else
   dialogue catch_failed
  end
 end
else
 dialogue no_balls
end
```

#### `heal` — Heal and refill balls

```text
set hp 20
if has inventory pokeball < 5
 give inventory pokeball 5
end
```

#### `report` — Check the collection

```text
if has party bulbasaur >= 1
 if has party charmander >= 1
  if has party squirtle >= 1
   dialogue complete
  else
   dialogue progress
  end
 else
  dialogue progress
 end
else
 dialogue progress
end
```

### 6. Grass encounters and starting the game

Select the area under **Project → Scene → Map → Pocket Meadow**. Click
**+ Terrain event** and set **Tile 4**, **Every 2 steps**, **35% chance**. Its event
is `call wild_encounter`. A step means entering a new tile with the player's foot
box, not elapsed time or animation frames. Standing still does not roll. Only
entries onto the selected tile count toward the interval.

Set **On enter area** to `dialogue intro`. Keep the interaction ray at 16 pixels.
The overworld combat ray is unused by this battle system; battles are scripted
choice menus. Place the Nurse and PC as above, and connect the sign after creating
its prompt. Every referenced script, item, variable, storage, prompt and asset is
visible in the editor. Broken references and text overflow are reported before
ROM export.

### 7. Test and export

Use **Play overworld** for immediate testing. Choose Back to trail, walk east to
the grass, Fight until the wild has 4 HP or less, then Catch. Open the PC or ask
Nurse to view the party and confirm the captured Pokémon appears. Repeat for all
three, then Check collection. Test Run, empty balls, Potion use, fainting and Nurse
healing too. To test encounters quickly, temporarily set Every=1 and Chance=100;
restore 2/35 for the normal example.

**Build & Play** tests the complete real ROM in the embedded emulator. Switch the
toolbar to Game Boy to test monochrome too. **Save** writes the self-contained
project; **Build ROM…** exports the cartridge. The sample fits a 32 KiB cartridge.
The example is bundled in the app, including this guide and importable PNGs.

## Prompt reference

Unless specified, choices have no condition/action and prompts have no entry
script. Every textbox is `[0,5,20,13]`. Empty choice lists are transient routing
prompts whose entry scripts choose the next screen.

### `intro` — POCKET MEADOW

Portrait: **pikachu**. Modal: **no**.

```text
Pikachu is yours!
Catch 3 wild Pokemon
in the long grass.
Nurse heals north.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `battle_bulbasaur` — WILD BULBASAUR

Portrait: **bulbasaur**. Modal: **yes**.

```text
Wild HP {wild_hp}
Pikachu HP {hp}
Weaken, then catch!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Fight | `resolve_fight` | always | — |
| Catch | `resolve_catch` | `has inventory pokeball >= 1` | — |
| Run | `escaped` | always | — |

### `caught_bulbasaur` — CAUGHT BULBASAUR

Portrait: **bulbasaur**. Modal: **yes**.

```text
Bulbasaur joins
your party!
Caught {caught} of 3
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `dex_bulbasaur` — BULBASAUR

Portrait: **bulbasaur**. Modal: **no**.

```text
A wild friend from
the meadow.
Use the PC to view
your collection.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `battle_charmander` — WILD CHARMANDER

Portrait: **charmander**. Modal: **yes**.

```text
Wild HP {wild_hp}
Pikachu HP {hp}
Weaken, then catch!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Fight | `resolve_fight` | always | — |
| Catch | `resolve_catch` | `has inventory pokeball >= 1` | — |
| Run | `escaped` | always | — |

### `caught_charmander` — CAUGHT CHARMANDER

Portrait: **charmander**. Modal: **yes**.

```text
Charmander joins
your party!
Caught {caught} of 3
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `dex_charmander` — CHARMANDER

Portrait: **charmander**. Modal: **no**.

```text
A wild friend from
the meadow.
Use the PC to view
your collection.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `battle_squirtle` — WILD SQUIRTLE

Portrait: **squirtle**. Modal: **yes**.

```text
Wild HP {wild_hp}
Pikachu HP {hp}
Weaken, then catch!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Fight | `resolve_fight` | always | — |
| Catch | `resolve_catch` | `has inventory pokeball >= 1` | — |
| Run | `escaped` | always | — |

### `caught_squirtle` — CAUGHT SQUIRTLE

Portrait: **squirtle**. Modal: **yes**.

```text
Squirtle joins
your party!
Caught {caught} of 3
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `dex_squirtle` — SQUIRTLE

Portrait: **squirtle**. Modal: **no**.

```text
A wild friend from
the meadow.
Use the PC to view
your collection.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `resolve_fight` — BATTLE

Portrait: **pikachu**. Modal: **yes**.

```text
Pikachu attacks!
```

On enter:

```text
call attack_turn
```

No choices.

### `resolve_catch` — POKE BALL

Portrait: **pikachu**. Modal: **yes**.

```text
Throwing a ball...
```

On enter:

```text
call throw_ball
```

No choices.

### `battle_router` — BATTLE

Portrait: **pikachu**. Modal: **yes**.

```text
A wild Pokemon!
```

On enter:

```text
call show_battle
```

No choices.

### `victory` — VICTORY

Portrait: **pikachu**. Modal: **yes**.

```text
The wild Pokemon
fled after battle.
Wins {wins}
Try catching next!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `escaped` — GOT AWAY

Portrait: **pikachu**. Modal: **yes**.

```text
You escaped safely.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `fainted` — PIKACHU FAINTED

Portrait: **pikachu**. Modal: **yes**.

```text
Pikachu needs rest.
Go north to Nurse
for free healing.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `catch_failed` — BROKE FREE

Portrait: **pikachu**. Modal: **yes**.

```text
The ball broke open!
The wild attacked.
Pikachu HP {hp}
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Try again | `battle_router` | always | — |

### `no_balls` — NO POKE BALLS

Portrait: **pikachu**. Modal: **yes**.

```text
Ask Nurse for more
Poke Balls.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to battle | `battle_router` | always | — |

### `already_owned` — ALREADY CAUGHT

Portrait: **pikachu**. Modal: **yes**.

```text
This Pokemon is
already in your
party. Find another!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `party_full` — PARTY FULL

Portrait: **pikachu**. Modal: **yes**.

```text
Your party is full.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `lab` — NURSE

Portrait: **pikachu**. Modal: **no**.

```text
Free healing and
Poke Ball refills.
Caught all 3 yet?
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Heal / refill | `healed` | always | — |
| Check collection | `progress` | always | `call report` |
| View party | `lab` | always | `view party` |

### `healed` — ALL BETTER

Portrait: **pikachu**. Modal: **no**.

```text
Pikachu has 20 HP.
Your bag has fresh
Poke Balls. Good luck!
```

On enter:

```text
call heal
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `progress` — KEEP EXPLORING

Portrait: **pikachu**. Modal: **no**.

```text
Caught {caught} of 3
Find Bulbasaur,
Charmander and
Squirtle in grass.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `complete` — WELL DONE!

Portrait: **pikachu**. Modal: **no**.

```text
You caught all 3!
Your tiny adventure
is complete. Keep
exploring for fun.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `potion_used` — POTION USED

Portrait: **pikachu**. Modal: **no**.

```text
Pikachu HP {hp}
Ready to explore!
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `ball_help` — POKE BALLS

Portrait: **None**. Modal: **no**.

```text
Use Catch during
battle. Weaken wild
Pokemon to 4 HP
for a sure catch.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `dex_pikachu` — PIKACHU

Portrait: **pikachu**. Modal: **no**.

```text
Your loyal starter!
Battle with Fight;
rest with Nurse.
HP {hp} of 20
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

### `sign` — LONG GRASS

Portrait: **None**. Modal: **no**.

```text
Grass may hide wild
Pokemon. Weaken to
4 HP before Catch.
Select opens Bag.
```

| Choice | Destination | Enabled when | On choose |
|---|---|---|---|
| Back to trail | `intro` | always | `resume` |

