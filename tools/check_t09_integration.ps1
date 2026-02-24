$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$tokensPath = Join-Path $repoRoot 'src\ui_style_tokens.h'
$primitivesPath = Join-Path $repoRoot 'src\ui_primitives.h'
$hudPath = Join-Path $repoRoot 'src\hud.h'

foreach ($path in @($tokensPath, $primitivesPath, $hudPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$tokens = Get-Content -Raw -Path $tokensPath
$primitives = Get-Content -Raw -Path $primitivesPath
$hud = Get-Content -Raw -Path $hudPath

$requiredTokenFragments = @(
    'enum UiFontRole',
    'struct UiFontChain',
    'inline UiFontChain uiFontChainForRole(UiFontRole role)',
    'inline float uiClampTypographyScale(float requestedScale, UiFontRole role)',
    'namespace UiIconography'
)

foreach ($fragment in $requiredTokenFragments) {
    if ($tokens.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T09 integration check failed: missing token contract fragment in src/ui_style_tokens.h -> $fragment"
        exit 1
    }
}

$requiredPrimitiveFragments = @(
    'struct UiFontPipelineState',
    'struct UiIconAtlasState',
    'inline void uiInitFontPipeline()',
    'inline void uiInitIconAtlasScaffold()',
    'inline void uiPrimitiveIcon(',
    'uiPrimitiveIcon(UI_ICON_GLYPH_NETWORK',
    'uiPrimitiveIcon(UI_ICON_GLYPH_OBJECTIVE'
)

foreach ($fragment in $requiredPrimitiveFragments) {
    if ($primitives.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T09 integration check failed: missing primitive scaffold fragment in src/ui_primitives.h -> $fragment"
        exit 1
    }
}

$menuContractPattern = 'inline\s+void\s+drawNewUI\s*\(\s*\)\s*\{\s*if\s*\(\s*gameState\s*==\s*STATE_MENU\s*\)\s*\{\s*drawUiPrimitiveMainMenuSample\s*\(\s*vhsTime\s*\)\s*;\s*return\s*;\s*\}\s*drawUiParityPath\s*\(\s*\)\s*;\s*\}'
$menuContractOk = [regex]::IsMatch($hud, $menuContractPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)

if (-not $menuContractOk) {
    Write-Error 'T09 integration check failed: drawNewUI() menu integration contract regressed in src/hud.h'
    exit 1
}

Write-Host '[PASS] T09 font pipeline + icon atlas scaffold contract validated'
exit 0
