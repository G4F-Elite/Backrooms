$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$audioPath = Join-Path $repoRoot 'src\audio.h'
$docPath = Join-Path $repoRoot 'docs\plan\ui-overhaul-2026-02-23\T13-audio-qa-high-intensity-checklist.md'

foreach ($path in @($audioPath, $docPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$audio = Get-Content -Raw -Path $audioPath
$doc = Get-Content -Raw -Path $docPath

$requiredAudioFragments = @(
    'float uiMix = (uiMove + uiAdjust + uiConfirm + uiBack)',
    'float sceneIntensity = clamp01Audio(sndState.monsterProximity * 0.45f + sndState.monsterMenace * 0.35f + sndState.dangerLevel * 0.20f);',
    'float uiDuckStrength = uiPriority * (0.22f + sceneIntensity * 0.30f);',
    'if(sndState.scareVol > 0.35f || sndState.deathMode) {',
    'if(uiDuckStrength > 0.18f) uiDuckStrength = 0.18f;',
    'ambienceMix *= (1.0f - uiDuckStrength * 0.50f);',
    'sfxMix *= (1.0f - uiDuckStrength * 0.18f);',
    'musicMix *= (1.0f - uiDuckStrength * 0.58f);'
)

foreach ($fragment in $requiredAudioFragments) {
    if ($audio.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T13 high-intensity hook check failed: missing audio fragment in src/audio.h -> $fragment"
        exit 1
    }
}

$requiredDocFragments = @(
    '# T13 Audio QA — High-Intensity Clarity Checklist',
    'HI-01',
    'HI-02',
    'HI-03',
    'HI-04',
    'HI-05',
    'check_t13_integration.ps1',
    'check_t13_high_intensity_hooks.ps1'
)

foreach ($fragment in $requiredDocFragments) {
    if ($doc.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T13 high-intensity hook check failed: checklist fragment missing -> $fragment"
        exit 1
    }
}

Write-Host '[PASS] T13 high-intensity audio cue + ducking hook contract validated'
exit 0
