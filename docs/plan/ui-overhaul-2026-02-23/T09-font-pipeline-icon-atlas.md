# T09 — Font Pipeline and Iconography Atlas Integration

Status: implemented  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T09`  
Date: 2026-02-23

## Scope delivered
- Added font hierarchy and fallback chain definitions in `src/ui_style_tokens.h`:
  - role-based font chains (`UI_FONT_ROLE_DISPLAY`, `HEADING`, `BODY`, `META`, `ICON_FALLBACK`)
  - fallback coverage ordering (`primary` → latin fallback → CJK fallback → symbols fallback)
  - role-specific readable scale boundaries via `uiClampTypographyScale(...)`
- Added icon atlas integration scaffold in `src/ui_primitives.h`:
  - atlas spec/state (`UiIconAtlasSpec`, `UiIconAtlasState`)
  - runtime initialization path (`uiInitIconAtlasScaffold()`)
  - scalable icon draw entry (`uiPrimitiveIcon(...)`) with readable-size clamping
- Integrated scaffold into the existing T08 sample path without replacing the legacy text pipeline:
  - text rendering still routes through `drawText`/`drawTextCentered`
  - icon rendering uses deterministic glyph fallback while atlas texture is not bound

## Font loading and fallback rules
- Current runtime font contract is role-based and non-breaking:
  - UI text requests a role and scale.
  - Scale is clamped to readability bounds for that role.
  - Draw calls continue through the existing immediate text functions.
- Fallback order for each role is explicit and deterministic:
  - Display/Heading/Body/Meta prioritize game fonts, then `Noto Sans`, then CJK/symbol coverage.
  - Icon fallback role prioritizes symbol-oriented coverage.
- This establishes language-coverage intent while keeping renderer behavior stable until asset loaders are added.

## Icon atlas scaffold rules
- Atlas descriptor defaults:
  - path: `assets/ui/icons/icon_atlas_01.png`
  - grid: `8x8`
  - SDF softness/padding from tokenized defaults (`UiIconography`)
- Runtime toggle:
  - `BR_UI_ICON_ATLAS=1` enables atlas path intent.
  - Until texture plumbing is bound, the draw path remains readable via fallback icon glyphs.
- Scalable rendering guardrails:
  - icon width is clamped between `UiIconography::kMinReadableNdc` and `kMaxReadableNdc`.

## Runtime integration touchpoints
- `drawUiPrimitiveMainMenuSample(...)` now demonstrates icon path usage with two icon draws while preserving all T08 primitive behavior.
- `drawNewUI()` integration contract in `src/hud.h` remains unchanged (menu still routes into the primitive sample).

## Targeted validation
- Existing guard retained:
  - `powershell -ExecutionPolicy Bypass -File tools/check_t08_integration.ps1`
- New T09 guard:
  - `powershell -ExecutionPolicy Bypass -File tools/check_t09_integration.ps1`
- Pass criteria:
  - font chain + readable clamp APIs present
  - icon atlas scaffold + icon primitive draw path present
  - sample screen uses icon primitive without breaking the T08 menu integration route
