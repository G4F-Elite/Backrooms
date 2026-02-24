# T17 — Secondary Screens and Overlay Migration

Status: implemented  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T17`  
Date: 2026-02-23

## Scope delivered
- Migrated remaining player-facing secondary text screens to the immersive component system in `src/ui_menu_immersive.h`:
  - Guide screen (`STATE_GUIDE`)
  - Keybind menus (`STATE_KEYBINDS`, `STATE_KEYBINDS_PAUSE`)
  - Intro sequence (`STATE_INTRO`)
  - Note reader overlay (`STATE_NOTE`)
- Migrated session-end player-facing overlays in `src/hud.h`:
  - Death overlay (`isPlayerDead` path)
  - Escape/extraction overlay (`playerEscaped` path)
- Legacy rendering paths remain available as fallback behind migration routing (`useNewUiMigrationPath()`).

## Player-facing legacy text screen checklist
Coverage: 14/14 (100%)

| Surface ID | Screen/Overlay | Status | Migration state |
|---|---|---|---|
| ui-main-menu | Main Menu | migrated | T11 immersive path |
| ui-guide | Guide Screen | migrated | T17 immersive path |
| ui-settings | Settings | migrated | T11 immersive path |
| ui-keybinds | Keybinds Menu | migrated | T17 immersive path |
| ui-pause-single | Pause Menu (singleplayer) | migrated | T11 immersive path |
| ui-intro | Intro Text Sequence | migrated | T17 immersive path |
| ui-note-reader | Note Reader Overlay | migrated | T17 immersive path |
| ui-death | Death Screen | migrated | T17 immersive path |
| ui-escape | Escape/Extraction Screen | migrated | T17 immersive path |
| ui-multi-main | Multiplayer Main Menu | migrated | T11 immersive path |
| ui-multi-join | Multiplayer Join/Connect | migrated | T11 immersive path |
| ui-multi-host-lobby | Host Lobby | migrated | T11 immersive path |
| ui-multi-wait | Multiplayer Waiting/Connecting | migrated | T11 immersive path |
| ui-multi-pause | Multiplayer Pause Menu | migrated | T11 immersive path |

Intentionally retained: none

## Regression smoke checks
Executed targeted migration checks:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t17_integration.ps1
powershell -ExecutionPolicy Bypass -File tools/check_t11_integration.ps1
```

Pass condition:
- Both commands exit with code `0`.
- T17 check reports secondary-screen and session-end migration integration contract valid.
- T11 check confirms immersive menu v1 integration remains valid after T17 additions.

Current result (2026-02-23): pass
