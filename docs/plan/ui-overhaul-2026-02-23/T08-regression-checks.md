# T08 Regression Risk Checks

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T08`  
Date: 2026-02-23

## Purpose
Capture explicit T08 quality and regression-risk validation so downstream UI work can detect accidental breakage early.

## Validation scope (T08-only)
1. **Integration path**
   - `drawNewUI()` in `src/hud.h` routes `STATE_MENU` to `drawUiPrimitiveMainMenuSample(vhsTime)`.
   - Branch returns before parity fallback to avoid mixed rendering in menu state.

2. **Primitives-only sample composition**
   - Sample remains built from T08 primitives (panel, button, input, list, modal, tooltip, status indicator) without one-off custom widgets.

3. **Baseline continuity**
   - `UiPrimitivePerfBaseline` in `src/ui_primitives.h` continues to expose counters and CPU timing used by the sample indicator.
   - In-screen baseline row continues to report `P/B/I/L/M/T/S`, `R`, `TXT`, and `CPU`.

## Regression risks covered
- Menu integration drift back to parity path.
- Silent introduction of non-primitive one-off widgets in the sample.
- Baseline signal loss that blocks T15 optimization comparisons.

## Targeted guard check
Command:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t08_integration.ps1
```

Pass condition:
- Script exits `0` and prints `[PASS] T08 integration contract validated in src/hud.h`.

Fail condition:
- Script exits non-zero with a message describing the missing integration contract fragment.

## Current result (2026-02-23)
- Status: pass