$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$styleTokensPath = Join-Path $repoRoot 'src\ui_style_tokens.h'
$menuPath = Join-Path $repoRoot 'src\menu.h'
$hudPath = Join-Path $repoRoot 'src\hud.h'
$immersivePath = Join-Path $repoRoot 'src\ui_menu_immersive.h'
$primitivesPath = Join-Path $repoRoot 'src\ui_primitives.h'
$inputCommonPath = Join-Path $repoRoot 'src\input_common.h'
$inputMenuPath = Join-Path $repoRoot 'src\input_menu.h'
$inputSettingsPath = Join-Path $repoRoot 'src\input_settings.h'
$inputKeybindsPath = Join-Path $repoRoot 'src\input_keybinds.h'

$requiredPaths = @(
    $styleTokensPath,
    $menuPath,
    $hudPath,
    $immersivePath,
    $primitivesPath,
    $inputCommonPath,
    $inputMenuPath,
    $inputSettingsPath,
    $inputKeybindsPath
)

foreach ($path in $requiredPaths) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$styleTokens = Get-Content -Raw -Path $styleTokensPath
$menu = Get-Content -Raw -Path $menuPath
$hud = Get-Content -Raw -Path $hudPath
$immersive = Get-Content -Raw -Path $immersivePath
$primitives = Get-Content -Raw -Path $primitivesPath
$inputCommon = Get-Content -Raw -Path $inputCommonPath
$inputMenu = Get-Content -Raw -Path $inputMenuPath
$inputSettings = Get-Content -Raw -Path $inputSettingsPath
$inputKeybinds = Get-Content -Raw -Path $inputKeybindsPath

function Parse-UiColorToken {
    param(
        [string]$content,
        [string]$tokenName
    )

    $pattern = "constexpr\s+UiColorToken\s+$tokenName\s*\{\s*([0-9]*\.?[0-9]+)f\s*,\s*([0-9]*\.?[0-9]+)f\s*,\s*([0-9]*\.?[0-9]+)f\s*\};"
    $match = [regex]::Match($content, $pattern)
    if (-not $match.Success) {
        throw "Could not parse color token '$tokenName' from src/ui_style_tokens.h"
    }

    return @(
        [double]$match.Groups[1].Value,
        [double]$match.Groups[2].Value,
        [double]$match.Groups[3].Value
    )
}

function Convert-SrgbToLinear([double]$channel) {
    if ($channel -le 0.03928) {
        return $channel / 12.92
    }
    return [Math]::Pow(($channel + 0.055) / 1.055, 2.4)
}

function Get-RelativeLuminance([double[]]$rgb) {
    $r = Convert-SrgbToLinear $rgb[0]
    $g = Convert-SrgbToLinear $rgb[1]
    $b = Convert-SrgbToLinear $rgb[2]
    return 0.2126 * $r + 0.7152 * $g + 0.0722 * $b
}

function Get-ContrastRatio([double[]]$a, [double[]]$b) {
    $la = Get-RelativeLuminance $a
    $lb = Get-RelativeLuminance $b
    $light = [Math]::Max($la, $lb)
    $dark = [Math]::Min($la, $lb)
    return ($light + 0.05) / ($dark + 0.05)
}

function Blend-Color([double[]]$foreground, [double[]]$background, [double]$alpha) {
    $fa = [double]$alpha
    $ba = [double](1.0 - $fa)
    $fr = [double]$foreground[0]
    $fg = [double]$foreground[1]
    $fb = [double]$foreground[2]
    $br = [double]$background[0]
    $bg = [double]$background[1]
    $bb = [double]$background[2]

    return [double[]]@(
        ($fr * $fa) + ($br * $ba)
        ($fg * $fa) + ($bg * $ba)
        ($fb * $fa) + ($bb * $ba)
    )
}

function Assert-RenderedContrast {
    param(
        [string]$Name,
        [double[]]$TextColor,
        [double]$TextAlpha,
        [double[]]$SurfaceColor,
        [double]$SurfaceAlpha,
        [double[]]$BackdropColor,
        [double]$MinRatio
    )

    $surfaceComposite = Blend-Color -foreground $SurfaceColor -background $BackdropColor -alpha $SurfaceAlpha
    $textComposite = Blend-Color -foreground $TextColor -background $surfaceComposite -alpha $TextAlpha
    $ratio = Get-ContrastRatio $textComposite $surfaceComposite
    if ($ratio -lt $MinRatio) {
        Write-Error ("T14 integration check failed: rendered legibility case '{0}' below {1:N2}:1 (actual {2:N2}:1)." -f $Name, $MinRatio, $ratio)
        exit 1
    }
    return $ratio
}

$textPrimary = Parse-UiColorToken -content $styleTokens -tokenName 'kTextPrimary'
$textSecondary = Parse-UiColorToken -content $styleTokens -tokenName 'kTextSecondary'
$warning = Parse-UiColorToken -content $styleTokens -tokenName 'kStateWarning'
$overlayBase = Parse-UiColorToken -content $styleTokens -tokenName 'kOverlayBase'

