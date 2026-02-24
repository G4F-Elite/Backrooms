# T05 — Interaction and Motion Guidelines

Status: ready_for_review  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T05`  
Date: 2026-02-23

## 1) Purpose and scope
- Define a shared interaction-motion language for HUD, menus, overlays, and alerts.
- Keep immersion high while preserving low-distraction, survival-first readability.
- Provide implementation-ready rules for design and engineering with explicit interruption behavior.

## 2) Alignment with existing token system

Canonical timing source: `src/ui_style_tokens.h` (`UiMotion` namespace).

| Token | Value | Usage intent |
|---|---:|---|
| `UiMotion::kPulseFastSec` | 0.16s | Fast feedback (hover/focus, small state changes) |
| `UiMotion::kPulseNormalSec` | 0.26s | Standard transitions (panel/overlay enter, emphasis shifts) |
| `UiMotion::kPulseSlowSec` | 0.40s | Slow ambient motion (non-critical breathing/drift) |
| `UiMotion::kPulseTitleRate` | 2.0 Hz | Rhythmic pulse source for title/hero text only |
| `UiMotion::kDriftAmbientRate` | 0.7 Hz | Background atmospheric drift source |

Rules:
- Do not hard-code ad-hoc duration values in new UI paths.
- Prefer token-direct durations (`fast`, `normal`, `slow`).
- If a shorter action is needed (e.g., click compression), derive from token multipliers (`0.5 * kPulseFastSec`) and keep derivation explicit.

## 3) Easing contract

Use exactly these easing curves across new UI implementation:

| Easing name | Curve | Intended use |
|---|---|---|
| `ui-ease-out` | `cubic-bezier(0.22, 1.00, 0.36, 1.00)` | Enter, reveal, focus gain, tooltip fade-in |
| `ui-ease-in-out` | `cubic-bezier(0.40, 0.00, 0.20, 1.00)` | Neutral transitions between stable states |
| `ui-ease-in` | `cubic-bezier(0.55, 0.00, 1.00, 0.45)` | Exit/de-emphasis/fade-out |
| `ui-alert-snap` | `cubic-bezier(0.20, 0.90, 0.30, 1.20)` | Critical alert onset only (brief overshoot allowed) |

Constraints:
- Overshoot is allowed only on critical alert onset and must never move core HUD anchors.
- Ambient loops use sinusoidal modulation over token rates; no additional bespoke easing functions.

## 4) Interaction timing matrix

| Interaction | Duration | Easing | Notes |
|---|---:|---|---|
| Hover/focus gain | `kPulseFastSec` | `ui-ease-out` | High responsiveness; must remain readable in low light |
| Hover/focus loss | `kPulseFastSec` | `ui-ease-in` | Avoid lingering motion tails |
| Button press/compress | `0.5 * kPulseFastSec` | `ui-ease-in-out` | Immediate tactile response |
| Button release/settle | `0.5 * kPulseFastSec` | `ui-ease-out` | Return to idle without bounce |
| Panel/overlay enter | `kPulseNormalSec` | `ui-ease-out` | Menus, pause, objective cards |
| Panel/overlay exit | `kPulseFastSec` | `ui-ease-in` | Exit faster than enter to reduce occlusion time |
| Tooltip/hint reveal | `kPulseFastSec` | `ui-ease-out` | Text must remain crisp, no jitter |
| Tooltip/hint dismiss | `kPulseFastSec` | `ui-ease-in` | Keep cognitive interruption brief |
| Damage feedback flash | `kPulseFastSec` | `ui-ease-out` | One-shot event, no persistent oscillation |
| Critical alert onset | `kPulseFastSec` | `ui-alert-snap` | Allowed only for warning shell, not text baseline |
| Critical alert sustain pulse | `kPulseNormalSec` cycle | sinusoid at `kPulseTitleRate` max | Must clamp amplitude (see §6) |
| Ambient non-critical drift | `kPulseSlowSec` cycle | sinusoid at `kDriftAmbientRate` | Background only |

## 5) Interruption and priority rules

Priority stack (highest to lowest):
1. Survival-critical alerts (death risk, immediate threat, critical health/sanity).
2. Damage/state-change feedback.
3. Objective and interaction prompts.
4. Ambient atmospheric motion.

Interruption behavior:
- Higher-priority motion may preempt lower-priority motion immediately.
- Preempted animations must snap to the nearest stable readable state within `<= 0.5 * kPulseFastSec`.
- Never queue more than one deferred animation per widget; drop stale deferred transitions.
- Re-triggering the same event while active should restart from current value, not from initial keyframe.
- During sustained critical alerts, suppress non-essential ambient loops on affected HUD region.

Determinism and net consistency:
- Trigger motion from deterministic UI state transitions, not frame-time accumulation drift.
- Host/client UI event ordering must map to same priority rules and interruption outcomes.

## 6) Low-distraction gameplay usability constraints

Readability-first constraints:
- Survival-critical information must remain identifiable in under 500ms during chase/high-stress moments.
- No full-screen flash behavior for UI-only events.
- Core HUD anchors (health, sanity, objective) must not translate from their baseline positions under motion effects.

Motion budget constraints:
- Maximum concurrent animated UI regions during gameplay: 2 non-critical + 1 critical channel.
- Non-critical ambient alpha modulation amplitude cap: `<= 0.06` (aligned with `UiDepth::kAlphaTitlePulse`).
- Critical pulse amplitude may exceed ambient but cannot reduce text/indicator contrast below required thresholds.

Input and control constraints:
- Focus movement feedback must complete within `kPulseFastSec`.
- Menu navigation should remain immediately interruptible by cancel/back and mode switches.
- Pointer and keyboard/controller interactions must use equivalent timing categories.

Reduced motion baseline:
- Reduced motion mode disables ambient drift loops and non-essential pulse effects.
- Essential state transitions remain, but use `kPulseFastSec` with no overshoot easing.
- Critical alerts retain color/state change and optional minimal alpha pulse only.

## 7) Implementation checklist (design + engineering)

Design checklist:
- Every motion behavior maps to a token duration category and approved easing name.
- Critical alert behaviors are clearly distinct from ambient/ornamental behaviors.
- Mockups include one low-light frame and one high-motion frame with readable anchors.

Engineering checklist:
- New UI code references `UiMotion` token constants (or explicit token-derived multipliers).
- Interruption handling follows the priority/preemption rules in §5.
- Reduced motion path is implemented and verified for all new HUD/menu interactions.

Validation checklist:
- No hard-coded one-off timing constants in migrated UI paths.
- Motion remains legible and non-obstructive in gameplay stress scenarios.
- Keyboard/controller parity maintained for focus and transition feedback.

## 8) Sign-off block

| Role | Name | Decision | Date | Notes |
|---|---|---|---|---|
| UX Designer |  | Approve / Request changes |  |  |
| UI Engineer |  | Approve / Request changes |  |  |
| Gameplay Engineer |  | Approve / Request changes |  |  |
| Accessibility Specialist |  | Approve / Request changes |  |  |
