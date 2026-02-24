$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$hudPath = Join-Path $repoRoot 'src\hud.h'
$gameMainPath = Join-Path $repoRoot 'src\game_main_entry.h'
$inputMenuPath = Join-Path $repoRoot 'src\input_menu.h'
$sessionPath = Join-Path $repoRoot 'src\session.h'
$immersiveMenuPath = Join-Path $repoRoot 'src\ui_menu_immersive.h'
$backlogPath = Join-Path $repoRoot 'BACKLOG.md'

foreach ($path in @($hudPath, $gameMainPath, $inputMenuPath, $sessionPath, $immersiveMenuPath, $backlogPath)) {
    if (-not (Test-Path $path)) {
        Write-Error "Missing expected file: $path"
        exit 1
    }
}

$hud = Get-Content -Raw -Path $hudPath
$gameMain = Get-Content -Raw -Path $gameMainPath
$inputMenu = Get-Content -Raw -Path $inputMenuPath
$session = Get-Content -Raw -Path $sessionPath
$immersiveMenu = Get-Content -Raw -Path $immersiveMenuPath
$backlog = Get-Content -Raw -Path $backlogPath

$requiredParityFragments = @(
    'buildImmersiveHudObjectiveLines(char* primary, int primarySize, char* support, int supportSize)',
    'if(multiState==MULTI_IN_GAME){',
    'CO-OP: SWITCHES %d/2',
    'if(multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime())){',
    'NETWORK UNSTABLE - RECONNECT MAY OCCUR'
)
foreach ($fragment in $requiredParityFragments) {
    if ($hud.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T16 integration check failed: host/client parity fragment missing in src/hud.h -> $fragment"
        exit 1
    }
}

$drawMultiHudPattern = 'if\s*\(\s*multiState\s*==\s*MULTI_IN_GAME\s*\)\s*drawMultiHUD\s*\(\s*netMgr\.getPlayerCount\s*\(\s*\)\s*,\s*netMgr\.isHost\s*\)\s*;'
$hasDrawMultiHud = [regex]::IsMatch($hud, $drawMultiHudPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $hasDrawMultiHud) {
    Write-Error 'T16 integration check failed: drawMultiHUD host/client parity call missing in src/hud.h'
    exit 1
}

$requiredReconnectFlowFragments = @(
    'if(gameState==STATE_MULTI_WAIT && reconnectInProgress){',
    'reconnectAttemptTimer = nextReconnectDelaySeconds(reconnectAttempts);',
    'if(shouldContinueReconnect(reconnectAttempts, 12) && lastSession.valid){',
    'restoreSessionSnapshot();',
    'resetReconnectState(true);'
)
foreach ($fragment in $requiredReconnectFlowFragments) {
    if ($gameMain.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T16 integration check failed: reconnect/session transition fragment missing in src/game_main_entry.h -> $fragment"
        exit 1
    }
}

$requiredWaitExitResets = @(
    'else if (gameState == STATE_MULTI_WAIT)',
    'if (esc && !escPressed)',
    'resetReconnectState(true);',
    'if (sincePacket > 6.0f) {'
)
foreach ($fragment in $requiredWaitExitResets) {
    if ($inputMenu.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T16 integration check failed: wait-state transition/reset fragment missing in src/input_menu.h -> $fragment"
        exit 1
    }
}

$resetHelperPattern = 'inline\s+void\s+resetReconnectState\s*\(\s*bool\s+clearSnapshot\s*\)\s*\{\s*reconnectInProgress\s*=\s*false\s*;\s*restoreAfterReconnect\s*=\s*false\s*;\s*reconnectAttemptTimer\s*=\s*0\.0f\s*;\s*reconnectAttempts\s*=\s*0\s*;\s*if\s*\(\s*clearSnapshot\s*\)\s*lastSession\.valid\s*=\s*false\s*;\s*\}'
$hasResetHelper = [regex]::IsMatch($session, $resetHelperPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $hasResetHelper) {
    Write-Error 'T16 integration check failed: resetReconnectState(bool) helper contract missing in src/session.h'
    exit 1
}

$requiredImmersiveMenuStates = @(
    'if (gameState == STATE_MULTI) {',
    'if (gameState == STATE_MULTI_JOIN) {',
    'if (gameState == STATE_MULTI_HOST) {',
    'if (gameState == STATE_MULTI_WAIT) {'
)
foreach ($fragment in $requiredImmersiveMenuStates) {
    if ($immersiveMenu.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
        Write-Error "T16 integration check failed: immersive multiplayer menu state missing in src/ui_menu_immersive.h -> $fragment"
        exit 1
    }
}

$criticalDesyncOpenPattern = '(?im)^\s*[-*]\s*\[\s*\]\s*.*desync|(?im)^\s*todo\s*[:\-].*desync|(?im)critical\s+desync\s+bug'
if ([regex]::IsMatch($backlog, $criticalDesyncOpenPattern)) {
    Write-Error 'T16 integration check failed: potential open critical desync issue found in BACKLOG.md'
    exit 1
}

Write-Host '[PASS] T16 multiplayer + net-state UI consistency contract validated'
exit 0
