# T04 — UI Style Token System

Status: implemented  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T04`  
Date: 2026-02-23

## Canonical token source
- Runtime token definitions live in `src/ui_style_tokens.h`.
- Tokens are grouped by primitive class to avoid ad-hoc literals in new UI examples:
  - `UiSpacing` — layout spacing and offsets.
  - `UiTypography` — text scale hierarchy.
  - `UiColor` — contrast-safe text and overlay colors.
  - `UiDepth` — opacity/state emphasis values.
  - `UiMotion` — timing and pulse-rate constants.

## Token coverage summary
- Spacing: menu row step, chevron gap, title drift offset.
- Typography: title, subtitle, menu item, hint, and meta sizes.
- Colors: primary/secondary/muted/hint text plus neutral/warm overlays.
- State contract (explicit): default/warning/critical/disabled color + alpha tokens.
- Depth: selected/idle emphasis plus overlay/hint/meta/menu-state alpha values.
- Motion: fast/normal/slow timing and title/ambient animation rates.

## Example integration (proof of no ad-hoc literals in new examples)
- `drawMenu(float tm)` in `src/menu.h` now consumes tokens for:
  - title drift + pulse,
  - menu item spacing/scale/emphasis,
  - overlay tint layers,
  - hint/meta typography and color,
  - subtitle alpha and atmosphere edge bands/alpha.
- This function runs through the migration parity path (`drawUiParityPath()`), so the tokenized implementation is immediately usable by the current/new UI route without introducing one-off style literals.

## Notes for follow-up UI tasks
- Prefer extending `src/ui_style_tokens.h` first when adding style values.
- New UI screens should reference token namespaces directly instead of embedding raw constants.
