# Backrooms: Seed Round - Complete Reference

Co-op loot run: a startup that steals furniture from the Backrooms. Story and tone: `LORE.md`.

## Build & Run
```bash
build.bat          # Compile
build/backrooms.exe  # Run
```

## Code Architecture

**IMPORTANT: Max 500 lines per file, each module has ONE responsibility**

### Module Structure
```
src/
├── game.cpp        - Main entry, includes all modules
├── game_loop.h     - Main loop logic
├── math.h          - Vec3, Mat4 math
├── audio.h         - Sound state, mixer (stereo, reverb, limiter)
├── audio_synth.h   - Oscillators, filters, noise, room reverb
├── audio_params.h  - Game parameters -> synth controls (route table)
├── audio_cues.h    - Layered one-shots: scare, hits, warnings, monster calls
├── audio_director.h- Threat stages, when warnings fire
├── audio_ambience.h- Light hum, air, far-away events
├── audio_sfx.h     - Footsteps, flashlight, menu
├── audio_threat.h  - Drone, heartbeat, chase pulse, monster beds, breath, whispers, death
├── audio_toy.h     - Squad squeaks, shoves, callouts
├── shaders.h       - GLSL shader code
├── textures.h      - Procedural textures
├── geometry.h      - Mesh building
├── world_mesh.h    - Level vertex data (no OpenGL)
├── zfight_guard.h  - Finds and repairs z-fighting faces
├── render.h        - OpenGL setup
├── world.h         - Chunk generation
├── entity_types.h  - Entity data structures
├── entity_model.h  - Entity mesh generation
├── entity.h        - Entity AI/manager
├── story.h         - Notes & phases
├── menu.h          - Font, text, backdrop (UI kit: see "UI")
├── input.h         - Input handling
├── net.h           - UDP networking
├── player_model.h  - Player meshes
└── menu_multi.h    - Multiplayer menus
```

### Module Responsibilities

| Module | Purpose | Dependencies |
|--------|---------|--------------|
| math.h | Vector/Matrix math | None |
| audio.h | Sound synthesis and mix (`audio_*.h` hold the voices) | windows.h |
| shaders.h | GLSL code strings | None |
| textures.h | Generate textures | OpenGL |
| geometry.h | Build mesh data | math.h |
| render.h | VAO/FBO helpers | OpenGL |
| world.h | Maze generation | math.h |
| entity_types.h | Entity structs | math.h |
| entity_model.h | Entity meshes | geometry.h |
| entity.h | Entity logic | entity_types.h |
| story.h | Game story | math.h |
| menu.h | Font, text, backdrop; UI kit in ui_core.h / ui_widgets.h | OpenGL |
| input.h | Handle keys | GLFW, menu.h |
| net.h | Networking | winsock2 |
| player_model.h | Player meshes | net.h |
| menu_multi.h | MP menus | menu.h, net.h |
| game_loop.h | Main loop | All above |
| game.cpp | Entry point | game_loop.h |

## Multiplayer

### Setup (LAN via Radmin/Hamachi)
1. All players install Radmin VPN or Hamachi
2. Create/join same virtual network
3. Host starts game → Multiplayer → Host Game
4. Others join with host's Radmin/Hamachi IP
5. Port: **27015** (UDP)

### Features
- Up to 4 players
- Shared world seed
- See other players (colored models)
- Synced entity positions
- Synced scare events

### Network Protocol (UDP)
```
Packet Types:
- PKT_JOIN (1)       - Request to join
- PKT_WELCOME (4)    - Accept + world seed
- PKT_PLAYER_STATE   - Position sync
- PKT_ENTITY_STATE   - Entity sync
- PKT_SCARE          - Jump scare sync
```

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Shift | Sprint |
| Ctrl | Crouch |
| F | Flashlight |
| E | Interact: pick up, deliver, feed / start the drill, sell, take the charger battery. Hold: rummage, break a cracked wall, repair the rig, revive |
| R | Drop the last thing you picked up |
| 1 / 2 / 3 / 4 | Flashlight / scanner / plush / quick-use supply. At the kiosk: buy battery / med spray / plush / extraction |
| ESC | Pause |
| Space | Skip/Confirm |
| LMB | Shove a teammate (co-op) |
| Q | Squeak (heard by the squad, raises attention) |
| G | Ping marker (co-op) |
| T / Y / U / I / O | Squad callouts (co-op) |

