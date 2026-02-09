param(
    [string]$SrcPath = "src",
    [int]$MaxLines = 500
)

$ErrorActionPreference = "Stop"

Write-Host "CODE STYLE REQUIREMENTS:" -ForegroundColor Cyan
Write-Host "- Max $MaxLines lines per file"
Write-Host "========================================"
Write-Host ""

if (-not (Test-Path $SrcPath)) {
    Write-Host "ERROR: Source path '$SrcPath' not found." -ForegroundColor Red
    exit 1
}

$sourceFiles = Get-ChildItem -Path $SrcPath -Include *.cpp,*.c,*.h,*.hpp -Recurse -File
if (-not $sourceFiles -or $sourceFiles.Count -eq 0) {
    Write-Host "ERROR: No source files found in '$SrcPath'." -ForegroundColor Red
    exit 1
}

$hasError = $false
Write-Host "[1/1] Checking file sizes..." -ForegroundColor Yellow

foreach ($file in $sourceFiles) {
    $lineCount = (Get-Content $file.FullName | Measure-Object -Line).Lines
    if ($lineCount -gt $MaxLines) {
        Write-Host ("  ERROR: {0} = {1} lines [MAX {2}]" -f $file.Name, $lineCount, $MaxLines) -ForegroundColor Red
        $hasError = $true
    } else {
        Write-Host ("  OK: {0} = {1} lines" -f $file.Name, $lineCount) -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "========================================"
if ($hasError) {
    Write-Host "BUILD REFUSED - line limit violations found." -ForegroundColor Red
    Write-Host "========================================"
    exit 1
}

Write-Host "All checks passed!" -ForegroundColor Green
Write-Host "========================================"
exit 0
