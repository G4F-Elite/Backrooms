$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$hudPath = Join-Path $repoRoot 'src\hud.h'
$immersivePath = Join-Path $repoRoot 'src\ui_menu_immersive.h'
$inputCommonPath = Join-Path $repoRoot 'src\input_common.h'
$inputMenuPath = Join-Path $repoRoot 'src\input_menu.h'
$inputSettingsPath = Join-Path $repoRoot 'src\input_settings.h'
$togglePath = Join-Path $repoRoot 'src\ui_migration_toggle.h'

foreach ($path in @($hudPath, $immersivePath, $inputCommonPath, $inputMenuPath, $inputSettingsPath, $togglePath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$hud = Get-Content -Raw -Path $hudPath
$immersive = Get-Content -Raw -Path $immersivePath
$inputCommon = Get-Content -Raw -Path $inputCommonPath
$inputMenu = Get-Content -Raw -Path $inputMenuPath
$inputSettings = Get-Content -Raw -Path $inputSettingsPath
$toggle = Get-Content -Raw -Path $togglePath

$requiredHudFragments = @(
    '#include "ui_menu_immersive.h"',
    'if(drawUiImmersiveMenuLayer(vhsTime)){',
    'drawUiParityPath();'
)
foreach ($fragment in $requiredHudFragments) {
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T11 integration check failed: missing hud integration fragment in src/hud.h -> $fragment"
        exit 1
    }
}

$requiredImmersiveStates = @(
    'if (gameState == STATE_MENU)',
    'if (gameState == STATE_PAUSE)',
    'if (gameState == STATE_SETTINGS || gameState == STATE_SETTINGS_PAUSE)',
    'if (gameState == STATE_MULTI)',
    'if (gameState == STATE_MULTI_JOIN)',
    'if (gameState == STATE_MULTI_HOST)',
    'if (gameState == STATE_MULTI_WAIT)'
)
foreach ($fragment in $requiredImmersiveStates) {
    if ($immersive.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T11 integration check failed: immersive menu state missing in src/ui_menu_immersive.h -> $fragment"
        exit 1
    }
}

$requiredGamepadHelpers = @(
    'inline bool menuGamepadUp()',
    'inline bool menuGamepadDown()',
    'inline bool menuGamepadLeft()',
    'inline bool menuGamepadRight()',
    'inline bool menuGamepadConfirm()',
    'inline bool menuGamepadBack()'
)
foreach ($fragment in $requiredGamepadHelpers) {
    if ($inputCommon.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T11 integration check failed: controller helper missing in src/input_common.h -> $fragment"
        exit 1
    }
}

if ($inputMenu.IndexOf('menuGamepadConfirm()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadBack()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadUp()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadDown()', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T11 integration check failed: menu input lacks controller navigation hooks in src/input_menu.h'
    exit 1
}

if ($inputSettings.IndexOf('menuGamepadConfirm()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadBack()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadLeft()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadRight()', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T11 integration check failed: settings input lacks controller navigation hooks in src/input_settings.h'
    exit 1
}

$requiredToggleFragments = @(
    'UI_MIGRATION_PATH_LEGACY',
    'UI_MIGRATION_PATH_NEW',
    'useNewUiMigrationPath()'
)
foreach ($fragment in $requiredToggleFragments) {
    if ($toggle.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T11 integration check failed: legacy/new toggle contract missing in src/ui_migration_toggle.h -> $fragment"
        exit 1
    }
}

Write-Host '[PASS] T11 immersive menu v1 integration contract validated'
exit 0
