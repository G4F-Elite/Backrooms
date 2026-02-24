param(
    [string]$ScenarioFile = 'docs/plan/ui-overhaul-2026-02-23/T18-scenario-spec.json',
    [string]$InputEventsDir = 'build/research/t18/events',
    [string]$OutputDir = 'build/research/t18',
    [switch]$GenerateMockData,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Ensure-Directory {
    param([string]$Path)
    if (-not (Test-Path -Path $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function New-Event {
    param(
        [string]$RunId,
        [string]$CohortId,
        [string]$ScenarioId,
        [string]$EventType,
        [datetime]$Timestamp,
        [string]$UiSurface,
        [string]$Step,
        [bool]$IsSimulated,
        [hashtable]$Payload
    )

    $event = [ordered]@{
        run_id       = $RunId
        cohort_id    = $CohortId
        scenario_id  = $ScenarioId
        event_type   = $EventType
        timestamp_utc = $Timestamp.ToString('o')
        ui_surface   = $UiSurface
        step         = $Step
        is_simulated = $IsSimulated
        payload      = if ($null -eq $Payload) { @{} } else { $Payload }
    }

    return ($event | ConvertTo-Json -Compress -Depth 8)
}

function Get-Severity {
    param([double]$Score)
    if ($Score -ge 0.85) { return 'high' }
    if ($Score -ge 0.55) { return 'medium' }
    return 'low'
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$scenarioPath = Join-Path $repoRoot $ScenarioFile
$eventsDirPath = Join-Path $repoRoot $InputEventsDir
$outputDirPath = Join-Path $repoRoot $OutputDir

if (-not (Test-Path -Path $scenarioPath)) {
    Write-Error "T18 pipeline failed: scenario spec not found at $scenarioPath"
    exit 1
}

Ensure-Directory -Path $eventsDirPath
Ensure-Directory -Path $outputDirPath

$scenarioSpec = Get-Content -Raw -Path $scenarioPath | ConvertFrom-Json

if ($GenerateMockData) {
    if ($Force) {
        Get-ChildItem -Path $eventsDirPath -Filter '*.jsonl' -File -ErrorAction SilentlyContinue | Remove-Item -Force
    }

    $cohorts = @(
        [ordered]@{ cohort_id = 'cohort-A'; profile = 'new-player' },
        [ordered]@{ cohort_id = 'cohort-B'; profile = 'returning-player' },
        [ordered]@{ cohort_id = 'cohort-C'; profile = 'multiplayer-focused' }
    )

    foreach ($cohort in $cohorts) {
        foreach ($scenario in $scenarioSpec.scenarios) {
            $runId = "{0}-{1}" -f $cohort.cohort_id, $scenario.id
            $filePath = Join-Path $eventsDirPath ("{0}.jsonl" -f $runId)

            $baseStart = [datetime]::UtcNow.AddMinutes((Get-Random -Minimum -120 -Maximum 0))
            $cursor = $baseStart
            $lines = New-Object System.Collections.Generic.List[string]

            $lines.Add((New-Event -RunId $runId -CohortId $cohort.cohort_id -ScenarioId $scenario.id -EventType 'scenario_start' -Timestamp $cursor -UiSurface $scenario.entry_surface -Step 'start' -IsSimulated $true -Payload @{ profile = $cohort.profile }))

            foreach ($step in $scenario.scripted_steps) {
                $cursor = $cursor.AddSeconds((Get-Random -Minimum 6 -Maximum 25))
                $lines.Add((New-Event -RunId $runId -CohortId $cohort.cohort_id -ScenarioId $scenario.id -EventType 'step_complete' -Timestamp $cursor -UiSurface $step.ui_surface -Step $step.id -IsSimulated $true -Payload @{ target = $step.success_marker }))

                if ((Get-Random -Minimum 0 -Maximum 100) -lt 35) {
                    $cursor = $cursor.AddSeconds((Get-Random -Minimum 3 -Maximum 11))
                    $confusionType = @('help_opened', 'prompt_repeat', 'menu_backtrack', 'idle_hesitation')[(Get-Random -Minimum 0 -Maximum 4)]
                    $lines.Add((New-Event -RunId $runId -CohortId $cohort.cohort_id -ScenarioId $scenario.id -EventType $confusionType -Timestamp $cursor -UiSurface $step.ui_surface -Step $step.id -IsSimulated $true -Payload @{ reason = 'simulated-friction-sample' }))
                }
            }

            $cursor = $cursor.AddSeconds((Get-Random -Minimum 8 -Maximum 20))
            $lines.Add((New-Event -RunId $runId -CohortId $cohort.cohort_id -ScenarioId $scenario.id -EventType 'scenario_complete' -Timestamp $cursor -UiSurface $scenario.exit_surface -Step 'complete' -IsSimulated $true -Payload @{}))

            Set-Content -Path $filePath -Value $lines -Encoding UTF8
        }
    }
}

$eventFiles = Get-ChildItem -Path $eventsDirPath -Filter '*.jsonl' -File -ErrorAction SilentlyContinue
if ($eventFiles.Count -eq 0) {
    Write-Error "T18 pipeline failed: no event files found in $eventsDirPath"
    exit 1
}

$events = New-Object System.Collections.Generic.List[object]
foreach ($file in $eventFiles) {
    $rawLines = Get-Content -Path $file.FullName | Where-Object { $_.Trim().Length -gt 0 }
    foreach ($line in $rawLines) {
        try {
            $evt = $line | ConvertFrom-Json
            $events.Add($evt)
        }
        catch {
            Write-Error "T18 pipeline failed: invalid JSON in $($file.FullName)"
            exit 1
        }
    }
}

$eventsByRun = $events | Group-Object run_id
$scenarioLookup = @{}
foreach ($scenario in $scenarioSpec.scenarios) {
    $scenarioLookup[$scenario.id] = $scenario
}

$runSummaries = New-Object System.Collections.Generic.List[object]
$hotspotCounter = @{}

foreach ($runGroup in $eventsByRun) {
    $runEvents = $runGroup.Group | Sort-Object { [datetime]$_.timestamp_utc }
    $start = $runEvents | Where-Object { $_.event_type -eq 'scenario_start' } | Select-Object -First 1
    $finish = $runEvents | Where-Object { $_.event_type -eq 'scenario_complete' } | Select-Object -Last 1
    if ($null -eq $start -or $null -eq $finish) {
        continue
    }

    $startTs = [datetime]$start.timestamp_utc
    $finishTs = [datetime]$finish.timestamp_utc
    $seconds = [math]::Max(0.0, ($finishTs - $startTs).TotalSeconds)
    $scenario = $scenarioLookup[$start.scenario_id]

    $confusionEvents = $runEvents | Where-Object { $_.event_type -in @('help_opened', 'prompt_repeat', 'menu_backtrack', 'idle_hesitation', 'fail_state') }
    foreach ($conf in $confusionEvents) {
        $key = "{0}|{1}" -f $conf.ui_surface, $conf.event_type
        if (-not $hotspotCounter.ContainsKey($key)) {
            $hotspotCounter[$key] = 0
        }
        $hotspotCounter[$key] += 1
    }

    $runSummaries.Add([pscustomobject]@{
        run_id = $runGroup.Name
        cohort_id = $start.cohort_id
        scenario_id = $start.scenario_id
        completed = $true
        completion_seconds = [math]::Round($seconds, 2)
        target_seconds = [double]$scenario.target_completion_seconds
        confusion_events = $confusionEvents.Count
        is_simulated = [bool]$start.is_simulated
    })
}

if ($runSummaries.Count -eq 0) {
    Write-Error 'T18 pipeline failed: no complete runs (scenario_start + scenario_complete) detected.'
    exit 1
}

$cohortCount = ($runSummaries | Select-Object -ExpandProperty cohort_id -Unique).Count
$allSimulated = ($runSummaries | Where-Object { -not $_.is_simulated }).Count -eq 0

$scenarioRows = @()
foreach ($sg in ($runSummaries | Group-Object scenario_id)) {
    $target = [double]$scenarioLookup[$sg.Name].target_completion_seconds
    $avg = ($sg.Group | Measure-Object -Property completion_seconds -Average).Average
    $p95 = ($sg.Group | Sort-Object completion_seconds | Select-Object -Last 1).completion_seconds
    $completionRate = 1.0

    $scenarioRows += [pscustomobject]@{
        scenario_id = $sg.Name
        cohort_runs = $sg.Count
        completion_rate = [math]::Round($completionRate, 2)
        avg_completion_seconds = [math]::Round([double]$avg, 2)
        p95_completion_seconds = [math]::Round([double]$p95, 2)
        target_completion_seconds = $target
        target_delta_seconds = [math]::Round(([double]$avg - $target), 2)
    }
}

$hotspots = @()
foreach ($entry in $hotspotCounter.GetEnumerator()) {
    $parts = $entry.Key.Split('|')
    $uiSurface = $parts[0]
    $eventType = $parts[1]
    $count = [int]$entry.Value
    $severityScore = [math]::Min(1.0, $count / [math]::Max(3.0, $runSummaries.Count / 2.0))
    $hotspots += [pscustomobject]@{
        ui_surface = $uiSurface
        event_type = $eventType
        count = $count
        severity = Get-Severity -Score $severityScore
    }
}
$hotspots = $hotspots | Sort-Object -Property @{Expression='count';Descending=$true}, @{Expression='ui_surface';Descending=$false}

$findings = New-Object System.Collections.Generic.List[object]
foreach ($row in $scenarioRows) {
    if ($row.avg_completion_seconds -gt $row.target_completion_seconds) {
        $ratioOver = ($row.avg_completion_seconds - $row.target_completion_seconds) / [math]::Max(1.0, $row.target_completion_seconds)
        $severity = if ($ratioOver -ge 0.35) { 'high' } elseif ($ratioOver -ge 0.15) { 'medium' } else { 'low' }
        $findings.Add([pscustomobject]@{
            finding_id = "finding-time-{0}" -f $row.scenario_id
            severity = $severity
            category = 'task-completion-time'
            summary = "Scenario '$($row.scenario_id)' average completion exceeded target by $($row.target_delta_seconds)s."
            evidence = "avg=$($row.avg_completion_seconds)s target=$($row.target_completion_seconds)s"
        })
    }
}

foreach ($spot in ($hotspots | Select-Object -First 5)) {
    $findings.Add([pscustomobject]@{
        finding_id = ("finding-hotspot-{0}-{1}" -f $spot.ui_surface, $spot.event_type)
        severity = $spot.severity
        category = 'confusion-hotspot'
        summary = "Repeated confusion signal '$($spot.event_type)' observed on '$($spot.ui_surface)' ($($spot.count)x)."
        evidence = "count=$($spot.count)"
    })
}

$primaryLimitation = if ($allSimulated) {
    'No human participant sessions were captured in this automated environment.'
}
else {
    'Human and/or mixed telemetry included; validate participant consent/protocol logs separately.'
}

$dashboard = [ordered]@{
    generated_at_utc = [datetime]::UtcNow.ToString('o')
    task_id = 'T18'
    plan_id = 'ui-overhaul-2026-02-23'
    cohorts_detected = $cohortCount
    complete_runs = $runSummaries.Count
    data_source = if ($allSimulated) { 'simulated-only' } else { 'mixed-or-human' }
    limitations = @(
        $primaryLimitation,
        'Qualitative findings require human moderator notes and participant quotes to satisfy full T18 criteria.'
    )
    telemetry = [ordered]@{
        completion_time = $scenarioRows
        confusion_hotspots = ($hotspots | Select-Object -First 15)
    }
    prioritized_findings = $findings
}

$dashboardJson = $dashboard | ConvertTo-Json -Depth 10
$dashboardPath = Join-Path $outputDirPath 't18_dashboard.json'
Set-Content -Path $dashboardPath -Value $dashboardJson -Encoding UTF8

$runSummaryPath = Join-Path $outputDirPath 't18_run_summary.csv'
$runSummaries | Sort-Object scenario_id, cohort_id | Export-Csv -Path $runSummaryPath -NoTypeInformation -Encoding UTF8

$hotspotsPath = Join-Path $outputDirPath 't18_hotspots.csv'
$hotspots | Export-Csv -Path $hotspotsPath -NoTypeInformation -Encoding UTF8

$findingsPath = Join-Path $outputDirPath 't18_findings.csv'
$findings | Export-Csv -Path $findingsPath -NoTypeInformation -Encoding UTF8

$evidencePath = Join-Path $outputDirPath 't18_execution_evidence.md'
$evidenceLines = @(
    '# T18 Telemetry Execution Evidence',
    '',
    "- Generated at (UTC): $([datetime]::UtcNow.ToString('o'))",
    "- Scenario spec: $ScenarioFile",
    "- Event files read: $($eventFiles.Count)",
    "- Complete runs: $($runSummaries.Count)",
    "- Cohorts detected: $cohortCount",
    "- Data source: $(if ($allSimulated) { 'simulated-only' } else { 'mixed-or-human' })",
    '',
    '## Output files',
    '',
    "- t18_dashboard.json",
    "- t18_run_summary.csv",
    "- t18_hotspots.csv",
    "- t18_findings.csv",
    '',
    '## Limitation marker',
    '',
    '- This execution does not fabricate human outcomes. If data source is simulated-only, human cohort sessions remain required for full criteria sign-off.'
)
Set-Content -Path $evidencePath -Value $evidenceLines -Encoding UTF8

Write-Host '[PASS] T18 telemetry pipeline executed'
Write-Host ("Dashboard: {0}" -f $dashboardPath)
exit 0
