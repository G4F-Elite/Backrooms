# T01 — UI Vision, Principles, and Target Mood Board

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T01`  
Date: `2026-02-23`

## 1) Vision Brief (Signoff Template)

### Product Context
- Game: Backrooms VHS Horror (survival horror, procedural, no-asset heavy style).
- Core fantasy: survive inside an infinite, hostile, uncanny office maze where escape is not the goal.
- Emotional loop: confusion → awareness → pursuit → desperation.
- Critical systems to communicate at a glance: health, sanity, stamina/flashlight pressure, threat proximity, objective/prompt state, multiplayer cohesion.

### UI Vision Statement
Build an immersive, diegetic-leaning UI that feels like unstable analog instrumentation inside the Backrooms: readable under panic, oppressive but not noisy, and always prioritizing survival decisions over decoration.

### Experience Goals (What “good” looks like)
1. **Readable in motion and darkness:** player can identify survival-critical signals in under 500ms during chase moments.
2. **Tension without clutter:** visual language raises dread (flicker, instability, pressure cues) while preserving hierarchy.
3. **Guidance without handholding:** the UI directs immediate action (danger, pathing intent, interaction state) using subtle but clear prioritization.
4. **Multiplayer trust:** host/client players receive consistent state cues for danger, status, and session flow.

### Non-Goals (for scope control)
- Do not redesign core gameplay systems in T01.
- Do not commit to heavy cinematic overlays that obscure navigation.
- Do not remove legacy path until migration gates and fallback behavior are validated.

### Mood + Tone Keywords
- Oppressive
- Uncanny
- Analog/VHS degradation
- Clinical fluorescent fatigue
- Survival-first clarity

### Definition of Done for T01
- Vision accepted by game director and art lead.
- Mood board includes 3+ benchmark references for each major UI surface.
- Principles for readability, tension, player guidance documented and used by downstream tasks (T04/T05/T06/T07).

### Signoff Block
| Role | Name | Decision | Date | Notes |
|---|---|---|---|---|
| Game Director |  | Approve / Request changes |  |  |
| Art Lead |  | Approve / Request changes |  |  |
| UI/UX Lead |  | Approve / Request changes |  |  |

---

## 2) Target Mood Board References (Benchmarks)

> Each reference is a benchmark for **visual language and UX treatment**, not a direct visual copy.

### A) HUD (in-game survival HUD)
1. **Dead Space (Remake / UI language)**  
   - Use for: diegetic-feeling status urgency, health/stress communication under pressure.  
   - Apply to Backrooms: prioritize immediate survivability cues with minimal eye travel.
2. **Alien: Isolation (HUD + threat presence cues)**  
   - Use for: sparse HUD discipline, fear amplification through information restraint.  
   - Apply to Backrooms: keep threat cues precise; avoid over-informing player.
3. **GTFO (co-op combat readability)**  
   - Use for: team status legibility and tactical clarity in dark/high-noise scenes.  
   - Apply to Backrooms: clean co-op identity markers and shared danger readability.

### B) Menus (main/pause/navigation)
1. **Control (menu hierarchy + subtle motion)**  
   - Use for: modern, clean information architecture with atmospheric tone.
2. **The Outlast Trials (horror UI framing)**  
   - Use for: oppressive framing and unsettling visual cadence in menu contexts.
3. **SIGNALIS (minimalist horror UI discipline)**  
   - Use for: restrained typography and strong mood from sparse elements.

### C) Overlays (alerts, objectives, danger/sanity events)
1. **Amnesia: The Bunker (threat-forward feedback)**  
   - Use for: panic-state signaling without full-screen clutter.
2. **Escape from Tarkov (state-rich overlays)**  
   - Use for: layered situational info with strict hierarchy.
3. **Lethal Company (co-op communication clarity)**  
   - Use for: concise co-op cues and status communication under stress.

### D) Settings + Multiplayer flows
1. **Phasmophobia (horror co-op lobby flow)**  
   - Use for: straightforward host/join readiness and role clarity.
2. **Deep Rock Galactic (co-op session usability)**  
   - Use for: practical session setup, team state readability, error-recovery paths.
3. **Ready or Not (settings categorization + tactical clarity)**  
   - Use for: dense settings grouped for fast access and confidence.

---

## 3) Core Principles (Actionable)

### Readability Principles
1. **Critical-first hierarchy:** health, sanity, and immediate threat always outrank flavor effects.
2. **Distance-tested legibility:** all survival-critical text/icons must remain legible at gameplay camera distance and low light.
3. **State clarity over style:** each status change (safe, caution, critical) must be distinguishable without relying only on color.
4. **Input parity:** keyboard/mouse and controller navigation provide equivalent clarity and pace in menus/settings.

### Tension Principles
1. **Atmosphere in the margins:** apply VHS/noise/flicker primarily to containers/background layers, not core text glyphs.
2. **Escalation curve alignment:** UI stress effects intensify with gameplay phases (confusion→desperation), never abruptly spike without gameplay cause.
3. **Controlled instability:** motion and distortion cues should imply danger while preserving stable anchor points for key data.

### Player Guidance Principles
1. **Next-best-action signaling:** every high-risk state should indicate the player’s immediate best response (move, hide, conserve, regroup).
2. **Contextual prompts:** interaction/objective overlays appear only when relevant and retire quickly after action.
3. **Co-op consistency:** party status and network state changes surface in a predictable location and phrasing.
4. **Fail-forward feedback:** settings and multiplayer errors explain recovery action (what happened + what to do next).

---

## 4) Implementation Guardrails for Next Tasks

- Feed this brief directly into token work (T04), motion spec (T05), HUD concepting (T06), and menu concepting (T07).
- Maintain compile/runtime migration safety constraints from `src/ui_migration_toggle.h` and `Reference.md` while introducing new UI.
- Keep outputs practical and testable: each concept pass must include at least one low-light and one high-motion validation frame.
