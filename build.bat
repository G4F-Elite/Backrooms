@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Backrooms VHS Horror - Build System
echo ========================================
echo.

cd /d "%~dp0"

:: Run PowerShell checks inline
powershell -ExecutionPolicy Bypass -Command ^
$ErrorActionPreference = 'Stop'; ^
$hasError = $false; ^
$srcPath = '%~dp0src'; ^
^
Write-Host 'CODE STYLE REQUIREMENTS:' -ForegroundColor Cyan; ^
Write-Host '- Max 500 lines per file'; ^
Write-Host '- Max 150 chars per line'; ^
Write-Host '- Max 3 semicolons per line (5 for for loops)'; ^
Write-Host '- Min 5%% empty lines'; ^
Write-Host '- NO fake splitting (file_1, file_2...)'; ^
Write-Host '========================================'; ^
Write-Host ''; ^
^
$sourceFiles = Get-ChildItem -Path $srcPath -Include '*.cpp','*.c','*.h','*.hpp' -Recurse -ErrorAction SilentlyContinue; ^
if (-not $sourceFiles) { Write-Host 'ERROR: No source files in src/' -ForegroundColor Red; exit 1 }; ^
^
Write-Host '[1/4] Checking for fake file splitting...' -ForegroundColor Yellow; ^
$splitAbuse = 0; ^
$whitelist = @('vec2','vec3','vec4','mat2','mat3','mat4','mp3','wav','ogg','utf8','utf16','utf32','win32','win64','x64','x86','d3d9','d3d10','d3d11','d3d12','gl2','gl3','gl4','gles2','gles3','crc32','md5','sha1','sha256','base64','rgb8','rgba8','r32f'); ^
^
foreach ($file in $sourceFiles) { ^
    $name = $file.BaseName.ToLower(); ^
    $suspicious = $false; ^
    $reason = ''; ^
    ^
    if ($name -match '_\d+$') { $suspicious = $true; $reason = 'numbered suffix (_N)' }; ^
    elseif ($name -match '[a-z]\d$' -and $name -notin $whitelist) { $suspicious = $true; $reason = 'ends with digit' }; ^
    if ($name -match '(part^|chunk^|section^|piece^|segment^|frag^|block)\d*$') { $suspicious = $true; $reason = 'chunk/part pattern' }; ^
    if ($name -match '_[a-e]$') { $suspicious = $true; $reason = 'letter suffix (_A-E)' }; ^
    foreach ($word in @('continued','continue','cont','more','extra','additional','remaining','rest','_next','_prev','other')) { ^
        if ($name -match \"[\\._]$word`$\") { $suspicious = $true; $reason = \"continuation ($word)\"; break } ^
    }; ^
    if ($name -match '_(copy^|backup^|old^|new^|v\d^|ver\d)$') { $suspicious = $true; $reason = 'copy/version pattern' }; ^
    ^
    if ($suspicious) { Write-Host \"  SUSPICIOUS: $($file.Name) - $reason\" -ForegroundColor Red; $splitAbuse++ } ^
}; ^
^
$prefixes = @{}; ^
foreach ($file in $sourceFiles ^| Where-Object { $_.Extension -in '.cpp','.c' }) { ^
    if ($file.BaseName -match '^([a-zA-Z]+)_') { ^
        $prefix = $matches[1].ToLower(); ^
        if (-not $prefixes.ContainsKey($prefix)) { $prefixes[$prefix] = 0 }; ^
        $prefixes[$prefix]++ ^
    } ^
}; ^
foreach ($prefix in $prefixes.Keys) { ^
    if ($prefixes[$prefix] -gt 3) { Write-Host \"  SUSPICIOUS: $($prefixes[$prefix]) files with prefix '${prefix}_'\" -ForegroundColor Red; $splitAbuse++ } ^
}; ^
^
$cppCount = ($sourceFiles ^| Where-Object { $_.Extension -in '.cpp','.c' }).Count; ^
if ($cppCount -gt 25) { Write-Host \"  SUSPICIOUS: $cppCount source files - excessive?\" -ForegroundColor Red; $splitAbuse++ }; ^
^
if ($splitAbuse -gt 0) { Write-Host \"`n  ERROR: $splitAbuse signs of fake splitting!\" -ForegroundColor Red; $hasError = $true } ^
else { Write-Host '  OK - No fake splitting' -ForegroundColor Green }; ^
^
Write-Host ''; ^
Write-Host '[2/4] Checking file sizes (max 500 lines)...' -ForegroundColor Yellow; ^
foreach ($file in $sourceFiles) { ^
    $lines = (Get-Content $file.FullName -ErrorAction SilentlyContinue ^| Measure-Object -Line).Lines; ^
    if ($lines -gt 500) { Write-Host \"  ERROR: $($file.Name) = $lines lines [MAX 500]\" -ForegroundColor Red; $hasError = $true } ^
    else { Write-Host \"  OK: $($file.Name) = $lines lines\" -ForegroundColor Green } ^
}; ^
^
Write-Host ''; ^
Write-Host '[3/4] Checking for minified code...' -ForegroundColor Yellow; ^
foreach ($file in $sourceFiles) { ^
    $content = Get-Content $file.FullName -ErrorAction SilentlyContinue; ^
    if (-not $content) { continue }; ^
    $totalLines = $content.Count; ^
    $longLines = 0; $semicolonAbuse = 0; $emptyLines = 0; $veryLongLines = 0; ^
    ^
    foreach ($line in $content) { ^
        if ($line -match '^\s*$') { $emptyLines++; continue }; ^
        $len = $line.Length; ^
        if ($len -gt 150) { $longLines++ }; ^
        if ($len -gt 300) { $veryLongLines++ }; ^
        ^
        $cleanLine = $line -replace '//.*$','' -replace '\"[^\"]*\"','\"\"' -replace \"'[^']*'\",\"''\"; ^
        $sc = ($cleanLine.ToCharArray() ^| Where-Object { $_ -eq ';' }).Count; ^
        if ($line -match '\bfor\s*\(') { if ($sc -gt 5) { $semicolonAbuse++ } } ^
        else { if ($sc -gt 3) { $semicolonAbuse++ } } ^
    }; ^
    ^
    $emptyRatio = if ($totalLines -gt 0) { [math]::Round(($emptyLines / $totalLines) * 100, 1) } else { 0 }; ^
    $fileOK = $true; ^
    ^
    if ($longLines -gt 5) { Write-Host \"  ERROR: $($file.Name) - $longLines lines over 150 chars [MINIFIED]\" -ForegroundColor Red; $hasError = $true; $fileOK = $false }; ^
    if ($veryLongLines -gt 0) { Write-Host \"  ERROR: $($file.Name) - $veryLongLines lines over 300 chars [SEVERE]\" -ForegroundColor Red; $hasError = $true; $fileOK = $false }; ^
    if ($semicolonAbuse -gt 3) { Write-Host \"  ERROR: $($file.Name) - $semicolonAbuse lines with excess semicolons\" -ForegroundColor Red; $hasError = $true; $fileOK = $false }; ^
    if ($totalLines -gt 50 -and $emptyRatio -lt 5) { Write-Host \"  ERROR: $($file.Name) - only $emptyRatio%% empty lines [MINIFIED]\" -ForegroundColor Red; $hasError = $true; $fileOK = $false }; ^
    if ($fileOK) { Write-Host \"  OK: $($file.Name) ($emptyRatio%% whitespace)\" -ForegroundColor Green } ^
}; ^
^
Write-Host ''; ^
Write-Host '[4/4] Checking for obfuscation...' -ForegroundColor Yellow; ^
$obfuscationFound = $false; ^
foreach ($file in $sourceFiles) { ^
    $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue; ^
    if (-not $content) { continue }; ^
    ^
    $macroCount = ([regex]::Matches($content, '#define\s+\w+\s*\(')).Count; ^
    if ($macroCount -gt 30) { Write-Host \"  WARNING: $($file.Name) - $macroCount function macros\" -ForegroundColor Yellow }; ^
    ^
    $commaAbuse = ([regex]::Matches($content, '\([^()]*=[^()]*,[^()]*=[^()]*,[^()]*=[^()]*\)')).Count; ^
    if ($commaAbuse -gt 5) { Write-Host \"  ERROR: $($file.Name) - comma operator abuse\" -ForegroundColor Red; $hasError = $true; $obfuscationFound = $true }; ^
    ^
    $ternaryChains = ([regex]::Matches($content, '\?[^;]+\?[^;]+\?')).Count; ^
    if ($ternaryChains -gt 3) { Write-Host \"  ERROR: $($file.Name) - nested ternary abuse\" -ForegroundColor Red; $hasError = $true; $obfuscationFound = $true }; ^
    ^
    $singleLetterVars = ([regex]::Matches($content, '\b(int^|float^|double^|char^|bool^|auto)\s+[a-z]\s*[,;=]')).Count; ^
    if ($singleLetterVars -gt 20) { Write-Host \"  WARNING: $($file.Name) - $singleLetterVars single-letter vars\" -ForegroundColor Yellow } ^
}; ^
if (-not $obfuscationFound) { Write-Host '  OK - No obfuscation detected' -ForegroundColor Green }; ^
^
Write-Host ''; ^
Write-Host '========================================'; ^
if ($hasError) { ^
    Write-Host 'BUILD REFUSED - Code quality issues!' -ForegroundColor Red; ^
    Write-Host '========================================'; ^
    Write-Host ''; ^
    Write-Host 'RULES:' -ForegroundColor Yellow; ^
    Write-Host '  - Max 500 lines per file'; ^
    Write-Host '  - Max 150 chars per line'; ^
    Write-Host '  - Max 3 semicolons per line'; ^
    Write-Host '  - Min 5%% empty lines'; ^
    Write-Host '  - No numbered files (file_1, file_2)'; ^
    Write-Host ''; ^
    Write-Host 'PROPER MODULES:' -ForegroundColor Yellow; ^
    Write-Host '  net.cpp player.cpp render.cpp'; ^
    Write-Host '  audio.cpp world.cpp menu.cpp'; ^
    Write-Host '  input.cpp utils.cpp'; ^
    Write-Host ''; ^
    Write-Host 'Each file = ONE responsibility!' -ForegroundColor Cyan; ^
    exit 1 ^
} else { ^
    Write-Host 'All checks passed!' -ForegroundColor Green; ^
    Write-Host '========================================'; ^
    exit 0 ^
}

if !errorlevel! neq 0 (
    echo.
    echo Build cancelled.
    exit /b 1
)

:: ==========================================
:: BUILD
:: ==========================================
if not exist "build" mkdir build

echo.
echo Compiling...
g++ -std=c++17 -O2 -Wall -Wno-unused-result -Wno-unknown-pragmas -mwindows -static -static-libgcc -static-libstdc++ ^
    -I"deps/include" ^
    -o "build/backrooms.exe" ^
    "src/game.cpp" ^
    "deps/src/glad.c" ^
    -L"deps/lib" ^
    -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32

if !errorlevel! neq 0 (
    echo.
    echo Build FAILED!
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo Run: build\backrooms.exe
exit /b 0