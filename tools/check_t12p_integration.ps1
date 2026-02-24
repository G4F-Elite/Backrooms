$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$migrationPath = Join-Path $repoRoot 'src\ui_migration_toggle.h'
$hudPath = Join-Path $repoRoot 'src\hud.h'
$buildPath = Join-Path $repoRoot 'build.bat'

foreach ($path in @($migrationPath, $hudPath, $buildPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$migration = Get-Content -Raw -Path $migrationPath
$hud = Get-Content -Raw -Path $hudPath
$build = Get-Content -Raw -Path $buildPath

$requiredMigrationFragments = @(
    '#ifndef BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX',
    '#define BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX 0',
    'constexpr bool kUiCompileAllowT12pContextualFx = BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX != 0;',
    'inline bool useT12pContextualFeedbackSpike() {',
    'if (!useNewUiMigrationPath()) return false;',
    'const char* env = std::getenv("BR_UI_T12P_CONTEXTUAL_FX");'
)

foreach ($fragment in $requiredMigrationFragments) {
    if ($migration.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T12P integration check failed: missing migration gating fragment in src/ui_migration_toggle.h -> $fragment"
        exit 1
    }
}

$requiredHudFragments = @(
    'inline void drawProvisionalContextualFeedbackSpike(const UiPrimitiveTheme& theme, UiPrimitiveTone eventTone, UiPrimitiveTone promptTone){',
    'if(!useT12pContextualFeedbackSpike()) return;',
    'std::snprintf(spikeBuf, sizeof(spikeBuf), "T12P CONTEXT SPIKE  PRESSURE %.0f%%", pressure * 100.0f);',
    'drawProvisionalContextualFeedbackSpike(theme, eventTone, promptTone);'
)

foreach ($fragment in $requiredHudFragments) {
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T12P integration check failed: missing HUD spike fragment in src/hud.h -> $fragment"
        exit 1
    }
}

$releaseProfiles = @('production-stable', 'production-new-default', 'staging-rc')
foreach ($profile in $releaseProfiles) {
    $profilePattern = [regex]::Escape('else if /I "%BR_BUILD_PROFILE%"=="' + $profile + '" (')
    if (-not [regex]::IsMatch($build, $profilePattern)) {
        Write-Error "T12P integration check failed: could not locate build profile block in build.bat -> $profile"
        exit 1
    }
}

if ([regex]::IsMatch($build, 'BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX\s*=\s*1', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
    Write-Error 'T12P integration check failed: build.bat should not default-enable BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX for release profiles.'
    exit 1
}

Write-Host '[PASS] T12P provisional contextual feedback spike gating contract validated'
exit 0
