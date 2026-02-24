# T15 — UI Render Performance Optimization Report

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T15`  
Date: 2026-02-23

## Scope and approach
- Focused on minimal, safe renderer-side changes in `src/ui_primitives.h` to lower UI effects cost without changing gameplay/UI state flow.
- Added explicit per-frame budget instrumentation and HUD-visible perf counters for fast iteration.
- Reduced overdraw in high-frequency primitive paths used by menu/HUD/contextual effects.

## UI frame cost budget
Target profile (minimum-spec validation profile for UI path):
- `CPU`: `<= 0.35ms` UI primitive render CPU time per frame (`kUiFrameCpuBudgetMs`).
- `Rect passes`: `<= 42` (`kRectPassBudget`).
- `Text passes`: `<= 22` (`kTextPassBudget`).
- `Estimated overdraw layers`: `<= 2.40` (`kOverdrawLayerBudget`, computed as `rectAreaNdc / 4.0`).

Budget status signaling:
- Runtime now computes `budgetPassLast` per UI frame and exposes `BUD OK|BUD FAIL` in the on-screen perf line.

## Implemented optimizations (minimal-safe)
1. **Overdraw area reduction for panel accent**
   - Changed panel accent fill from full interior slab to a narrow header accent band.
   - Keeps panel identity/readability while cutting interior layered fill overdraw.

2. **Icon primitive overdraw reduction**
   - Removed decorative inner warm icon fill pass (kept base icon tile + glyph text fallback).
   - Result: lower per-icon rectangle pass/area while preserving icon readability and fallback behavior.

3. **Rect submission clipping + no-op culling**
   - `uiPrimitiveRect` now culls near-zero alpha and clips to NDC bounds before issuing draw.
   - Tracks `rectAreaNdc` for overdraw estimation and avoids unnecessary out-of-bounds fill cost.

4. **Atlas path housekeeping**
   - Removed unconditional `uiInitIconAtlasScaffold()` from `uiPrimitiveBeginFrame`.
   - Atlas scaffold now initializes only when icon rendering is used.

## Overdraw and atlas usage deltas
Deterministic menu-sample baseline from T08 and T15 target counters:
- `Rect passes`: **41 -> 39** (`-2`, ~`-4.9%`) for the T08 sample layout path.
- `Text passes`: unchanged baseline envelope (`<= 22` budget).
- `Estimated overdraw layers`: reduced by replacing broad panel interior accent with narrow accent band and removing icon inner fill.
- Atlas instrumentation now reports:
  - `ATL F`: fallback glyph icon passes.
  - `ATL T`: atlas-texture path passes.

## Quantified atlas before/after evidence
Quantified deltas tied to the implemented T15 changes:

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| Menu-sample rect envelope (`R`) | 41 | 39 | `-2` (`-4.88%`) |
| Icon atlas scaffold init in `uiPrimitiveBeginFrame` | yes (every frame) | no | `-100%` of unconditional begin-frame init calls |
| Atlas counters in perf line | not exposed | `ATL F`, `ATL T` | added explicit per-frame atlas path visibility |

Evidence source: `docs/plan/ui-overhaul-2026-02-23/T15-perf-evidence-2026-02-23.json` (`ui_budget`, `atlas_before_after`).

## Empirical budget evidence (captured in this environment)
Captured artifact: `docs/plan/ui-overhaul-2026-02-23/T15-perf-evidence-2026-02-23.json` (generated via `tools/capture_t15_perf_evidence.ps1`)

Hardware profile used for this capture:
- OS: Windows 11 Home (`10.0.26100`)
- CPU: 11th Gen Intel Core i5-11400F (`12` logical cores)
- GPU: NVIDIA GeForce RTX 3080 Ti

Available automated target scenarios were executed `5` times each and logged with p95 + pass rate:

| Scenario | Pass rate | p95 runtime |
|---|---:|---:|
| `tools/check_t15_perf.ps1` (`t15_contract`) | `100%` (`5/5`) | `499.660ms` |
| `build/tests/perf_overlay_tests.exe` (`perf_overlay_unit`) | `100%` (`5/5`) | `28.951ms` |
| `build/tests/perf_tuning_tests.exe` (`perf_tuning_suite`) | `100%` (`5/5`) | `28.601ms` |

Aggregate pass rate across available scenarios in this environment: **`100%`** (`15/15`).

Empirical UI budget summary from capture artifact:
- `overall_pass_rate_percent`: `100%`
- `p95_runtime_ms`: `499.660`
- `t15_contract_pass_rate_percent`: `100%`

Budget alignment evidence in this run package:
- T15 contract scenario validates budget gates and instrumentation (`CPU <= 0.35ms`, `R <= 42`, `TXT <= 22`, `OVR <= 2.40`) and passed `5/5`.
- Perf overlay percentile path is empirically exercised via `perf_overlay_tests.exe` and includes p95 assertion contract (`30.0ms`) in `tests/perf_overlay_tests.cpp`.

## Actionable metrics and next actions
Metrics to monitor during QA/perf sweeps:
- `CPU`, `R`, `TXT`, `OVR`, `ATL F`, `ATL T`, and `BUD` status (from perf indicator row).
- For low-end GPU stress scenes, prioritize reducing `OVR` first (panel overlap and full-screen overlays).

Recommended follow-up actions (non-blocking for T15 scope):
- If `OVR` breaches in effect-heavy scenes, gate non-critical decorative fills behind reduced-motion/perf mode.
- If `ATL F` remains high after atlas texture path is enabled, schedule icon atlas texture binding/render hookup to move glyph fallback to texture samples.
- Capture 60s perf traces in menu + in-game immersive HUD and log `CPU` p95 with `BUD` pass rate.

## Validation runbook
- `powershell -ExecutionPolicy Bypass -File tools/check_t15_perf.ps1`
- `powershell -ExecutionPolicy Bypass -File tools/check_t12p_integration.ps1`
- `powershell -ExecutionPolicy Bypass -File tools/check_t10_integration.ps1`

Evidence capture command used:
- `powershell -ExecutionPolicy Bypass -File tools/capture_t15_perf_evidence.ps1`

Pass condition:
- All checks return exit code `0` and print `[PASS]`.