Dead in co-op (spectator camera):

| Key | Action |
|-----|--------|
| A / D (or LMB / RMB) | Previous / next target: teammates first, then the monsters |
| Mouse | Orbit the camera around the target |
| W / S | Zoom in / out |
| V | Bodycam: look through the teammate's eyes |
| Space | Let go and walk around as a free ghost (Space again to ride) |
| Q | Squeak from wherever you sit (blind things come to check) |

## Game Systems

### The run
The HUD explains all of this with pictograms; there is no objective text (see "Show, Don't Write" in `LORE.md`).

| Stage (`HeistStage`) | What happens | Ends when |
|-------|--------------|-----------|
| `TUT_GARAGE` | squad stands in the garage, marker on the portal | somebody walks through |
| `TUT_HAUL` | three deliveries to the pallet: carry / flashlight (dark wing) / cracked wall (scanner) | 3 boxes on the pallet |
| `TUT_CORE` | the core appears; marker leads back to the pallet | its carrier reaches the portal |
| `ACCIDENT` | flash, blackout, portal cell becomes a wall, everyone ends up on the Backrooms side | 4.5 s |
| `RUN` | the rig crawls and breaks down; loot is scattered along its way; feed the drill, sell to the kiosk, study monsters | drill quota met and started |
| `DRILLING` | shake + fade | 4.2 s, then the next floor |
| `EXTRACTED` | bought at the kiosk | report screen, run over |

- First floor (`B1`) = garage + tutorial + first drill quota, no kiosk. Players who have seen the accident
  (`save_heist.dat`) start at `RUN`; main menu -> TUTORIAL replays it.
- Monsters, wall shifts, floor events, POI risks and sanity drain only run while `heistDangerActive()`.
- Loot: 8 kinds (`heist_rules.h`), value x(1 + 0.35 per floor), weight 1-3, four carry slots, weight slows and tires.
  Sources: lying around, junk piles (hold E), POI rooms, caches inside cracked walls.
- Cracked walls: ~9% of wall cells with floor on two opposite sides. Hold E facing one (2.4 s, loud). 60% hold a cache.
  Broken walls and stamped rooms are persistent cell overrides (`gCellOverrides` in `world.h`), so they survive chunk reloads.
- Drill quota: 180, then 260 + 170 per floor. Five gauge lamps; each new lamp also brings dead co-op players back.
- Kiosk (from `B2`): sell carried loot (E), buy battery 50 / med 110 / plush 90, extraction 3000 + 900 per extra player.
- Field notes: scanner out, monster in view for 3.2 s -> cash (120 + 40 per floor), once per kind per floor;
  the monster's rule pictogram then stays on the HUD.
- The drill is a moving base, see "The rig" below. The hopper, the gauge and the start lever are inside it.
- Wallet, drill and field notes are shared by the squad. Carried loot and the wallet ride the drill to the next floor.
- Squad wipe / solo death: the floor restarts with a new layout and half the wallet.
- Co-op: the host owns the heist state and mirrors it in `NetVoidShiftState` (`NetHeistState`); clients ask through
  interact requests (`REQ_PICK_ITEM`, `REQ_HEIST_ACTION`, `REQ_HEIST_BREAK`, `REQ_HEIST_BUY`, `REQ_HEIST_STUDY`).
  Loot and junk piles are ordinary world items, so they ride the existing item snapshot.

### The rig (moving base)
The drill is a site trailer on crawler tracks: auger in front, engine, a break room with a back door. Numbers: `rig_rules.h`.

