# T08 — UI Foundation Layer and Primitives

Status: implemented  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T08`  
Date: 2026-02-23

## Scope delivered
- Added reusable primitives in `src/ui_primitives.h`:
  - panel (`drawUiPanelPrimitive`)
  - button (`drawUiButtonPrimitive`)
  - input (`drawUiInputPrimitive`)
  - list (`drawUiListPrimitive`)
  - modal (`drawUiModalPrimitive`)
  - tooltip (`drawUiTooltipPrimitive`)
  - status indicator (`drawUiStatusIndicatorPrimitive`)
- Primitives are token-aligned using `UiColor`, `UiDepth`, `UiTypography`, and `UiMotion` from `src/ui_style_tokens.h`.
- Theming support is built in with `UiPrimitiveTheme` and runtime variant selection (`BR_UI_THEME=high-contrast`).

## Runtime integration
- New UI runtime path now renders a primitives-only sample menu screen when `gameState == STATE_MENU`.
- Integration point: `drawNewUI()` in `src/hud.h`.
- All other UI states remain on the existing parity path to keep migration risk low and changes minimal.

## Sample screen composition (primitives only)
The sample menu screen is built from primitives only and includes:
- 1 root panel
- 2 status indicators
- 1 input
- 1 list
- 2 buttons
- 1 tooltip
- 1 modal
- 1 performance status indicator

No one-off custom widgets are used for this screen.

## Primitive performance baseline
Baseline is measured every frame by `UiPrimitivePerfBaseline` in `src/ui_primitives.h`.

Measured counters captured for the sample screen:
- primitive instance counts (`panels`, `buttons`, `inputs`, `lists`, `modals`, `tooltips`, `indicators`)
- render pass counters (`rectPasses`, `textPasses`)
- CPU frame timing for primitive rendering (`cpuMsLast`, `cpuMsAvg`, `cpuMsPeak`)

The active baseline is surfaced in-screen via the final status indicator row:
- `P/B/I/L/M/T/S` primitive counts
- `R` rectangle passes
- `TXT` text passes
- `CPU` primitive render CPU time (ms)

This baseline is deterministic for the static sample layout and serves as the initial T08 performance reference for follow-up optimization work (T15).

## T08 quality + regression risk checks
Artifact: `docs/plan/ui-overhaul-2026-02-23/T08-regression-checks.md`

Validation scope for T08 is explicitly constrained to:
- integration path stability (menu route still enters primitives sample via `drawNewUI()`)
- primitives-only composition integrity for the sample screen
- baseline metric continuity (`P/B/I/L/M/T/S`, `R`, `TXT`, `CPU`)

Targeted automated guard tied to integration:
- `tools/check_t08_integration.ps1`
- Asserts `drawNewUI()` keeps the T08 menu integration contract:
  - `STATE_MENU` branch calls `drawUiPrimitiveMainMenuSample(vhsTime)`
  - branch exits early (`return`) before parity fallback

Validation status on 2026-02-23:
- Guard check result: pass
- Scope reviewed against implemented T08 primitives/sample/baseline: pass
