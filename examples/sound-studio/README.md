# Foundry After Hours

An original eight-bar, 150 BPM Game Boy song composed in CartFoundry's Sound &
music editor. Open `game.cartfoundry`, then select **Sound & music** and the
`foundry_four` entry. All four channel lanes are enabled. Play or Loop auditions
the complete song; channel tabs and Solo let you inspect individual parts.

| Channel | Part | Instrument |
| --- | --- | --- |
| Pulse 1 | Lead melody, second-phrase variation | 50% pulse; velocity accents; fading envelope; left |
| Pulse 2 | Broken-fifth arpeggio | 25% pulse; softer envelope; right |
| Wave | Root/fifth bass | Editable triangle wave; full wave level; center |
| Noise | Kick/snare/offbeat hats | Per-pitch drum noise; fast envelope; velocity accents; center |

The progression is C–Ab–F–G, repeated with melodic variations. The four lanes
share a 768-tick (12.8 second at 60 Hz) loop. Build & Play starts the song on scene
entry with `sound loop foundry_four`. The title screen labels the four parts.

Draw notes in each lane, drag to move them and resize the right edge for length.
The instrument inspector shows only controls supported by that hardware channel.
Wave samples are four-bit (0–15). Drum kit pitches are C2 kick, D2 snare, F#2 closed
hat, A2 tom, A#2 open hat, C#3 crash. Note velocity and raw noise overrides are
available in Selected note. Solo is never saved into the song.

To regenerate this project: `cf-build --example-sound game.cartfoundry`.
Windows audio remains unverified; the demo is tested in actual GB/GBC emulation.
