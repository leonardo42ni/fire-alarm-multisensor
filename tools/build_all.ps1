[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projects = @('node_h2', 'gw_h2', 'gw_host')

# Uu tien python cua venv ESP-IDF de ham idf.py dung dung interpreter co 'click'
if ($env:IDF_PYTHON_ENV_PATH) {
    $venvScripts = Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts'
    if (Test-Path -LiteralPath $venvScripts) {
        $env:PATH = "$venvScripts;$env:PATH"
    }
}

# Moi project co target rieng trong sdkconfig; IDF_TARGET toan cuc gay mismatch
if ($env:IDF_TARGET) {
    Write-Host "[INFO] Bo IDF_TARGET=$env:IDF_TARGET de build theo target tung project." -ForegroundColor Yellow
    Remove-Item Env:IDF_TARGET -ErrorAction SilentlyContinue
}

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