- **Stations** (no words, shapes only; stripes = hands go here). On the bulkhead, straight ahead from the door:
  the hopper (open bin, striped rim, striped arrow and a carton plate above it), the gauge next to it (5 lamps, a
  drill bit on top, fed by a pipe from the hopper), the lever on the other side (latched under a dark arrow plate
  until the gauge is full, then the plate is lit; down while drilling). Charger = dock with a battery drawn on it,
  on the desk-side wall by the bulkhead. Fuse box by the door; engine hatches outside, above the tracks.
  The carton-arrow-funnel plate is repeated outside, next to the back door.

- **Crawls** at 0.30 m/s along one row of cells (`rigDir`, fixed per floor; `+X` on `B1`, seeded below), only in `RUN`,
  only while nothing is broken and the quota is not met. Lane cells are opened 1.2 m ahead of the auger tip as
  persistent overrides, on every peer, from start cell + heading + distance (`rigCarveLane`). No door frames, props,
  pits, cracks or loot on the lane next to it (`heistIsReservedCell`).
- **Riding**: whoever stands on the floor plate is carried; the hull is solid for players and monsters
  (`rigMoveLocalPlayer`, `rigCollide`). The squad starts every floor (and veterans start `B1`) seated in the break room;
  ghosts respawn there. Touching the auger while it crawls: 34 hp/s and a throw forward.
- **Haven** (`EntityManager::havenPos / havenRadius`): running = 14 m around the break room, broken = 2.6 m (the room, not the engine covers outside)
  for the 25 s the spare battery lasts, then 0. Inside it squad members and their noise do not exist for the monsters,
  and a monster that ends up inside walks out (or leaves the stage after 4 s). For the local player in the haven:
  no danger tint / heartbeat / monster voices, no jump scares (`gScaresMuted`), attention runs down.
- **Quiet ring** (`quietRadius`, 27 m, running rig only): monsters that are not hunting, charging or attacking turn
  back, nothing spawns or re-enters there, no floor holes open there. While the host stands in it no roam event
  except the care package fires, attention events are skipped, and furnished rooms inside it never roll a risk.
- **Comfort**: in the haven +2.4 morale/s; in the break room +5 morale/s and, with the engine on, +1.2 hp/s up to 80
  and flashlight +3 %/s. Charger: one battery per 70 s of engine time. Radar: loot / junk pile markers within 34 m
  while standing in the room.
- **Breakdowns**: every `78 - 7 * floor` s of crawling (min 36), +-30%; one of fuse box (inside, by the door) /
  belt hatch / valve hatch (outside, either side of the engine). The broken one hangs open, framed in stripes: hold E 2.2 s there. A dead rig
  backfires every 7 s (noise radius 28 m, +5 attention).
- **Loot follows the rig**: the first scatter is laid from 4 cells behind to 16 ahead of the start; every cell gained
  adds one item 3-8 cells ahead (every third is a junk pile), anything 45 m behind is removed, at most 24 alive.
- The kiosk stands 6-9 cells ahead of the arrival spot, 2-3 cells off the lane: the rig drives past it.
- Co-op: `drillPos`, `rigStart`, `rigDir`, fault, fault seconds and charge ride `NetHeistState`; clients predict the
  crawl and are nudged towards the host position. Requests: `HEIST_ACT_REPAIR`, `HEIST_ACT_BATTERY`.

### Floor events and the work chat
In-fiction framing: "The Building Has Opinions", "Morale" and "The Work Chat" in `LORE.md`.

