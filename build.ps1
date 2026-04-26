$ErrorActionPreference = "Continue"

# Configuration
$SourceDir = $PSScriptRoot
$BuildDir = "C:\Users\egavi\pio_temp_build\TechTest_v2"
$EnvName = "esp32-s3-devkitc-1"

# Logs live next to the build cache (outside OneDrive). Timestamped + alias to latest.
$LogDir = Join-Path $BuildDir '.logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
$LogStamp = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$LogFile = Join-Path $LogDir "build_$LogStamp.txt"
$LogLatest = Join-Path $LogDir 'build_latest.txt'

Write-Host ""
Write-Host "=== BUILD ESP32S3 Tech Test ===" -ForegroundColor Cyan
Write-Host "Source: $SourceDir"
Write-Host "Build:  $BuildDir"
Write-Host "Log:    $LogFile"
Write-Host ""

# 1. Prepare Target Directory (preserve .pio cache for incremental builds)
Write-Host "[1/3] Preparing build directory..." -ForegroundColor Yellow
if (Test-Path $BuildDir) {
    # Remove everything EXCEPT .pio folder (build cache) and .logs (log history)
    Get-ChildItem -Path $BuildDir -Exclude ".pio", ".logs" | Remove-Item -Recurse -Force
}
else {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

# 2. Copy Project Files (skip build artifacts, scripts, IDE config)
Write-Host "[2/3] Copying project files..." -ForegroundColor Yellow

$Exclude = @(".pio", ".git", "build.ps1", "upload.ps1", "monitor.ps1", ".vscode")
Get-ChildItem -Path $SourceDir -Exclude $Exclude | Copy-Item -Destination $BuildDir -Recurse -Force

# 3. Build
Write-Host "[3/3] Compiling firmware..." -ForegroundColor Yellow

Push-Location $BuildDir
try {
    # Locate PlatformIO
    $PIO = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
    if (-not (Test-Path $PIO)) {
        $PIO = "platformio" # Try PATH
    }

    # Disable Component Manager to avoid online lookups/issues
    $env:IDF_COMPONENT_MANAGER = "0"

    # Run PlatformIO: full output goes to log file only
    $startTime = Get-Date
    & $PIO run --environment $EnvName *> $LogFile
    $exitCode = $LASTEXITCODE
    $elapsed = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)

    if ($exitCode -eq 0) {
        # Extract memory usage from log (the interesting lines)
        $ramLine = Select-String -Path $LogFile -Pattern "^RAM:" | Select-Object -Last 1
        $flashLine = Select-String -Path $LogFile -Pattern "^Flash:" | Select-Object -Last 1

        Write-Host ""
        Write-Host "BUILD SUCCESS ($elapsed s)" -ForegroundColor Green
        if ($ramLine) { Write-Host "  $($ramLine.Line)" -ForegroundColor Gray }
        if ($flashLine) { Write-Host "  $($flashLine.Line)" -ForegroundColor Gray }
        Write-Host "  Log: $LogFile" -ForegroundColor DarkGray
    }
    else {
        # Extract only error lines from log to show on screen
        Write-Host ""
        Write-Host "BUILD FAILED ($elapsed s) - Exit code: $exitCode" -ForegroundColor Red
        Write-Host ""
        Write-Host "--- Errors ---" -ForegroundColor Red
        $errors = Select-String -Path $LogFile -Pattern "error:" -CaseSensitive
        if ($errors) {
            $errors | ForEach-Object { Write-Host "  $($_.Line.Trim())" -ForegroundColor Red }
        }
        else {
            # If no 'error:' found, show last 15 lines as fallback
            Get-Content $LogFile | Select-Object -Last 15 | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
        }
        Write-Host ""
        Write-Host "  Full log: $LogFile" -ForegroundColor Yellow
        exit $exitCode
    }
}
catch {
    Write-Host "EXCEPTION: $_" -ForegroundColor Red
}
finally {
    Pop-Location
    # Update _latest alias and rotate (keep last 10 builds)
    if (Test-Path $LogFile) {
        Copy-Item -LiteralPath $LogFile -Destination $LogLatest -Force -ErrorAction SilentlyContinue
        Get-ChildItem -Path $LogDir -Filter 'build_2*.txt' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip 10 |
        Remove-Item -Force -ErrorAction SilentlyContinue
    }
}
