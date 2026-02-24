# T12P Provisional Contextual Feedback Spike (non-release)

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T12P`  
Date: 2026-02-23

## Scope implemented
- Added explicit provisional gating in `src/ui_migration_toggle.h`:
  - Compile-time gate: `BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX` (default `0`).
  - Runtime gate: `BR_UI_T12P_CONTEXTUAL_FX=1|true|on`.
  - Migration gate: spike is active only when `useNewUiMigrationPath()` is active.
- Added a focused immersive HUD spike in `src/hud.h`:
  - `drawProvisionalContextualFeedbackSpike(...)` renders a small pressure indicator band + status line.
  - Signal blends health/sanity pressure plus downed/network instability critical escalation.
- Added targeted guard script: `tools/check_t12p_integration.ps1`.

## Non-release/release behavior
- Default behavior remains unchanged across existing build profiles.
- `BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX` defaults to `0`, so production/release artifacts are default-off unless explicitly compiled otherwise.
- Even in non-release builds where compile gate is enabled, runtime still requires explicit opt-in via `BR_UI_T12P_CONTEXTUAL_FX`.

## Perf/readability observations for T12 reconciliation
- Draw cost impact is intentionally low (single status indicator + thin accent rect per immersive HUD frame when enabled).
- No additional texture/font assets were introduced; effect reuses existing primitive/theme pipeline.
- Readability remains bounded by existing primitive text hierarchy and tone tokens; spike text is short and uses existing warning/critical/default tones.
- Placement was chosen near prompt/scanner zone to stay contextual while avoiding objective/event panel overlap.
- Reduced-motion behavior is respected indirectly by avoiding continuous animated transforms in this spike implementation.

## Open risks/questions for T05A stakeholder review
- Motion language alignment: should pressure signaling stay static (current) or adopt approved pulse cadence post-T05A?
- Escalation semantics: is network instability intended to contribute to the same pressure channel as health/sanity, or should it remain separate?
- Text label policy: should the final release path keep explicit `PRESSURE` text, switch to iconographic cues, or both?
- Screen density: if scanner + prompt + pressure signals coincide, do we need a priority/stacking rule before T12 release integration?
- Acceptance threshold: define a concrete “readable and non-obscuring” threshold for T12 sign-off (for example, max occupied lower-right HUD area over time).

## Targeted validation
Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t12p_integration.ps1
powershell -ExecutionPolicy Bypass -File tools/check_t10_integration.ps1
```

Pass condition:
- Both scripts exit `0` and print `[PASS]`.
