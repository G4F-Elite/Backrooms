# T13 Audio QA — High-Intensity Clarity Checklist

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T13`  
Date: 2026-02-23

## Purpose
Provide independent, reproducible QA evidence for the success criterion: **"Audio QA confirms clarity in high-intensity scenes."**

## Preconditions
- Build includes current T13 audio integration (`src/audio.h`, `src/input_menu.h`, `src/input_settings.h`).
- Tester can trigger menu navigation/adjust/confirm/back while danger/scare/death pressure is active.
- Master, ambience, SFX, and music sliders are at default values.

## Reproducible scenarios and pass/fail cases

| Case ID | Setup / Trigger | Expected PASS behavior | FAIL condition |
|---|---|---|---|
| HI-01 | Enter high-danger gameplay moment (high menace/proximity), open menu, repeatedly navigate options for 10s. | Navigate cue remains audible and distinct; ambience/music reduce subtly; gameplay SFX remain intelligible. | Navigate cue is masked, or ambience/music do not duck at all, or SFX collapse unnaturally. |
| HI-02 | In same high-danger moment, adjust slider repeatedly for 10s. | Adjust cue is clearly distinguishable from navigate cue and remains audible under pressure. | Adjust cue sounds identical to navigate/confirm or is frequently inaudible. |
| HI-03 | While high-danger is active, trigger confirm/apply and back/cancel alternating 10 times. | Confirm and back cues are perceptibly different; both remain clear over tension bed. | Confirm/back are not distinguishable, or one is consistently buried. |
| HI-04 | During active scare/death pressure, spam menu navigation/confirm/back for 5s. | UI cues remain audible but ducking stays capped so scare/death dominance is preserved. | UI cues overpower scare/death content or no cap-like behavior is observed. |
| HI-05 | Exit pressure state to normal intensity, perform same actions again. | Ducking reduces versus high-intensity state; UI remains responsive without over-attenuation of ambience/music. | Ducking strength appears unchanged across normal vs high-intensity states. |

## Targeted automated checks
Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t13_integration.ps1
powershell -ExecutionPolicy Bypass -File tools/check_t13_high_intensity_hooks.ps1
```

Pass condition:
- Both scripts exit `0` and print their `[PASS]` lines.

Fail condition:
- Any script exits non-zero and reports missing contract fragments.

## Execution record (2026-02-23)
- `check_t13_integration.ps1`: pass (`[PASS] T13 UI audio feedback + mix rules integration contract validated`)
- `check_t13_high_intensity_hooks.ps1`: pass (`[PASS] T13 high-intensity audio cue + ducking hook contract validated`)
- Manual listening checklist (`HI-01`..`HI-05`): pending QA sign-off

## QA sign-off block
- Tester: ____________________
- Build/Commit: ____________________
- Date/Time: ____________________
- Result: ☐ PASS  ☐ FAIL
- Notes (required on FAIL): _____________________________________________
