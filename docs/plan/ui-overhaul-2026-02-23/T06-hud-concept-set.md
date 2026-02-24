# T06 — High-Fidelity HUD Concept Set

Status: ready_for_review  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T06`  
Date: `2026-02-23`

## 1) Deliverable scope
- Provide high-fidelity HUD concepts for three gameplay scenarios: exploration, combat, and critical-state.
- Cover required HUD domains: health, sanity, objectives, minimap.
- Validate visual hierarchy in low-light and high-motion frames.
- Select one final concept for implementation handoff.

## 2) Shared constraints (from T04/T05)
- Use style/motion tokens from `src/ui_style_tokens.h`; no ad-hoc constants in implementation.
- Priority order must remain: survival-critical > damage/state-change > objective/prompt > ambient.
- Core HUD anchors do not translate under motion effects.
- Reduced-motion path disables ambient drift and non-essential pulses.

## 3) Concept set

### Concept A — "Peripheral Bracket"
**Intent:** Keep center screen clear while wrapping critical data into peripheral corner brackets.

**Composition**
- Bottom-left: health + sanity dual-stack with compact icon + value + segmented fill.
- Top-left: objective card (single-line primary objective, optional sub-line).
- Top-right: minimap circular ring + heading marker.
- Bottom-right: contextual prompt slot and transient status notices.

**Visual treatment**
- Uses `UiColor::kStateDefault`, `kStateWarning`, `kStateCritical` mapped by threshold state.
- Overlay uses `UiColor::kOverlayBase` + `UiColor::kOverlayWarm` with `UiDepth::kAlphaOverlayBase` and `kAlphaOverlayWarm`.
- Critical shell pulse only on outer bracket stroke (not on numeric text baseline).

**Motion behavior**
- Focus/hover emphasis: `UiMotion::kPulseFastSec`.
- Objective update reveal: `UiMotion::kPulseNormalSec` with `ui-ease-out`.
- Critical onset: `UiMotion::kPulseFastSec` with `ui-alert-snap`; sustain pulse clamped to T05 amplitude constraints.

**Strengths**
- Strong center-screen preservation for navigation/combat.
- Clear left-right separation between survivability and orientation info.

**Risks**
- Minimap/top-right can be missed during frantic lateral movement.
- Dual-stack health/sanity may feel dense at smaller resolutions.

---

### Concept B — "Split Spine"
**Intent:** Create a vertical left "spine" for survivability and a right "intel" rail for navigation/context.

**Composition**
- Left vertical spine (mid-left): health over sanity in large, glanceable bars with numeric core.
- Right vertical rail (mid-right): minimap top, objective block center, prompt/status block lower.
- Bottom center: small direction/stance strip only when relevant.

**Visual treatment**
- Larger typography for survivability values using `UiTypography::kScaleMenuItem` equivalent in gameplay HUD scale mapping.
- Secondary metadata uses `UiTypography::kScaleHint` and `kScaleMeta` mapping.
- Uses edge glow only for state transitions, otherwise matte containers.

**Motion behavior**
- Rail modules fade/slide within-place (`kPulseFastSec`/`kPulseNormalSec`) without anchor shift.
- Damage flash constrained to left spine container.
- Non-critical ambient drift only on rail backgrounds at `UiMotion::kDriftAmbientRate`.

**Strengths**
- Fast survivability reads due to centralized left-side stack.
- Scales well for co-op additions (teammate chips can append below).

**Risks**
- Right rail density can compete with world-space interact prompts.
- Vertical rails occupy more lateral field in narrow FOV setups.

---

### Concept C — "Ring + Anchors"
**Intent:** Hybrid cinematic HUD with a compact center-bottom status ring and minimal corner anchors.

**Composition**
- Center-bottom ring: combined health/sanity radial split with inner numeric readout.
- Top-left: objective micro-card.
- Top-right: minimap reduced to directional wedge + ping dots.
- Bottom-right: contextual prompt.

**Visual treatment**
- High atmospheric quality with analog ring noise and subtle chroma separation.
- State color mapping same as T04 token contract.

**Motion behavior**
- Ring breathes on `UiMotion::kPulseSlowSec`; critical overlays switch to `kPulseFastSec` shell pulse.
- Prompt transitions remain fast and deterministic.

**Strengths**
- Highest visual identity and mood coherence.
- Immediate focal survivability read when eyes already track lower center.

**Risks**
- Center-bottom element can conflict with close-quarters floor hazard awareness.
- Radial split is harder for rapid differential read (health vs sanity) in panic.

## 4) Scenario boards (implementation framing)

### Exploration scenario (low threat, route planning)
**Primary goals**
- Keep objective and minimap legible without clutter.
- Preserve atmosphere through subtle ambient motion only.

**HUD state requirements**
- Health/sanity in default state (`UiColor::kStateDefault`, `UiDepth::kStateDefaultAlpha`).
- Objective card persistent, one-line title + optional breadcrumb/meta.
- Minimap always-on with low visual weight; interaction prompts transient.

**Acceptance checks**
- Objective text parseable in <= 1.0s glance.
- Prompt appearance/dismissal never obscures objective line.

### Combat scenario (active threat, high camera motion)
**Primary goals**
- Sub-500ms survivability read (health/sanity + immediate alert).
- Minimize motion noise while preserving critical feedback.

**HUD state requirements**
- Health/sanity escalation to warning color tokens at thresholds.
- Damage feedback uses one-shot fast pulse; no looping flash.
- Objective block de-emphasized but still visible.

**Acceptance checks**
- During sprint + turn movement, survivability values remain distinguishable by both shape and text.
- Concurrent animation channels respect T05 budget (<= 2 non-critical + 1 critical).

### Critical-state scenario (near-failure / panic)
**Primary goals**
- Survival-critical clarity dominates all other UI information.
- Remove non-essential ambient motion.

**HUD state requirements**
- Critical color + alpha tokens applied to survivability module only.
- Objective/minimap dim but present; prompts restricted to life-saving actions.
- Critical onset uses `ui-alert-snap`; sustain pulse clamped to preserve contrast.

**Acceptance checks**
- First readable signal is survivability module in <= 500ms.
- No critical text baseline jitter or displacement.

## 5) Visual hierarchy validation matrix

| Test frame | Lighting / motion | Expected first read | Expected second read | Result by concept |
|---|---|---|---|---|
| V1 | Low-light corridor / idle walk | Health + sanity | Objective | A: pass, B: pass, C: pass |
| V2 | Low-light + flicker / objective update | Objective update badge | Minimap heading | A: pass, B: pass, C: borderline (ring draws eye) |
| V3 | High-motion chase / 120° camera sweep | Health state color+value | Sanity state | A: pass, B: pass, C: borderline (radial split slower) |
| V4 | Damage intake burst / combat | Damage alert shell | Prompt action | A: pass, B: pass, C: pass |
| V5 | Critical near-failure / motion blur | Critical survivability | Escape prompt | A: pass, B: pass, C: borderline (center-bottom competition) |

Validation notes:
- A and B satisfy hierarchy goals consistently across low-light/high-motion frames.
- C meets mood goals but underperforms readability in V2/V3/V5 under panic conditions.

## 6) Final concept selection

### Selected concept: **Concept A — Peripheral Bracket**

**Rationale**
1. Best balance of immersion and survival readability under high motion.
2. Strong anchor separation reduces cognitive conflict: survivability (left), orientation (right), intent (top-left).
3. Lowest implementation risk relative to existing HUD render structure (corner-anchored modules map cleanly to current draw flow).
4. Easiest to harden for accessibility and reduced-motion with minimal structural change.

## 7) Engineer handoff (implementation-ready)

### Module map (proposed)
- `HudHealthSanityWidget` (bottom-left anchor)
- `HudObjectiveWidget` (top-left anchor)
- `HudMinimapWidget` (top-right anchor)
- `HudPromptWidget` (bottom-right anchor)
- `HudAlertChannel` (non-layout overlay channel; no anchor translation)

### Token mapping contract
- Typography: map module text tiers to `UiTypography` scales (`title/subtitle/menuItem/hint/meta` equivalents).
- Color/state: use `UiColor::kStateDefault|kStateWarning|kStateCritical|kStateDisabled`.
- Alpha/depth: use `UiDepth` state/overlay tokens; do not introduce extra opacity literals.
- Motion: all durations from `UiMotion`; easing names from T05.

### State thresholds (design contract for engineering)
- Default: health >= 60%, sanity >= 60%.
- Warning: health < 60% or sanity < 60%.
- Critical: health < 25% or sanity < 25%.
- If both are critical, prioritize lower absolute value for alert emphasis source.

### Event priority and interruption
- Follow T05 §5 exactly.
- Critical events preempt all non-essential animations.
- Preempted animations settle to readable state within `<= 0.5 * UiMotion::kPulseFastSec`.

### Reduced motion behavior
- Disable ambient drift and ornamental pulse.
- Keep essential state transitions at `UiMotion::kPulseFastSec` without overshoot.
- Preserve color/shape/text distinctions for warning/critical states.

## 8) Review checklist for T06 acceptance
- [x] Concepts delivered for exploration/combat/critical-state.
- [x] Hierarchy validated on low-light/high-motion frames.
- [x] Final concept selected for implementation.
- [x] Handoff includes token/motion/state contracts for engineers.

## 9) Sign-off block

| Role | Name | Decision | Date | Notes |
|---|---|---|---|---|
| UI Artist |  | Approve / Request changes |  |  |
| UI Engineer |  | Approve / Request changes |  |  |
| Gameplay Engineer |  | Approve / Request changes |  |  |
| Accessibility Specialist |  | Approve / Request changes |  |  |
