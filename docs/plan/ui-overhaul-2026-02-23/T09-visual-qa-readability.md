# T09 Visual QA — Readability and Icon Legibility

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T09`  
Date: 2026-02-23

## Purpose
Provide a lightweight QA artifact confirming typography and icon fallback remain readable at gameplay-relevant sizes while the atlas path is scaffolded.

## Test matrix
| Scenario | Resolution | UI Path | Expected readability outcome |
|---|---:|---|---|
| Menu sample baseline | 1920x1080 | New UI (`drawUiPrimitiveMainMenuSample`) | Title, list labels, status lines, and icon glyphs all readable at normal viewing distance |
| Lower render density | 1280x720 | New UI | Meta/hint text remains legible; icon glyphs remain identifiable |
| Stress readability floor | 854x480 | New UI | Clamp prevents tiny icon rendering; critical labels remain distinguishable |
| Legacy parity reference | 1920x1080 | Legacy UI path | Existing text rendering remains unchanged |

## Readability checks
1. **Typography hierarchy**
   - Display/heading/body/meta text appears in clear visual order.
   - Small metadata text remains readable and does not collapse below role min scale.

2. **Fallback resilience**
   - If atlas texture is unavailable, icon slots render fallback glyphs (`#`, `~`, `!`, `+`) instead of blank content.
   - Fallback rendering uses existing text pipeline, preserving stability.

3. **Icon scalability**
   - Icon width respects min/max readable bounds to avoid unreadable tiny marks or oversized blockers.

4. **Contrast sanity pass**
   - Icon foreground and nearby labels stay distinguishable against panel backgrounds in default and high-contrast theme variants.

## Execution notes
- Use migration toggle path already in place for T08/T09.
- For scaffold behavior verification, run once with and without `BR_UI_ICON_ATLAS=1`.
- Record observations in this file or in follow-up QA log if any readability regression appears.

## Current result (2026-02-23)
- Status: pass (scaffold-level validation)
- No breakage observed in menu integration route or text fallback readability contract.
