# T13 UI audio feedback and mix rules

## Scope
Implements task `T13` for `ui-overhaul-2026-02-23` by adding distinct UI action cues and adaptive mix ducking so UI interactions remain legible without flattening the horror soundscape.

## Distinct UI action cues
Primary UI actions now use distinct synthesized cue families in `src/audio.h`:

- Navigate: `triggerMenuNavigateSound()` (`uiMove` envelope, short high-mid chirp)
- Adjust: `triggerMenuAdjustSound()` (`uiAdjust` envelope, shorter knob-step pulse)
- Confirm/apply: `triggerMenuConfirmSound()` (`uiConfirm` envelope, descending confirm tone)
- Back/cancel: `triggerMenuBackSound()` (`uiBack` envelope, lower retreat tone)

Back/cancel routes are wired in:

- `src/input_menu.h` for `ESC`/back navigation and explicit Back selections
- `src/input_settings.h` for settings back and numeric entry cancel

## Mix ducking and priority rules
Implemented in `src/audio.h` in the main mixer path before final bus sum.

Priority stack (highest first):

1. Death/scare critical cues
2. UI confirm/back cues
3. UI adjust cues
4. UI navigate cues
5. Ambient/music bed

Implemented rules:

- UI events raise a short `uiPriorityEnv` (different weights by cue class).
- `uiDuckStrength` scales with both UI priority and scene intensity (`monsterProximity`, `monsterMenace`, `dangerLevel`).
- Ducking attenuates ambience/music more than SFX to keep gameplay readability while preserving event texture.
- When scare/death is active, UI ducking is capped so critical horror events remain dominant.

## Audio QA clarity validation evidence
Independent QA artifact:

- `docs/plan/ui-overhaul-2026-02-23/T13-audio-qa-high-intensity-checklist.md`

Validation commands:

- `powershell -ExecutionPolicy Bypass -File tools/check_t13_integration.ps1`
- `powershell -ExecutionPolicy Bypass -File tools/check_t13_high_intensity_hooks.ps1`

Expected evidence criteria from checks:

- Distinct cue contract present (`navigate`, `adjust`, `confirm`, `back`)
- UI mix includes back/cancel cue path
- Ducking + priority fragments exist in mixer path
- Menu/settings cancel routes call back cue
- High-intensity hook contract exists for scene-intensity scaling and scare/death ducking cap
- Reproducible QA checklist includes concrete high-intensity pass/fail cases (`HI-01` .. `HI-05`)

Result (2026-02-23):

- `[PASS] T13 UI audio feedback + mix rules integration contract validated`
- `[PASS] T13 high-intensity audio cue + ducking hook contract validated`
- Manual high-intensity listening checklist (`HI-01`..`HI-05`) is prepared and pending QA sign-off.
