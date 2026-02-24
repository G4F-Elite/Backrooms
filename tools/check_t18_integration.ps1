$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$requiredFiles = @(
    'tools\run_t18_telemetry_pipeline.ps1',
    'docs\plan\ui-overhaul-2026-02-23\T18-scenario-spec.json',
    'docs\plan\ui-overhaul-2026-02-23\T18-ux-playtests-telemetry.md',
    'docs\plan\ui-overhaul-2026-02-23\T18-playtest-runbook.md',
    'docs\plan\ui-overhaul-2026-02-23\T18-participant-observation-template.csv',
    'build\research\t18\t18_dashboard.json',
    'build\research\t18\t18_findings.csv'
)

foreach ($relativePath in $requiredFiles) {
    $absolutePath = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -Path $absolutePath)) {
        Write-Error "T18 integration check failed: missing required artifact -> $relativePath"
        exit 1
    }
}

$dashboardPath = Join-Path $repoRoot 'build\research\t18\t18_dashboard.json'
$dashboard = Get-Content -Raw -Path $dashboardPath | ConvertFrom-Json

$requiredDashboardProps = @(
    'task_id',
    'plan_id',
    'cohorts_detected',
    'telemetry',
    'prioritized_findings',
    'limitations'
)

foreach ($prop in $requiredDashboardProps) {
    if ($null -eq $dashboard.$prop) {
        Write-Error "T18 integration check failed: dashboard missing property '$prop'"
        exit 1
    }
}

if ($dashboard.task_id -ne 'T18') {
    Write-Error "T18 integration check failed: dashboard task_id expected 'T18' but got '$($dashboard.task_id)'"
    exit 1
}

if ($dashboard.plan_id -ne 'ui-overhaul-2026-02-23') {
    Write-Error "T18 integration check failed: dashboard plan_id expected 'ui-overhaul-2026-02-23' but got '$($dashboard.plan_id)'"
    exit 1
}

if ($dashboard.cohorts_detected -lt 3) {
    Write-Error "T18 integration check failed: expected at least 3 cohorts in dashboard, got $($dashboard.cohorts_detected)"
    exit 1
}

$docPath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T18-ux-playtests-telemetry.md'
$docContent = Get-Content -Raw -Path $docPath
$requiredDocFragments = @(
    'Task ID: `T18`',
    'At least 3 playtest cohorts complete scripted scenarios.',
    'Telemetry dashboard includes completion time and confusion hotspots.',
    'Top usability findings prioritized with severity labels.',
    'No human participant sessions were executed in this automated run.',
    'Runbook and templates for human cohorts are included'
)

foreach ($fragment in $requiredDocFragments) {
    if ($docContent.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T18 integration check failed: documentation evidence fragment missing -> $fragment"
        exit 1
    }
}

Write-Host '[PASS] T18 UX playtest telemetry integration contract validated'
exit 0
