# T18H Human Playtest Execution Checklist + Signoff Template

Plan ID: `ui-overhaul-2026-02-23`  
Gate ID: `T18H`  
Current Status: **Pending** (human cohorts not yet signed off)

## Gate requirement summary
- Minimum `3` human cohorts must complete scripted scenarios.
- Human findings must be consolidated with automated outputs (`T18A`).
- Gate signoff must be recorded in [t18-gating-note.md](./t18-gating-note.md).

## Cohort execution checklist (all required)

### Cohort 1 — First-time players
- [ ] Recruit participants and assign anonymized IDs.
- [ ] Execute all scenarios from [T18-scenario-spec.json](./T18-scenario-spec.json).
- [ ] Capture observations using [T18-participant-observation-template.csv](./T18-participant-observation-template.csv).
- [ ] Export events with `is_simulated=false`.

### Cohort 2 — Returning/experienced players
- [ ] Recruit participants and assign anonymized IDs.
- [ ] Execute all scenarios from [T18-scenario-spec.json](./T18-scenario-spec.json).
- [ ] Capture observations using [T18-participant-observation-template.csv](./T18-participant-observation-template.csv).
- [ ] Export events with `is_simulated=false`.

### Cohort 3 — Multiplayer-focused players
- [ ] Recruit participants and assign anonymized IDs.
- [ ] Execute all scenarios from [T18-scenario-spec.json](./T18-scenario-spec.json).
- [ ] Capture observations using [T18-participant-observation-template.csv](./T18-participant-observation-template.csv).
- [ ] Export events with `is_simulated=false`.

## Consolidation + verification checklist
- [ ] Runbook followed: [T18-playtest-runbook.md](./T18-playtest-runbook.md).
- [ ] Human event files stored under `build/research/t18/events/`.
- [ ] Telemetry pipeline re-run after human data ingest.
- [ ] Integration check passes: `powershell -ExecutionPolicy Bypass -File tools/check_t18_integration.ps1`.
- [ ] Human findings merged with [T18-ux-playtests-telemetry.md](./T18-ux-playtests-telemetry.md) outputs.
- [ ] Prioritized issues list reviewed by UX + UI engineering leads.

## Required evidence links for approvers
- Governance state note: [t18-gating-note.md](./t18-gating-note.md)
- Scenario contract: [T18-scenario-spec.json](./T18-scenario-spec.json)
- Run protocol: [T18-playtest-runbook.md](./T18-playtest-runbook.md)
- Observation template: [T18-participant-observation-template.csv](./T18-participant-observation-template.csv)
- Telemetry summary: [T18-ux-playtests-telemetry.md](./T18-ux-playtests-telemetry.md)

## Signoff capture template
Decision options:
- [ ] **Approve**
- [ ] **Approve with conditions**
- [ ] **Reject / Rework required**

Decision date (YYYY-MM-DD):  
Decision selected:

UX Research Lead  
Name:  
Role:  
Decision:  
Signature/confirmation:  
Timestamp:

UI/UX Lead  
Name:  
Role:  
Decision:  
Signature/confirmation:  
Timestamp:

Engineering representative (recommended)  
Name:  
Role:  
Decision:  
Signature/confirmation:  
Timestamp:

Conditions / required follow-ups:
- Condition 1:
- Condition 2:

## Governance note
Do not mark `T18H` as completed until all three cohorts are evidenced and required signoff is captured.
