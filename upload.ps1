# Upload & Monitor script for ESP32S3 Tech Test
# Reuses build artifacts from build.ps1 (no recompilation needed)
# Usage: ./upload.ps1

$ErrorActionPreference = "Continue"

$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
$ProjectDir = $PSScriptRoot
$BuildDir = "C:\Users\egavi\pio_temp_build\TechTest_v2"
$EnvName = "esp32-s3-devkitc-1"
$FirmwareBin = "$BuildDir\.pio\build\$EnvName\firmware.bin"

# Logs next to build cache (outside OneDrive). Timestamped + _latest alias.
$LogDir = Join-Path $BuildDir '.logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
$LogStamp = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$LogFile = Join-Path $LogDir "upload_$LogStamp.txt"
$LogLatest = Join-Path $LogDir 'upload_latest.txt'

Write-Host ""
Write-Host "=== UPLOAD ESP32S3 Tech Test ===" -ForegroundColor Cyan
Write-Host "Build:  $BuildDir"
Write-Host "Log:    $LogFile"
Write-Host ""

if (-not (Test-Path $PIO)) {
    Write-Host "ERROR: PlatformIO not found at $PIO" -ForegroundColor Red
    exit 1
}

# Check that build artifacts exist
if (-not (Test-Path $FirmwareBin)) {
    Write-Host "ERROR: No firmware found. Run build.ps1 first." -ForegroundColor Red
    Write-Host "  Expected: $FirmwareBin" -ForegroundColor Yellow
    exit 1
}

$fwSize = [math]::Round((Get-Item $FirmwareBin).Length / 1024, 1)
Write-Host "Firmware: $fwSize KB" -ForegroundColor DarkGray
Write-Host ""

# --- Step 1: Upload (from build dir, no recompilation) ---
Write-Host "[1/2] Uploading firmware..." -ForegroundColor Yellow
$startTime = Get-Date
& $PIO run --project-dir "$BuildDir" --environment $EnvName --target upload *> $LogFile
$exitCode = $LASTEXITCODE
$elapsed = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)

# Update _latest alias and rotate (keep last 10) regardless of success/failure
if (Test-Path $LogFile) {
    Copy-Item -LiteralPath $LogFile -Destination $LogLatest -Force -ErrorAction SilentlyContinue
    Get-ChildItem -Path $LogDir -Filter 'upload_2*.txt' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -Skip 10 |
    Remove-Item -Force -ErrorAction SilentlyContinue
}

if ($exitCode -ne 0) {
    Write-Host ""
    Write-Host "UPLOAD FAILED ($elapsed s) - Exit code: $exitCode" -ForegroundColor Red
    Write-Host ""
    Write-Host "--- Errors ---" -ForegroundColor Red
    $errors = Select-String -Path $LogFile -Pattern "error:" -CaseSensitive
    if ($errors) {
        $errors | ForEach-Object { Write-Host "  $($_.Line.Trim())" -ForegroundColor Red }
    }
    else {
        Get-Content $LogFile | Select-Object -Last 15 | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
    }
    Write-Host ""
    Write-Host "  Full log: $LogFile" -ForegroundColor Yellow
    exit $exitCode
}

# Extract port from log
$portLine = Select-String -Path $LogFile -Pattern "Serial port (COM\d+)" | Select-Object -Last 1
$port = if ($portLine -and $portLine.Matches) { $portLine.Matches[0].Groups[1].Value } else { "COM4" }

Write-Host "UPLOAD SUCCESS ($elapsed s) -> $port" -ForegroundColor Green
Write-Host "  Log: $LogFile" -ForegroundColor DarkGray
Write-Host ""


# --- Step 2: Serial Monitor ---
Write-Host "[2/2] Serial Monitor on $port (Ctrl+C to exit)" -ForegroundColor Yellow
Write-Host ""
& $PIO device monitor --project-dir "$BuildDir" --baud 115200
