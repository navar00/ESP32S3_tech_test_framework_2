# Serial Monitor with timestamped log capture
# Usage:
#   ./monitor.ps1            # default 115200, last used port
#   ./monitor.ps1 -Port COM7
#   ./monitor.ps1 -NoTee     # disable log capture (live only)

param(
    [string] $Port = "",
    [int]    $Baud = 115200,
    [switch] $NoTee
)

$ErrorActionPreference = "Continue"

$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
$BuildDir = "C:\Users\egavi\pio_temp_build\ESP32S3-TFT_Framework"
$LogDir = Join-Path $BuildDir '.logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
$Stamp = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$LogFile = Join-Path $LogDir "monitor_$Stamp.log"
$LogLatest = Join-Path $LogDir 'monitor_latest.log'

if (-not (Test-Path $PIO)) {
    Write-Host "ERROR: PlatformIO not found at $PIO" -ForegroundColor Red
    exit 1
}

$portArgs = if ($Port) { @('--port', $Port) } else { @() }

Write-Host ""
Write-Host "=== SERIAL MONITOR ===" -ForegroundColor Cyan
Write-Host "Build:  $BuildDir"
if ($Port) { Write-Host "Port:   $Port" }
Write-Host "Baud:   $Baud"
if (-not $NoTee) { Write-Host "Log:    $LogFile" }
Write-Host "(Ctrl+C to exit)"
Write-Host ""

try {
    if ($NoTee) {
        & $PIO device monitor --project-dir "$BuildDir" --baud $Baud @portArgs
    }
    else {
        # Tee live output to console AND timestamped log
        & $PIO device monitor --project-dir "$BuildDir" --baud $Baud @portArgs 2>&1 |
        Tee-Object -FilePath $LogFile
    }
}
finally {
    if ((-not $NoTee) -and (Test-Path $LogFile)) {
        Copy-Item -LiteralPath $LogFile -Destination $LogLatest -Force -ErrorAction SilentlyContinue
        # Rotate: keep last 10 monitor logs
        Get-ChildItem -Path $LogDir -Filter 'monitor_2*.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip 10 |
        Remove-Item -Force -ErrorAction SilentlyContinue
        Write-Host ""
        Write-Host "Captured: $LogFile" -ForegroundColor Green
    }
}