| Lore name | Code | Notes |
|-----------|------|-------|
| how loud the crew is | `attentionLevel` (`void_shift_runtime.h`) | sprint, flashlight, squeaks raise it; thresholds 50 / 75 / 90 |
| phones ringing | `triggerConferenceCallSetpiece` | B1 only |
| breaker / lights out | `triggerBlackoutSetpiece`, `ROAM_LIGHTS_OUT` | |
| walls move | `triggerCorridorShiftSetpiece`, `ROAM_GEOM_SHIFT` | solo only for now; co-op skips the reshuffle |
| fake doorway | `ROAM_FALSE_DOOR` | invisible blocker for the event duration |
| holes in the floor | `ROAM_FLOOR_HOLES` (`traps.h`) | never within 2 cells of a player |
| the pit | `placeAbyss` (`objective_layout.h`) | from B2 |
| care package | `ROAM_SUPPLY_CACHE` | batteries 70% / plush 30% |
| furnished rooms | `poi_runtime.h` | loot, risk (one of the events above), or both |
| morale | `playerSanity` (`player_vitals.h`) | plush: hold in hands = regen (always available, no use action), downed + plush = self-revive |
| work chat | `setTrapStatus` / `setEchoStatus` (`session.h`) | the two status lines; style rules at the end of "The Work Chat" |

Attention-spawned Crawler / Shadow are solo only; in co-op the pacing director does the sending.
Not wired into the loop (dead code, old design): echo signals and the trap corridor in `traps.h`, `smile_event.h`.

### Heist modules

| File | Content |
|------|---------|
| heist_rules.h | pure numbers: loot table, carry slots, quotas, prices, stages (tests: `tests/heist_rules_tests.cpp`) |
| heist_net.h / heist_state.h | wire struct, shared run state, local feedback (toasts, flash, hold progress) |
| heist_world.h | garage + portal stamp, kiosk / lesson placement, loot scatter, cracked walls, client apply |
| heist_models.h | kiosk, portal rig / wreck, pallet, whiteboard, crack faces, loot and junk pile meshes |
| rig_rules.h | pure numbers and rig-space geometry: hull boxes, panels, lane steps, breakdown timing, haven radius (tests: `tests/rig_rules_tests.cpp`) |
| rig_state.h / rig_world.h | where the rig is, haven / lamp queries, lane cells; placement per floor, lane carving, seats |
| rig_models.h / rig_render.h | outside: tracks, shell, roof, engine and hatches, auger, arrival hole; drawn with a model matrix, lamp fed in as a scene light |
| rig_cabin_models.h | inside: `RigLook`, furniture, hopper / gauge / lever, charger, fuse box, pictogram helpers |
| rig_host.h | host: breakdowns, repair, charger, backfires, loot ahead of the rig |
| rig_play.h | every peer: crawl / prediction, carrying and pushing the player, haven for the monsters, comfort, sounds |
| heist_sets.h | puts the models into the decor mesh, rebuilds it when the run changes, collision |
| heist_render.h | per-loot meshes for world items |
| heist_host.h | host rules: pick, deposit, sell, buy, break, study, the accident, starting the drill |
| heist_play.h | local side: prompt, interact / hold / shop keys, study, stage feedback, per-frame `heistUpdate()` |
| hud_icon_ids.h / hud_icon_art.h / hud_icons.h | pictogram ids, CPU atlas painter, GL drawing (icons, key caps, gauges, hold ring) |
| hud_heist.h | goals, carry slots, prompts, toasts, kiosk price list, lesson hints, field notes |
| hud_gameplay.h | world markers + edge pointer, downed / revive prompts |
| hud_shift_report.h | extraction report, death screen, key sheet (pause -> KEY SHEET) |
| void_shift_runtime.h | attention (noise / light) and the events it triggers |

### Monsters
Each one follows a single rule the squad can learn and abuse. They only know what they saw,
heard, or were hinted by the pacing director; none of them reads player positions.

| Monster | Rule | Counterplay |
|---------|------|-------------|
| Stalker | Moves only while nobody has it on screen (line of sight counts, in a blackout only a flashlight counts) | Somebody keeps watching it while the others work |
| Crawler | Blind. Hears footsteps (crouch 2.5 m, walk 9 m, sprint 21 m), squeaks, callouts, shoves and pings; walls muffle sound. Stops, listens, then charges in a straight line | Crouch or stand still; sidestep the charge; make it run into a wall or pillar (it knocks itself out) |
| Shadow | Sneaks up from behind on whoever is alone, routing around what the victim can see | Stick together; a glance sends it away, staring for ~2.5 s enrages it |

