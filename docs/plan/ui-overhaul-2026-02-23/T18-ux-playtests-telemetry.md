# T18 — Structured UX Playtests and Telemetry Collection

Status: implemented (automation path)  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T18`  
Date: 2026-02-23

## Scope
Execute scripted UX scenario telemetry that can run in this automated environment, generate a dashboard with completion time + confusion hotspots, and output prioritized findings with severity labels.

## Success criteria mapping
- At least 3 playtest cohorts complete scripted scenarios.
- Telemetry dashboard includes completion time and confusion hotspots.
- Top usability findings prioritized with severity labels.

## Implemented artifacts
- Scenario contract: `docs/plan/ui-overhaul-2026-02-23/T18-scenario-spec.json`
- Telemetry pipeline: `tools/run_t18_telemetry_pipeline.ps1`
- Integration check: `tools/check_t18_integration.ps1`
- Human-session runbook: `docs/plan/ui-overhaul-2026-02-23/T18-playtest-runbook.md`
- Observation template: `docs/plan/ui-overhaul-2026-02-23/T18-participant-observation-template.csv`

## Automated execution (truthful)
Command:

```powershell
powershell -ExecutionPolicy Bypass -File tools/run_t18_telemetry_pipeline.ps1 -GenerateMockData -Force
```

Generated outputs:
- `build/research/t18/t18_dashboard.json`
- `build/research/t18/t18_run_summary.csv`
- `build/research/t18/t18_hotspots.csv`
- `build/research/t18/t18_findings.csv`
- `build/research/t18/t18_execution_evidence.md`

Dashboard fields include:
- Cohort count and complete runs.
- Completion-time metrics per scripted scenario.
- Confusion hotspots grouped by UI surface and signal type.
- Prioritized usability findings with severity (`high|medium|low`).

## Human-session limitation (explicit)
- No human participant sessions were executed in this automated run.
- Any current cohort data marked `is_simulated=true` is for pipeline validation only.
- Full qualitative completion of T18 still requires moderated/unmoderated human sessions across at least 3 real cohorts.

## Human path coverage to close remaining gap
Runbook and templates for human cohorts are included and ready:
- `docs/plan/ui-overhaul-2026-02-23/T18-playtest-runbook.md`
- `docs/plan/ui-overhaul-2026-02-23/T18-participant-observation-template.csv`

## Contract validation
Command:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_t18_integration.ps1
```

Pass condition:
- Script exits `0` and prints `[PASS] T18 UX playtest telemetry integration contract validated`.
