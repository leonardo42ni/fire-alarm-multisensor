[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projects = @('node_h2', 'gw_h2', 'gw_host')

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    Write-Host '[FAIL] Khong tim thay idf.py. Hay chay export.ps1 cua ESP-IDF truoc.' -ForegroundColor Red
    exit 1
}

foreach ($name in $projects) {
    $projectDir = Join-Path $repoRoot (Join-Path 'firmware' $name)
    if (-not (Test-Path -LiteralPath $projectDir)) {
        Write-Host "[FAIL] Thieu thu muc: $projectDir" -ForegroundColor Red
        exit 1
    }

    Write-Host "=== [build] $name ===" -ForegroundColor Cyan
    Push-Location -LiteralPath $projectDir
    try {
        & idf.py build
        $exitCode = $LASTEXITCODE
    }
    catch {
        Write-Host "[FAIL] $name : $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        Write-Host "[FAIL] $name build exit=$exitCode" -ForegroundColor Red
        exit 1
    }

    Write-Host "[PASS] $name" -ForegroundColor Green
}

Write-Host '[PASS] Ca 3 firmware build thanh cong.' -ForegroundColor Green
exit 0
