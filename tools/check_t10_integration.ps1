$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$hudPath = Join-Path $repoRoot 'src\hud.h'
$migrationPath = Join-Path $repoRoot 'src\ui_migration_toggle.h'

foreach ($path in @($hudPath, $migrationPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$hud = Get-Content -Raw -Path $hudPath
$migration = Get-Content -Raw -Path $migrationPath

$requiredHudFragments = @(
    'inline void drawImmersiveGameplayHudV1()',
    'const bool immersiveHudActive = useNewUiMigrationPath();',
    'if(immersiveHudActive){',
    'drawImmersiveGameplayHudV1();',
    'if(!immersiveHudActive && squadCalloutTimer > 0.0f)',
    'if(!immersiveHudActive && multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime()))'
)

foreach ($fragment in $requiredHudFragments) {
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T10 integration check failed: missing HUD contract fragment in src/hud.h -> $fragment"
        exit 1
    }
}

$newUiContractPattern = 'inline\s+void\s+drawNewUI\s*\(\s*\)\s*\{\s*if\s*\(\s*drawUiImmersiveMenuLayer\s*\(\s*vhsTime\s*\)\s*\)\s*\{\s*return\s*;\s*\}\s*drawUiParityPath\s*\(\s*\)\s*;\s*\}'
$newUiContractOk = [regex]::IsMatch($hud, $newUiContractPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $newUiContractOk) {
    Write-Error 'T10 integration check failed: drawNewUI() immersive-menu early-return contract regressed in src/hud.h'
    exit 1
}

$toggleContractPattern = 'inline\s+bool\s+useNewUiMigrationPath\s*\(\s*\)\s*\{\s*return\s+resolveUiMigrationPath\s*\(\s*\)\s*==\s*UI_MIGRATION_PATH_NEW\s*;\s*\}'
$toggleContractOk = [regex]::IsMatch($migration, $toggleContractPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $toggleContractOk) {
    Write-Error 'T10 integration check failed: useNewUiMigrationPath() contract changed in src/ui_migration_toggle.h'
    exit 1
}

Write-Host '[PASS] T10 immersive HUD integration contract validated'
exit 0
