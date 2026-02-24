# T18 Human Playtest Runbook

Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T18`

## Purpose
Provide a concrete protocol to run human UX sessions that complement the automated telemetry pipeline and satisfy T18 criteria that require real participants.

## Cohort requirement
- Minimum: 3 human cohorts.
- Recommended cohort slices:
  - Cohort 1: first-time players
  - Cohort 2: returning/experienced players
  - Cohort 3: multiplayer-focused players

## Pre-session checklist
1. Ensure build includes T17 UI migration path and current T14 accessibility baseline.
2. Confirm scenario contract in `T18-scenario-spec.json` is unchanged or versioned.
3. Create per-session event file destination under `build/research/t18/events/`.
4. Confirm participant consent process aligns with team policy (recording/notes permissions).

## Scripted scenario protocol
Run all three scenarios from `T18-scenario-spec.json` in order:
- `S1-new-run-objective-read`
- `S2-settings-accessibility-adjust`
- `S3-multiplayer-join-status`

For each scenario:
1. Read instruction exactly as written.
2. Start timer at first actionable prompt.
3. Record completion time and confusion events:
   - `help_opened`
   - `prompt_repeat`
   - `menu_backtrack`
   - `idle_hesitation`
   - `fail_state`
4. Capture one short qualitative note (quote or paraphrase) on clarity/immersion/cognitive load.

## Data capture template
Use:
- `docs/plan/ui-overhaul-2026-02-23/T18-participant-observation-template.csv`

Required columns:
- `participant_id`, `cohort_id`, `scenario_id`, `completion_seconds`, `event_type`, `ui_surface`, `severity`, `observation_note`

## Post-session processing
1. Convert observation rows into JSONL event records using the event contract in `T18-scenario-spec.json`.
2. Place event files in `build/research/t18/events/` with `is_simulated=false`.
3. Re-run telemetry pipeline:

```powershell
powershell -ExecutionPolicy Bypass -File tools/run_t18_telemetry_pipeline.ps1
```

4. Re-run contract check:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t18_integration.ps1
```

## Completion gate for human criteria
T18 is fully human-complete when all are true:
- At least 3 real cohorts are present in dashboard input (`is_simulated=false`).
- Dashboard still includes completion-time and hotspot metrics.
- Prioritized findings are reviewed and triaged by UX + UI engineering leads.
