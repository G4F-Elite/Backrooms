$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$primitivesPath = Join-Path $repoRoot 'src\ui_primitives.h'
$hudPath = Join-Path $repoRoot 'src\hud.h'
$t12pCheckPath = Join-Path $repoRoot 'tools\check_t12p_integration.ps1'
$captureScriptPath = Join-Path $repoRoot 'tools\capture_t15_perf_evidence.ps1'
$reportPath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T15-render-performance-report.md'
$evidencePath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T15-perf-evidence-2026-02-23.json'

foreach ($path in @($primitivesPath, $hudPath, $t12pCheckPath, $captureScriptPath, $reportPath, $evidencePath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$primitives = Get-Content -Raw -Path $primitivesPath
$hud = Get-Content -Raw -Path $hudPath

$requiredPrimitivesFragments = @(
    'namespace UiPrimitivePerfBudget',
    'constexpr float kUiFrameCpuBudgetMs = 0.35f;',
    'constexpr int kRectPassBudget = 42;',
    'constexpr int kTextPassBudget = 22;',
    'constexpr float kOverdrawLayerBudget = 2.40f;',
    'constexpr int kMenuSampleRectPassBaselineT08 = 41;',
    'constexpr int kMenuSampleRectPassTargetT15 = 39;',
    'float rectAreaNdc = 0.0f;',
    'float overdrawLayersEstimate = 0.0f;',
    'int atlasFallbackGlyphPasses = 0;',
    'int atlasTexturePasses = 0;',
    'inline bool uiPrimitiveFrameWithinBudget() {',
    'gUiPrimitivePerf.overdrawLayersEstimate = gUiPrimitivePerf.rectAreaNdc / 4.0f;',
    'gUiPrimitivePerf.budgetPassLast = uiPrimitiveFrameWithinBudget();',
    'uiPrimitiveRect(left + 0.008f, top - 0.024f, right - 0.008f, top - 0.010f, theme.panelAccent',
    'OVR %.2f ATL F:%d T:%d CPU %.3fms %s'
)

foreach ($fragment in $requiredPrimitivesFragments) {
    if ($primitives.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T15 perf check failed: missing fragment in src/ui_primitives.h -> $fragment"
        exit 1
    }
}

if ($primitives.IndexOf('uiPrimitiveRect(left + 0.002f, bottom + 0.002f, right - 0.002f, top - 0.002f, UiColor::kOverlayWarm, 0.22f);', [System.StringComparison]::Ordinal) -ge 0) {
    Write-Error 'T15 perf check failed: legacy icon inner warm fill still present; expected reduced icon overdraw path'
    exit 1
}

if ($primitives.IndexOf('uiInitIconAtlasScaffold();', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T15 perf check failed: icon atlas scaffold init missing from icon draw path'
    exit 1
}

$beginFrameMatch = [regex]::Match($primitives, 'inline\s+void\s+uiPrimitiveBeginFrame\s*\(\s*float\s+tm\s*\)\s*\{(?<body>.*?)\n\}', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $beginFrameMatch.Success) {
    Write-Error 'T15 perf check failed: unable to parse uiPrimitiveBeginFrame body'
    exit 1
}
if ($beginFrameMatch.Groups['body'].Value.IndexOf('uiInitIconAtlasScaffold();', [System.StringComparison]::Ordinal) -ge 0) {
    Write-Error 'T15 perf check failed: uiPrimitiveBeginFrame still initializes icon atlas scaffold every frame'
    exit 1
}

if ($hud.IndexOf('drawProvisionalContextualFeedbackSpike(theme, eventTone, promptTone);', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T15 perf check failed: expected T12P contextual feedback hook missing from src/hud.h'
    exit 1
}

$report = Get-Content -Raw -Path $reportPath
$requiredReportFragments = @(
    '# T15 — UI Render Performance Optimization Report',
    'Plan ID: `ui-overhaul-2026-02-23`',
    'Task ID: `T15`',
    'UI frame cost budget',
    'Overdraw and atlas usage deltas',
    'Quantified atlas before/after evidence',
    'Empirical budget evidence (captured in this environment)',
    'Actionable metrics and next actions'
)
foreach ($fragment in $requiredReportFragments) {
    if ($report.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T15 perf check failed: report missing required section -> $fragment"
        exit 1
    }
}

$evidenceRaw = Get-Content -Raw -Path $evidencePath
try {
    $evidence = $evidenceRaw | ConvertFrom-Json -ErrorAction Stop
}
catch {
    Write-Error "T15 perf check failed: evidence JSON is invalid -> $evidencePath"
    exit 1
}

if ($evidence.plan_id -ne 'ui-overhaul-2026-02-23' -or $evidence.task_id -ne 'T15') {
    Write-Error 'T15 perf check failed: evidence plan/task identifiers are missing or incorrect'
    exit 1
}

if (-not $evidence.hardware_profile -or -not $evidence.hardware_profile.cpu -or -not $evidence.hardware_profile.gpu) {
    Write-Error 'T15 perf check failed: evidence missing hardware profile details (cpu/gpu)'
    exit 1
}

if (-not $evidence.ui_budget -or $null -eq $evidence.ui_budget.menu_sample_rect_delta_percent) {
    Write-Error 'T15 perf check failed: evidence missing quantified menu-sample delta percent'
    exit 1
}

if (-not $evidence.empirical_ui_budget -or $null -eq $evidence.empirical_ui_budget.p95_runtime_ms -or $null -eq $evidence.empirical_ui_budget.overall_pass_rate_percent) {
    Write-Error 'T15 perf check failed: evidence missing empirical UI budget summary (p95/pass-rate)'
    exit 1
}

if (-not $evidence.empirical_run_stats -or $null -eq $evidence.empirical_run_stats.overall_pass_rate_percent) {
    Write-Error 'T15 perf check failed: evidence missing empirical overall pass-rate'
    exit 1
}

if (-not $evidence.p95_logs -or -not $evidence.p95_logs.values -or $evidence.p95_logs.values.Count -lt 1) {
    Write-Error 'T15 perf check failed: evidence missing p95 log entries'
    exit 1
}

if (-not $evidence.atlas_before_after -or -not $evidence.atlas_before_after.quantified_change) {
    Write-Error 'T15 perf check failed: evidence missing quantified atlas before/after change statement'
    exit 1
}

if ($null -eq $evidence.atlas_before_after.rect_passes_before -or $null -eq $evidence.atlas_before_after.rect_passes_after -or $null -eq $evidence.atlas_before_after.rect_pass_delta_percent) {
    Write-Error 'T15 perf check failed: evidence missing quantified atlas before/after numeric fields'
    exit 1
}

$expectedAtlasDelta = [math]::Round((100.0 * (($evidence.atlas_before_after.rect_passes_after - $evidence.atlas_before_after.rect_passes_before) / [double]$evidence.atlas_before_after.rect_passes_before)), 2)
if ([math]::Abs([double]$evidence.atlas_before_after.rect_pass_delta_percent - $expectedAtlasDelta) -gt 0.01) {
    Write-Error 'T15 perf check failed: atlas rect-pass delta percent is inconsistent with before/after counts'
    exit 1
}

if ([double]$evidence.empirical_ui_budget.overall_pass_rate_percent -ne [double]$evidence.empirical_run_stats.overall_pass_rate_percent) {
    Write-Error 'T15 perf check failed: empirical_ui_budget overall pass-rate does not match empirical_run_stats overall pass-rate'
    exit 1
}

if ([double]$evidence.empirical_ui_budget.p95_runtime_ms -le 0) {
    Write-Error 'T15 perf check failed: empirical_ui_budget p95 runtime must be > 0'
    exit 1
}

Write-Host '[PASS] T15 performance budget/instrumentation/report contract validated'
exit 0
