$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$immersivePath = Join-Path $repoRoot 'src\ui_menu_immersive.h'
$hudPath = Join-Path $repoRoot 'src\hud.h'
$docPath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T17-secondary-screens-overlays.md'

foreach ($path in @($immersivePath, $hudPath, $docPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$immersive = Get-Content -Raw -Path $immersivePath
$hud = Get-Content -Raw -Path $hudPath
$doc = Get-Content -Raw -Path $docPath

$requiredImmersiveFunctions = @(
    'inline void uiImmersiveDrawGuide(float tm)',
    'inline void uiImmersiveDrawKeybinds(float tm, bool fromPause, int selected, int captureIndex)',
    'inline void uiImmersiveDrawIntro(float tm, int line, const char** introLines)',
    'inline void uiImmersiveDrawNote(float tm, int noteId, const char* title, const char* content)'
)
foreach ($fragment in $requiredImmersiveFunctions) {
    if ($immersive.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T17 integration check failed: missing immersive secondary-screen renderer -> $fragment"
        exit 1
    }
}

$requiredImmersiveStateRoutes = @(
    'if (gameState == STATE_GUIDE)',
    'if (gameState == STATE_KEYBINDS || gameState == STATE_KEYBINDS_PAUSE)',
    'if (gameState == STATE_INTRO)',
    'if (gameState == STATE_NOTE && storyMgr.readingNote && storyMgr.currentNote >= 0)'
)
foreach ($fragment in $requiredImmersiveStateRoutes) {
    if ($immersive.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T17 integration check failed: immersive state route missing -> $fragment"
        exit 1
    }
}

$requiredHudFragments = @(
    'inline void drawImmersiveSessionEndOverlay(bool escaped, float tm)',
    'const bool immersiveUiPath = useNewUiMigrationPath();',
    'if(immersiveUiPath) drawImmersiveSessionEndOverlay(true, vhsTime);',
    'if(immersiveUiPath) drawImmersiveSessionEndOverlay(false, vhsTime);'
)
foreach ($fragment in $requiredHudFragments) {
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T17 integration check failed: HUD/session-end integration missing -> $fragment"
        exit 1
    }
}

$requiredDocFragments = @(
    'Coverage: 14/14 (100%)',
    'ui-guide',
    'ui-keybinds',
    'ui-intro',
    'ui-note-reader',
    'ui-death',
    'ui-escape',
    'Intentionally retained: none'
)
foreach ($fragment in $requiredDocFragments) {
    if ($doc.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T17 integration check failed: migration checklist artifact incomplete -> $fragment"
        exit 1
    }
}

Write-Host '[PASS] T17 secondary screens + overlay migration integration contract validated'
exit 0