$ratioPrimary = Get-ContrastRatio $textPrimary $overlayBase
$ratioSecondary = Get-ContrastRatio $textSecondary $overlayBase
$ratioWarning = Get-ContrastRatio $warning $overlayBase

if ($ratioPrimary -lt 4.5) {
    Write-Error ("T14 integration check failed: text primary contrast ratio is below 4.5:1 (actual {0:N2}:1)." -f $ratioPrimary)
    exit 1
}
if ($ratioSecondary -lt 4.5) {
    Write-Error ("T14 integration check failed: text secondary contrast ratio is below 4.5:1 (actual {0:N2}:1)." -f $ratioSecondary)
    exit 1
}
if ($ratioWarning -lt 3.0) {
    Write-Error ("T14 integration check failed: warning/focus contrast ratio is below 3.0:1 (actual {0:N2}:1)." -f $ratioWarning)
    exit 1
}

# Rendered-state legibility checks on representative critical-screen primitives.
$stateDefault = Parse-UiColorToken -content $styleTokens -tokenName 'kStateDefault'
$stateCritical = Parse-UiColorToken -content $styleTokens -tokenName 'kStateCritical'

$renderedKeybindIndicatorRatio = Assert-RenderedContrast -Name 'keybinds-status-indicator-text' -TextColor $textPrimary -TextAlpha 0.92 -SurfaceColor $overlayBase -SurfaceAlpha 0.84 -BackdropColor $overlayBase -MinRatio 4.5
$renderedSettingsListRatio = Assert-RenderedContrast -Name 'settings-list-active-row-text' -TextColor $textPrimary -TextAlpha 1.0 -SurfaceColor $stateDefault -SurfaceAlpha 0.26 -BackdropColor $overlayBase -MinRatio 4.5
$renderedCriticalGuideRatio = Assert-RenderedContrast -Name 'guide-critical-indicator-stripe' -TextColor $stateCritical -TextAlpha 0.98 -SurfaceColor $overlayBase -SurfaceAlpha 0.84 -BackdropColor $overlayBase -MinRatio 3.0

$reducedMotionFragments = @(
    'bool reducedMotion=false;',
    'gUiReducedMotion = settings.reducedMotion;',
    'inline bool uiReducedMotionEnabled()',
    'if (uiReducedMotionEnabled()) return 0.0f;',
    'if (uiReducedMotionEnabled()) {',
    'uiMotionAmplitude(',
    'settings.reducedMotion?"ON":"OFF"'
)

foreach ($fragment in $reducedMotionFragments) {
    $found = $false
    if ($menu.IndexOf($fragment, [System.StringComparison]::Ordinal) -ge 0) { $found = $true }
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -ge 0) { $found = $true }
    if ($styleTokens.IndexOf($fragment, [System.StringComparison]::Ordinal) -ge 0) { $found = $true }
    if ($immersive.IndexOf($fragment, [System.StringComparison]::Ordinal) -ge 0) { $found = $true }
    if ($primitives.IndexOf($fragment, [System.StringComparison]::Ordinal) -ge 0) { $found = $true }

    if (-not $found) {
        Write-Error "T14 integration check failed: reduced-motion contract missing fragment -> $fragment"
        exit 1
    }
}

$navigationFragments = @(
    'inline bool menuGamepadUp()',
    'inline bool menuGamepadDown()',
    'inline bool menuGamepadLeft()',
    'inline bool menuGamepadRight()',
    'inline bool menuGamepadConfirm()',
    'inline bool menuGamepadBack()'
)
foreach ($fragment in $navigationFragments) {
    if ($inputCommon.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T14 integration check failed: controller helper missing in src/input_common.h -> $fragment"
        exit 1
    }
}

if ($inputMenu.IndexOf('menuGamepadConfirm()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadBack()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadUp()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputMenu.IndexOf('menuGamepadDown()', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T14 integration check failed: menu input lacks keyboard/controller navigation hooks in src/input_menu.h'
    exit 1
}

if ($inputSettings.IndexOf('menuGamepadConfirm()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadBack()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadLeft()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputSettings.IndexOf('menuGamepadRight()', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T14 integration check failed: settings input lacks keyboard/controller navigation hooks in src/input_settings.h'
    exit 1
}

if ($inputKeybinds.IndexOf('menuGamepadConfirm()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputKeybinds.IndexOf('menuGamepadBack()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputKeybinds.IndexOf('menuGamepadUp()', [System.StringComparison]::Ordinal) -lt 0 -or
    $inputKeybinds.IndexOf('menuGamepadDown()', [System.StringComparison]::Ordinal) -lt 0) {
    Write-Error 'T14 integration check failed: keybinds input lacks keyboard/controller navigation hooks in src/input_keybinds.h'
    exit 1
}

Write-Host ('[PASS] T14 accessibility baseline validated (token contrast primary {0:N2}:1, secondary {1:N2}:1, warning {2:N2}:1; rendered keybind indicator {3:N2}:1, settings list row {4:N2}:1, guide critical stripe {5:N2}:1)' -f $ratioPrimary, $ratioSecondary, $ratioWarning, $renderedKeybindIndicatorRatio, $renderedSettingsListRatio, $renderedCriticalGuideRatio)
exit 0
