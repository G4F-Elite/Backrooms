# Backrooms Mod

Backrooms game for G4F-Elite.
Writed fully with cpp mingw without assets.

## Seed Round

Co-op loot run for up to four: a garage startup steals furniture from the Backrooms, breaks its only
way home on day one, and has to feed loot to a half-finished drill to go deeper - a drill that never
stops crawling, keeps breaking down and has the only safe room on the floor bolted to its back - while a rival crew
downstairs buys the loot, sells supplies and offers a very expensive rope out.
The game explains itself with pictograms and props instead of text. See `LORE.md` and `Reference.md`.

## World

The world is LIMEN, a procedural synthesizer of liminal space (`src/limen_*.h`): endless slabs grown
by a gene field and a space grammar, megastructures, level DNA (every level number is its own place),
generated sound and a narrator that rewrites what the player has not seen yet. `build.bat` builds it
into `build/backrooms.exe`; `sh tools/limen/build.sh` builds `limen_tool.exe` (self-tests, maps:
`build/limen_tool.exe test 1337`) and `limen_shots.exe` (frame grabber, geometry audit). See `LIMEN.md`.
