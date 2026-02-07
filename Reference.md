# Backrooms VHS Horror - Complete Reference

## Build & Run
```bash
build.bat          # Compile
build/backrooms.exe  # Run
```

## Code Architecture

**IMPORTANT: Max 250 lines per file, each module has ONE responsibility**

### Module Structure
```
src/
├── game.cpp        - Main entry, includes all modules
├── game_loop.h     - Main loop logic
├── math.h          - Vec3, Mat4 math
├── audio.h         - Sound generation
├── shaders.h       - GLSL shader code
├── textures.h      - Procedural textures
├── geometry.h      - Mesh building
├── render.h        - OpenGL setup
├── world.h         - Chunk generation
├── entity_types.h  - Entity data structures
├── entity_model.h  - Entity mesh generation
├── entity.h        - Entity AI/manager
├── story.h         - Notes & phases
├── menu.h          - UI rendering
├── input.h         - Input handling
├── net.h           - UDP networking
├── player_model.h  - Player meshes
└── menu_multi.h    - Multiplayer menus
```

### Module Responsibilities

| Module | Purpose | Dependencies |
|--------|---------|--------------|
| math.h | Vector/Matrix math | None |
| audio.h | Sound synthesis | windows.h |
| shaders.h | GLSL code strings | None |
| textures.h | Generate textures | OpenGL |
| geometry.h | Build mesh data | math.h |
| render.h | VAO/FBO helpers | OpenGL |
| world.h | Maze generation | math.h |
| entity_types.h | Entity structs | math.h |
| entity_model.h | Entity meshes | geometry.h |
| entity.h | Entity logic | entity_types.h |
| story.h | Game story | math.h |
| menu.h | Draw UI | OpenGL |
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
| E | Interact |
| ESC | Pause |
| Space | Skip/Confirm |

## Game Systems

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

## Audio Channels

| Sound | Trigger | Volume |
|-------|---------|--------|
| Hum | Always | 15% |
| Footsteps | Walking | 50% |
| Danger | Entities near | 0-20% |
| Heartbeat | High danger | 35% |
| Whispers | Low sanity | 12% |
| Scare | Attack | 80% |
| Distant | Random | 30% |