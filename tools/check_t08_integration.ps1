$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$hudPath = Join-Path $repoRoot 'src\hud.h'

if (-not (Test-Path $hudPath)) {
    Write-Error "Missing expected file: $hudPath"
    exit 1
}

$hudContent = Get-Content -Raw -Path $hudPath

$hasInclude = $hudContent -match '#include\s+"ui_primitives\.h"'
if (-not $hasInclude) {
    Write-Error 'T08 integration check failed: src/hud.h no longer includes ui_primitives.h'
    exit 1
}

$pattern = 'inline\s+void\s+drawNewUI\s*\(\s*\)\s*\{\s*if\s*\(\s*gameState\s*==\s*STATE_MENU\s*\)\s*\{\s*drawUiPrimitiveMainMenuSample\s*\(\s*vhsTime\s*\)\s*;\s*return\s*;\s*\}\s*drawUiParityPath\s*\(\s*\)\s*;\s*\}'
$hasIntegrationContract = [regex]::IsMatch($hudContent, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)

if (-not $hasIntegrationContract) {
    Write-Error 'T08 integration check failed: drawNewUI() no longer enforces the menu->primitives sample contract with early return'
    exit 1
}

Write-Host '[PASS] T08 integration contract validated in src/hud.h'
exit 0