- A charging crawler bowls over whoever it hits
- Monsters path around walls (A* on the cell grid) and remember the last place they saw or heard somebody
- Pacing director: after a rough moment (damage, somebody downed) monsters back off and spawns pause for a breather;
  when it has been quiet for too long one monster is pointed at the squad's rough position (sooner at high ATTENTION)
- Monsters enter out of sight, a short walk from the squad, and re-enter nearby if the squad outruns them
- The last squad member standing is hunted a little less aggressively
- Co-op: the host simulates monsters; clients interpolate snapshots (id + position + mood)

### Spectator camera
- A dead co-op player rides along with a teammate as a tiny ghost on their shoulder (or on a monster's head)
- The spectator HUD shows what the monsters around the watched teammate are doing, and in monster cam what that monster is thinking
- Noise made by a ghost is real noise where the ghost sits

### Stamina
- Sprint drains 20/sec
- Recovers 15/sec walking
- Cooldown when depleted

### Flashlight
- 60 sec battery
- Recharges when off
- Red edge = danger nearby

### Sanity
- Entities drain sanity
- Low sanity = hallucinations
- 0 sanity = death risk

### Progressive Difficulty
| Time | Entities | Reshuffle |
|------|----------|-----------|
| 0-1m | 1 | 30% |
| 1-2m | 2 | 40% |
| 3-4m | 4 | 60% |
| 5m+ | 6 | 80%+ front |

## Props & Themes

Every placeable prop is a row in `src/prop_catalog.h`: themes, placement, footprint, pick weight.
Generators ask the catalog for "a prop of theme X" instead of hard-coding lists.

| File | Content |
|------|---------|
| prop_themes.h | theme bits, theme families, ready-made level mixes (`THEME_BLEND_*`) |
| prop_catalog.h | ids, themes, placement kinds, `pickThemedPropType()` |
| prop_models.h | base set used by levels 1-2 (crates, cones, desk, lockers...) |
| props_shared.h | cross-theme: bins, benches, vending, pipes, ducts, signs, shelves |
| props_fixtures.h | cross-theme, fixed to the building: clocks, speakers, wall TV, sprinkler main, payphone, radiator |
| props_clutter.h | cross-theme, loose: dead plant, bookcase, mop bucket, queue posts, stacked chairs, fallen tiles |
| props_office.h / props_parking.h / props_maintenance.h / props_warehouse.h / props_hotel.h / props_pool.h / props_hospital.h | theme sets |
| props_school.h / props_mall.h / props_playroom.h / props_station.h / props_lab.h / props_kitchen.h | theme sets for future levels |
| props_library.h | id -> mesh builder |
| decor_parts.h | slanted beams, rods, cones, balls, wheels, pallets |
| map_theme_spawn.h | themed chunk dressing, companions (bed + nightstand, rows of desks / gates / racks...) |

Themes: `THEME_OFFICE`, `THEME_PARKING`, `THEME_MAINT`, `THEME_WAREHOUSE`, `THEME_HOTEL`, `THEME_POOL`, `THEME_HOSPITAL`,
`THEME_SCHOOL`, `THEME_MALL`, `THEME_PLAYROOM`, `THEME_STATION`, `THEME_LAB`, `THEME_KITCHEN`.
Most props carry several themes, so neighbouring themes blend.

| Theme | Signature props | Blends well with |
|-------|-----------------|------------------|
| school | pupil desk, blackboard, cloakroom bench, vaulting box, trophy case | office, kitchen, playroom |
| mall | clothes rail, mannequin, shop counter, gondola shelving, fitting booth | hotel, playroom, kitchen, station |
| playroom | party table, balloons, ball pit, slide, foam blocks, gifts, bunting | mall, school, pool |
| station | fare gates, ticket machine, route map, platform strip, ticket booth | parking, maintenance, mall |
| lab | server rack, wet bench, fume hood, specimen tank, tape drive | office, maintenance, hospital |
| kitchen | range, fridge, prep table, serving line, extract hood, flour sacks | school, hospital, mall |

Level mixes that are checked to share enough props: `THEME_BLEND_SCHOOL`, `_MALL`, `_FUNHOUSE`, `_METRO`,
`_FACILITY`, `_CLINIC`, `_RESORT`.

Placement kinds: `FLOOR` and `WALL` block movement; `WALL_HIGH` / `CEILING` hang above 2.7 m and
`FLAT` lies on the floor - those three never collide and may share a spot with a low prop.

`levelDecorThemes(level)` in `map_theme_spawn.h` decides how a floor is dressed (each chunk picks one of the
returned themes). `B1`-`B2` return 0 and keep their hand-tuned props; every deeper floor cycles through the
`THEME_BLEND_*` mixes: metro, mall, school, clinic, resort, facility, funhouse.

New prop checklist: enum id + catalog row, mesh in a `props_*.h`, case in `props_library.h`.
Add new ids at the end of the enum; the base set (ids 0-12) keeps its numbers.
Only the 16 atlas materials (`DECOR_MAT_*`) are used, so props match the level finish.

```bash
# heist set pieces and loot, rendered off-screen (see header of tools/heist_preview.cpp for the build line)
build/heist_preview.exe build/heist_shots.bmp
# the rig alone: walk-around, then the stations from the doorway in every state (build line in tools/rig_preview.cpp)
build/rig_preview.exe build/rig_shots.bmp
# scripted play-through of the whole loop with a hidden window: tutorial, accident, drill, kiosk, extraction.
# Writes build/selftest.log and build/selftest_*.bmp (src/dev_selftest.h)
set BACKROOMS_SELFTEST=1 && buildackrooms.exe
# footprint / theme / spawn checks, and real mesh bounds next to the catalog
g++ -std=c++17 -I deps/include tests/prop_library_tests.cpp -o build/prop_tests.exe
build/prop_tests.exe            # asserts
build/prop_tests.exe --measure  # table of bounds
# contact sheets per theme (see header of tools/prop_preview.cpp for the build line)
build/prop_preview.exe build/prop_shots 0
```

## Z-fight Guard

Two faces in the same plane, facing the same way and covering the same spot get the same depth, so their
textures flicker through each other. The engine does not let that reach the screen, and the build does not
let through what the engine cannot fix.

| File | Content |
|------|---------|
| zfight_guard.h | `zfightScan()` (report), `zfightResolve()` (report + repair), `zfightRepair()` (what the engine calls) |
| zfight_grid.h | broad phase: triangles, facing buckets, grid |
| zfight_geom.h | narrow phase: overlap of two coplanar triangles, cutting it out; the limits (`ZfConfig`) |
| world_mesh.h | vertex data of the level (shell, pillars, decor, lamps) without OpenGL, shared by game and build check |
| tools/zfight_check.cpp | the build check |

- **At run time** every mesh goes through the guard before it is uploaded: the world mesh as a whole in
  `buildGeom()` (walls, floor, ceiling, pillars, decor and lamps share one depth buffer), the rig, loot and
  hand-held devices each on their own (`resolveModelMesh()`). A new mesh must take the same route.
- **The repair**: the face that was built first loses the shared area, it is cut out of it. So for content the
  rule is "what is built later lies on top", like paint. Between layers: shell < pillars < decor < lamps.
- **Same plane** means closer than 1.5 mm: a 1 mm step is repaired, a step of 2 mm or more is left to the depth
  buffer. Faces that only touch back to back (a box on a table) are hidden by the solids and are left alone.
- **At build time** `build.bat` compiles and runs `tools/zfight_check.cpp` before the game: all props, POI sets,
  door frames, heist sets, loot, devices, the rig and 16 generated worlds go through the same guard. The build
  is refused when something still flickers afterwards, when a mesh that moves on its own (the rig) has a face in
  the plane of the floor or the ceiling - the engine cannot cut the level under a moving mesh, lift such a face
  by 2 mm or more - or when the guard itself stops finding the test cases.

```bash
build/zfight_check.exe          # pass / fail, the same run build.bat does
build/zfight_check.exe --list   # what the engine repairs, model by model, with positions
# in the game: one entry per repaired mesh
set BACKROOMS_ZFIGHT_LOG=build\zfight.log && build\backrooms.exe
```

## Code Style Guidelines

### DO:
```cpp
// Good: readable formatting
void update(float dt) {
    if (health <= 0) {
        die();
        return;
    }
    
    position += velocity * dt;
    checkCollisions();
}
```

### DON'T:
```cpp
// Bad: minified mess
void update(float dt){if(health<=0){die();return;}position+=velocity*dt;checkCollisions();}
```

### Rules:
1. **Max 250 lines per file**
2. **One responsibility per module**
3. **Readable formatting with newlines**
4. **Comments for complex logic**
5. **Split large functions into helpers**

## File Descriptions

### game.cpp (Entry Point)
- Includes all modules
- Defines global variables
- Includes game_loop.h

### game_loop.h (Main Loop)
- buildGeom() - Build world mesh
- genWorld() - Initialize world
- gameInput() - Handle input
- renderScene() - Draw world
- main() - Game loop

### net.h (Networking)
- NetworkManager class
- UDP socket handling
- Host/Join logic
- State synchronization

### player_model.h (Players)
- Player mesh generation
- Colored per player
- Render other players

### menu_multi.h (MP Menus)
- Host lobby UI
- Join game UI
- IP entry
- Player list

## UI

Menus and the HUD share one kit. Layouts are written for a 1280x720 canvas (origin in the middle,
y up) and scale with the window, so text and panels keep their proportions at any resolution and
aspect ratio.

| File | Role |
|------|------|
| settings_model.h | `Settings`, the tabs and the row table of the settings screen: kind, label, hint, adjust / cycle / value text, bind swapping. No GL (tests: `tests/settings_model_tests.cpp`) |
| menu.h | font atlas, text and overlay shaders, `drawText`, corridor backdrop, `GameState` |
| ui_core.h | canvas units, palette (`UI_*`), flat / gradient quads, text helpers, per-frame state `ui`: click zones, mouse, key edges, typed characters, fade-in and highlight easing |
| ui_widgets.h | panel, header, list row, button, slider, switch, stepper, tabs, text field, key cap, hint bar, spinner. Text on darkness: no boxes, frames or divider lines; selection is the accent colour plus one thin bar |
| menu_screens.h | main menu, pause menu (item order enums shared with input) |
| menu_settings.h | settings screen |
| menu_multi.h | multiplayer: mode + callsign, join (rooms list), host lobby, waiting room |
| menu_hud.h | vital bars, shift clock, hurt / hallucination vignettes, HUD plates |
| input_common.h | `uiPollInput` (one mouse + key snapshot per frame), `uiMenuNav`, `uiBackRequested` |
| input_menu.h / input_settings.h / input_multi.h | what each screen does with that input |

How a screen works: the draw function lays out widgets and each widget registers a click zone
(`uiZone`); next frame the input function calls `uiPollInput`, then `uiMenuNav(count)` for
up/down, hover and click on the items, and checks the special zones it cares about (tabs, slider
tracks, steppers, back). Keyboard and mouse always do the same thing. A click is press + release
on the same item. Text fields read `ui.chars`, filled by the GLFW char / key callbacks.

To add a setting: add a `SET_*` id, one line in the tab's row table and its case in the
`setting*Ref` switch in settings_model.h. The screen and the input pick it up from there.

Every menu screen is drawn into `build/selftest_ui_*.bmp` by the self-test (src/dev_selftest_ui.h);
`BACKROOMS_SELFTEST_SIZE=1920x1080` runs it at another window size.

## Audio

Everything is synthesized, 44.1 kHz stereo. The game thread writes plain values into `sndState`;
`fillAudio()` on the audio thread glides towards them, renders every layer, sends part of each into one
shared room reverb, then limits and soft-clips the sum. Far-away events are mostly reverb, close sounds
are mostly dry.

| Layer | Trigger | Bus (volume slider) |
|-------|---------|---------------------|
| Fluorescent hum, air, building drone | Always | Ambience |
| Far events: pipe groan, vent, scrape, knocks, metal door, lamp flicker; footsteps and a moan under stress | Random, more often with danger / low sanity | Ambience |
| Footsteps, clothes | Head bob, movement | SFX |
| Flashlight switch, scanner chirp, toy sounds | Player actions | SFX |
| Scare (crack, slam, metal, two screams, rumble) | `triggerScare(vol)`, first frame of damage | SFX |
| Mauling hits | `triggerMonsterHit()` while taking damage | SFX |
| Monster calls and alarms | Monster in earshot / its mood changes | SFX |
| Warning stingers, chase pulse | Threat stage goes up / panic | Music |
| Menu blips | Menu navigation | SFX (never fully muted) |
| Breathing | Low stamina, sprint, monster close | Voice |
| Whispers, ear ringing | Sanity below ~18% | Voice |
| Tension drone | `dangerLevel` | Music |
| Heartbeat | `dangerLevel` above 0.3 | Music |
| Monster presence: stalker growl, crawler clicking, shadow wind/choir | Nearest monster within 30 m, panned to its side | Music |
| Death drone, impact, ear ringing | Player dead (ducks everything else) | Music |

### How the sound follows the game

1. **Parameters** (`AudioParam`, `audio_params.h`): danger, insanity, health loss, stamina, move, sprint,
   crouch, dark, monster near / menace / alert / crowd, hurt, depth, pacing. `audioReadGame()` in `audio.h`
   is the only place that fills them from `sndState`. Threat parameters rise in tens of milliseconds and
   fall slowly. The director derives `tension`, `panic`, `exertion`, `whisper` from them.
2. **Routes** (`audioRoutes()`): one row = "this synth control follows that parameter along this curve"
   (add / multiply / max). Controls (`AudioDest`): bed level, hum, far-event stress, drone, heart level and
   rate, chase pulse, monster bed, breath, whispers, footstep level, muffle, reverb size.
   New dependency = new row.
3. **Cues** (`cueDef()`, `audio_cues.h`): a one-shot is a list of layers (noise / thump / metal / throat /
   clicks), one row of numbers each: delay, envelope, pitch glide, gain, pan, reverb send, random detune.
   A cue has a bus, a cooldown and a duck amount (beds drop under loud hits).
   Game thread: `audioPostCue(cue, gain, pan, pitch)`.
4. **Director** (`audio_director.h`): threat stage CALM -> UNEASY -> STALKED -> HUNTED. Going up fires a
   stinger at once (`warn_near`, `warn_hunt`); coming down takes 2.5 s of quiet per stage. The tracked
   monster's `monsterCall` going NONE -> NOTICE -> ALERT fires its voice; idle calls come faster as it
   closes in. `triggerMonsterHit()` every damage frame = one scare, then hits every ~0.4 s.

Listen without launching the game:
```
g++ -std=c++17 -O2 -o build/audio_preview.exe tools/audio_preview.cpp
build/audio_preview.exe            # build/audio_preview/*.wav, one per situation
build/audio_preview.exe out solo   # same, room ambience muted
```
Tests: `tests/audio_engine_tests.cpp`, `audio_dsp_tests.cpp`, `audio_gameplay_state_tests.cpp`,
`menu_audio_feedback_tests.cpp`.
