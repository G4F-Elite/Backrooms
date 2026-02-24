# T14 Accessibility Baseline

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T14`  
Date: 2026-02-23

## Scope implemented
- Contrast baseline guard for core UI token pairs (`4.5:1` text on overlay, `3.0:1` warning/focus on overlay).
- Reduced motion mode for new UI surfaces via settings (`VIDEO -> REDUCED MOTION`) and shared motion helpers.
- Keyboard/controller navigation parity kept for critical menu/settings/multiplayer screens.

## Runtime behavior
- `Reduced Motion = ON` removes non-essential menu drift/pulse/ambient transition behavior.
- Focus visibility is strengthened in new primitives via explicit high-contrast focus outlines on active controls.
- If token contrast baseline drifts below thresholds, primitive theme falls back to a safer high-contrast profile.

## Targeted validation
Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t14_integration.ps1
```

Pass condition:
- Script exits `0` and prints a `[PASS]` line with computed contrast ratios.
