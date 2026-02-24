$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$audioPath = Join-Path $repoRoot 'src\audio.h'
$inputMenuPath = Join-Path $repoRoot 'src\input_menu.h'
$inputSettingsPath = Join-Path $repoRoot 'src\input_settings.h'
$docPath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T13-ui-audio-feedback-mix-rules.md'

foreach ($path in @($audioPath, $inputMenuPath, $inputSettingsPath, $docPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$audio = Get-Content -Raw -Path $audioPath
$inputMenu = Get-Content -Raw -Path $inputMenuPath
$inputSettings = Get-Content -Raw -Path $inputSettingsPath
$doc = Get-Content -Raw -Path $docPath

$requiredAudioFragments = @(
    'bool uiBackTrig=false;',
    'float uiBackPitch=1.0f;',
    'static float uiBackTime = -1.0f;',
    'static float uiPriorityEnv = 0.0f;',
    'float uiBack = 0.0f;',
    'float uiDuckStrength =',
    'if(sndState.scareVol > 0.35f || sndState.deathMode)',
    'inline void triggerMenuBackSound()'
)

foreach ($fragment in $requiredAudioFragments) {
    if ($audio.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T13 integration check failed: missing audio contract fragment in src/audio.h -> $fragment"
        exit 1
    }
}

if ($audio.IndexOf('float uiMix = (uiMove + uiAdjust + uiConfirm + uiBack)', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T13 integration check failed: UI mix does not include back/cancel cue in src/audio.h'
    exit 1
}

if ($inputMenu.IndexOf('triggerMenuBackSound();', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T13 integration check failed: src/input_menu.h missing back/cancel cue routing'
    exit 1
}

if ($inputSettings.IndexOf('triggerMenuBackSound();', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T13 integration check failed: src/input_settings.h missing back/cancel cue routing'
    exit 1
}

$requiredDocFragments = @(
    '# T13 UI audio feedback and mix rules',
    '## Distinct UI action cues',
    '## Mix ducking and priority rules',
    '## Audio QA clarity validation evidence',
    '[PASS] T13 UI audio feedback + mix rules integration contract validated'
)
foreach ($fragment in $requiredDocFragments) {
    if ($doc.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T13 integration check failed: documentation evidence fragment missing in T13 doc -> $fragment"
        exit 1
    }
}

Write-Host '[PASS] T13 UI audio feedback + mix rules integration contract validated'
exit 0
