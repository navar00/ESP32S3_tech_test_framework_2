#requires -Version 5.1
<#
.SYNOPSIS
    Scaffolds a new project from the ESP32S3-TFT Framework
    (engineering sandbox for ESP32-S3 Lolin S3 Pro + ILI9341).

.DESCRIPTION
    Copies a curated subset of files from this framework, rewrites placeholders
    (project name, build dir, fw version), and emits a clean .vscode/.github layout.

    Hardware target: ESP32-S3 Lolin S3 Pro (pinout reference) flashed via the
    'esp32-s3-devkitc-1' PlatformIO board ID. The DevKitC-1 board ID is kept on
    purpose to avoid a known naming conflict between the lolin_s3 board variant
    and the TFT_eSPI pin macros.

.PARAMETER ProjectName
    Logical project name (e.g. ESP32S3_robot_v1).

.PARAMETER ProjectPath
    Destination path. Will be created if missing.

.PARAMETER BuildDirName
    Folder name under C:\Users\<user>\pio_temp_build\ (avoid OneDrive sync).

.PARAMETER FwVersion
    Firmware version string written to Config.h. Default 0.1.0.

.PARAMETER Presets
    Array of optional presets. Valid values: wifi, web, ble-input, ble-scan, gol, clocks, palettes.

.PARAMETER InitGit
    If set, runs git init + first commit.

.EXAMPLE
    .\scaffold.ps1 -ProjectName ESP32S3_robot_v1 `
                   -ProjectPath "C:\Users\egavi\OneDrive\Documents\PlatformIO\Projects\ESP32S3_robot_v1" `
                   -BuildDirName Robot_v1 `
                   -Presets @('wifi','web','clocks') `
                   -InitGit
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory)] [string] $ProjectName,
    [Parameter(Mandatory)] [string] $ProjectPath,
    [Parameter(Mandatory)] [string] $BuildDirName,
    [string] $FwVersion = '0.1.0',
    [ValidateSet('wifi', 'web', 'ble-input', 'ble-scan', 'gol', 'clocks', 'palettes')]
    [string[]] $Presets = @(),
    [switch] $InitGit
)

# Avoid SupportsShouldProcess propagating -WhatIf to safe write cmdlets
$PSDefaultParameterValues['Set-Content:WhatIf'] = $false
$PSDefaultParameterValues['Add-Content:WhatIf'] = $false
$PSDefaultParameterValues['Out-File:WhatIf'] = $false
$PSDefaultParameterValues['Copy-Item:WhatIf'] = $false
$PSDefaultParameterValues['New-Item:WhatIf'] = $false

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# ---------------------------------------------------------------------------
# Resolve source root (framework root): this script lives at
# <SOURCE>/.github/skills/scaffold-new-project/scaffold.ps1
# ---------------------------------------------------------------------------
$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\')).Path.TrimEnd('\')
Write-Host "[scaffold] Source: $SourceRoot" -ForegroundColor Cyan
Write-Host "[scaffold] Target: $ProjectPath" -ForegroundColor Cyan
Write-Host "[scaffold] Presets: $($Presets -join ', ')" -ForegroundColor Cyan

if (Test-Path $ProjectPath) {
    $existing = Get-ChildItem -Path $ProjectPath -Force -ErrorAction SilentlyContinue
    if ($existing -and $existing.Count -gt 0) {
        throw "Target path '$ProjectPath' already exists and is not empty. Aborting."
    }
}
else {
    if ($PSCmdlet.ShouldProcess($ProjectPath, 'Create directory')) {
        New-Item -ItemType Directory -Path $ProjectPath -Force | Out-Null
    }
}

# ---------------------------------------------------------------------------
# Manifest: which files to copy per preset
# Paths are relative to $SourceRoot
# ---------------------------------------------------------------------------
$ManifestBase = @(
    'src\main.cpp',
    'src\core\Config.h',
    'src\core\Logger.h',
    'src\core\ScreenManager.h',
    'src\core\BootOrchestrator.h',
    'src\core\BootOrchestrator.cpp',
    'src\core\WatchdogManager.h',
    'src\hal\Hal.h',
    'src\hal\DisplayHAL.h',
    'src\hal\DisplayHAL.cpp',
    'src\hal\LedHAL.h',
    'src\hal\LedHAL.cpp',
    'src\hal\WatchdogHAL.h',
    'src\hal\WatchdogHAL.cpp',
    'src\hal\StorageHAL.h',
    'src\hal\StorageHAL.cpp',
    'src\screens\IScreen.h',
    'src\screens\BaseSprite.h',
    'src\screens\BootConsoleScreen.h',
    'src\screens\BootConsoleScreen.cpp',
    'src\screens\ScreenStatus.h',
    'platformio.ini',
    'build.ps1',
    'upload.ps1',
    'DevGuidelines.md'
)

$ManifestPresets = @{
    'wifi'      = @(
        'src\services\NetService.h',
        'src\services\NetService.cpp',
        'src\services\TimeService.h',
        'src\services\TimeService.cpp',
        'src\services\GeoService.h',
        'src\services\GeoService.cpp'
    )
    'web'       = @(
        'src\services\WebService.h',
        'src\services\WebService.cpp',
        'src\core\GOLConfig.h'
    )
    'ble-input' = @(
        'src\hal\InputHAL.h',
        'src\hal\InputHAL.cpp',
        'src\screens\ScreenGamepad.h'
    )
    'ble-scan'  = @(
        'src\screens\ScreenBLEScan.h'
    )
    'gol'       = @(
        'src\screens\ScreenGameOfLife.h'
    )
    'clocks'    = @(
        'src\screens\ScreenAnalogClock.h',
        'src\screens\ScreenFlipClock.h'
    )
    'palettes'  = @(
        'src\screens\ScreenPalette.h',
        'src\core\PalettesData.h'
    )
}

# Web preset implies wifi (WebService needs net)
if ($Presets -contains 'web' -and -not ($Presets -contains 'wifi')) {
    Write-Host "[scaffold] Web preset implies wifi — auto-enabling." -ForegroundColor Yellow
    $Presets = @($Presets) + 'wifi'
}

# ---------------------------------------------------------------------------
# Build full file list
# ---------------------------------------------------------------------------
$FilesToCopy = New-Object System.Collections.Generic.List[string]
$ManifestBase | ForEach-Object { $FilesToCopy.Add($_) }
foreach ($p in $Presets) {
    if ($ManifestPresets.ContainsKey($p)) {
        $ManifestPresets[$p] | ForEach-Object { $FilesToCopy.Add($_) }
    }
}

# ---------------------------------------------------------------------------
# Copy files
# ---------------------------------------------------------------------------
foreach ($rel in $FilesToCopy) {
    $src = Join-Path $SourceRoot $rel
    if (-not (Test-Path $src)) {
        Write-Host "[scaffold] WARN: missing source $rel — skipped." -ForegroundColor Yellow
        continue
    }
    $dst = Join-Path $ProjectPath $rel
    $dstDir = Split-Path $dst -Parent
    if (-not (Test-Path $dstDir)) {
        New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
    }
    Copy-Item -Path $src -Destination $dst -Force
}

# Empty include/, lib/, test/, aux_/
foreach ($d in 'include', 'lib', 'test', 'aux_') {
    $p = Join-Path $ProjectPath $d
    if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p -Force | Out-Null }
}

# Palettes: also create include/colores/
if ($Presets -contains 'palettes') {
    $cp = Join-Path $ProjectPath 'include\colores'
    if (-not (Test-Path $cp)) { New-Item -ItemType Directory -Path $cp -Force | Out-Null }
}

# ---------------------------------------------------------------------------
# Rename Config.h -> Config_Template.h (avoid leaking credentials)
# ---------------------------------------------------------------------------
$cfgPath = Join-Path $ProjectPath 'src\core\Config.h'
$cfgTpl = Join-Path $ProjectPath 'src\core\Config_Template.h'
if (Test-Path $cfgPath) {
    # Inject SYS_VERSION before renaming (the constant is named SYS_VERSION in this framework)
    $content = Get-Content -Raw -LiteralPath $cfgPath
    $content = $content -replace 'SYS_VERSION\s*\[\]\s*=\s*"[^"]*"', "SYS_VERSION[] = `"$FwVersion`""
    Set-Content -LiteralPath $cfgPath -Value $content -NoNewline
    if (Test-Path $cfgTpl) { Remove-Item -LiteralPath $cfgTpl -Force }
    Rename-Item -LiteralPath $cfgPath -NewName 'Config_Template.h'
}

# ---------------------------------------------------------------------------
# Rewrite build.ps1 / upload.ps1 placeholders ($BuildDir name)
# ---------------------------------------------------------------------------
foreach ($script in 'build.ps1', 'upload.ps1') {
    $sp = Join-Path $ProjectPath $script
    if (-not (Test-Path $sp)) { continue }
    $txt = Get-Content -Raw -LiteralPath $sp
    $txt = $txt -replace 'TechTest_v2', $BuildDirName
    Set-Content -LiteralPath $sp -Value $txt -NoNewline
}

# ---------------------------------------------------------------------------
# Generate .gitignore
# ---------------------------------------------------------------------------
@'
.pio/
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch
src/core/Config.h
*.log
build_debug_log.txt
upload_log.txt
pio_monitor.log
_port_backup_*/
'@ | Set-Content -LiteralPath (Join-Path $ProjectPath '.gitignore') -Encoding UTF8

# ---------------------------------------------------------------------------
# Generate minimal .vscode/
# ---------------------------------------------------------------------------
$vscDir = Join-Path $ProjectPath '.vscode'
New-Item -ItemType Directory -Path $vscDir -Force | Out-Null

@"
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build",
      "type": "shell",
      "command": "./build.ps1",
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": []
    },
    {
      "label": "Upload",
      "type": "shell",
      "command": "./upload.ps1",
      "problemMatcher": []
    }
  ]
}
"@ | Set-Content -LiteralPath (Join-Path $vscDir 'tasks.json') -Encoding UTF8

@"
{
  "recommendations": [
    "platformio.platformio-ide",
    "ms-vscode.cpptools"
  ]
}
"@ | Set-Content -LiteralPath (Join-Path $vscDir 'extensions.json') -Encoding UTF8

# ---------------------------------------------------------------------------
# Generate .github/copilot-instructions.md (project-specific subset)
# ---------------------------------------------------------------------------
$ghDir = Join-Path $ProjectPath '.github'
New-Item -ItemType Directory -Path $ghDir -Force | Out-Null

$presetList = if ($Presets.Count -gt 0) { $Presets -join ', ' } else { 'core only' }

# Use single-quoted here-string + explicit token replacement to preserve backticks in markdown code spans
$ciTemplate = @'
# {{PROJECT_NAME}} — Copilot instructions

Scaffolded from **ESP32S3-TFT Framework** — *Engineering sandbox for ESP32-S3 Lolin S3 Pro + ILI9341* (evolución de un sandbox previo).
Presets: {{PRESETS}}.

## Stack
- **Hardware target:** ESP32-S3 Lolin S3 Pro + TFT ILI9341 240x320.
- **PlatformIO board:** `esp32-s3-devkitc-1` (mantenido a propósito: el variant `lolin_s3` colisiona con los macros de pines de TFT_eSPI).
- TFT en HSPI 27 MHz: MOSI 11 / SCLK 12 / MISO 13 / CS 48 / DC 47 / RST 21.
- LED RGB GPIO 38, Botón BOOT GPIO 0.
- PSRAM deshabilitada por defecto.

## Capas
1. `src/hal/` — singletons HAL (Display, Led, Watchdog, Storage).
2. `src/core/` — Logger, ScreenManager, BootOrchestrator, Config.
3. `src/services/` — Net/Time/Geo/Web (según presets).
4. `src/screens/` — IScreen + BaseSprite (16->8->4 bpp graceful degradation).

## Reglas duras
- Sin globales (singletons o miembros de clase).
- `new` en `onEnter()`, `delete` en `onExit()` para sprites pesados.
- Loop nunca bloquea > 50 ms — operaciones blocking en `xTaskCreatePinnedToCore` Core 0.
- Cero warnings.
- Logger format: `[Timestamp] [TAG] Mensaje`.

## Build / Deploy
- `./build.ps1` (build incremental fuera de OneDrive: `C:\Users\egavi\pio_temp_build\{{BUILD_DIR}}`).
- `./upload.ps1` (no recompila).

## Estética UI: Retro Terminal
- Fondo TFT_BLACK, texto TFT_GREEN, headers inversos, separadores TFT_DARKGREEN.
- Font 2 (~16 px), leading 18 px, margen X 4 px.

## Setup inicial
1. `cp src/core/Config_Template.h src/core/Config.h` y rellenar SSID/PSK.
2. `./build.ps1`.
3. `./upload.ps1`.
'@

$ciContent = $ciTemplate.Replace('{{PROJECT_NAME}}', $ProjectName).Replace('{{PRESETS}}', $presetList).Replace('{{BUILD_DIR}}', $BuildDirName)
$ciContent | Set-Content -LiteralPath (Join-Path $ghDir 'copilot-instructions.md') -Encoding UTF8

# ---------------------------------------------------------------------------
# Copy reusable skills into .github/skills/ (project-agnostic + preset-specific)
# ---------------------------------------------------------------------------
$skillsSrcRoot = Join-Path $SourceRoot '.github\skills'
$skillsDstRoot = Join-Path $ghDir 'skills'

# Always-applicable skills (any ESP32-S3 framework derivative benefits from them)
$AlwaysSkills = @('git-workflow', 'build-pipeline', 'runtime-debug', 'iteration-close', 'tft-screen')

# Preset-conditional skills
$PresetSkills = @{
    'web'       = @('webservice-http')
    'ble-input' = @('ble-input')
    'ble-scan'  = @('ble-input')
}

$skillsToCopy = New-Object System.Collections.Generic.List[string]
$AlwaysSkills | ForEach-Object { $skillsToCopy.Add($_) }
foreach ($p in $Presets) {
    if ($PresetSkills.ContainsKey($p)) {
        foreach ($s in $PresetSkills[$p]) {
            if (-not $skillsToCopy.Contains($s)) { $skillsToCopy.Add($s) }
        }
    }
}

foreach ($skill in $skillsToCopy) {
    $sSrc = Join-Path $skillsSrcRoot $skill
    if (-not (Test-Path $sSrc)) {
        Write-Host "[scaffold] WARN: skill '$skill' not found at source — skipped." -ForegroundColor Yellow
        continue
    }
    $sDst = Join-Path $skillsDstRoot $skill
    Copy-Item -Path $sSrc -Destination $sDst -Recurse -Force
}

# Append skills index to generated copilot-instructions.md
$skillsIndex = "`n## Skills disponibles para invocar`n"
$skillsDescriptions = @{
    'git-workflow'    = 'commits, tags, release, recovery'
    'build-pipeline'  = 'build/upload scripts, logs, IntelliSense'
    'runtime-debug'   = 'panics, WDT resets, heap'
    'iteration-close' = 'cerrar iteración, changelog, sync README'
    'tft-screen'      = 'crear/editar pantallas (IScreen + BaseSprite)'
    'webservice-http' = 'endpoints HTTP, restricciones TCP, GOLConfig'
    'ble-input'       = 'Bluepad32 / BLE scan / dual stack'
}
foreach ($s in $skillsToCopy) {
    $desc = if ($skillsDescriptions.ContainsKey($s)) { $skillsDescriptions[$s] } else { '' }
    $skillsIndex += "- ``$s`` — $desc`n"
}
Add-Content -LiteralPath (Join-Path $ghDir 'copilot-instructions.md') -Value $skillsIndex -Encoding UTF8

# ---------------------------------------------------------------------------
# Generate skeleton README.md (NOT the v2 one)
# ---------------------------------------------------------------------------
$readmeTemplate = @'
<!-- scaffolded from ESP32S3-TFT Framework -->
# {{PROJECT_NAME}}

Scaffolded from **ESP32S3-TFT Framework** — *Engineering sandbox for ESP32-S3 Lolin S3 Pro + ILI9341*.

**Firmware version:** {{FW_VERSION}}
**Presets enabled:** {{PRESETS}}

## Hardware target
- **Board (physical):** ESP32-S3 Lolin S3 Pro — pinout reference for the TFT, LED and BOOT button.
- **Board (PlatformIO ID):** `esp32-s3-devkitc-1` — mantenido a propósito por incompatibilidad entre el variant `lolin_s3` y los macros de pines de TFT_eSPI.
- **Display:** ILI9341 240x320 sobre HSPI a 27 MHz.

## Setup
1. Duplicate `src/core/Config_Template.h` → `src/core/Config.h` and fill in WiFi credentials.
2. Connect the ESP32-S3 over USB.
3. Build: `./build.ps1`
4. Flash + monitor: `./upload.ps1`

## Architecture
Layered: HAL → Core → Services → Screens. See `DevGuidelines.md` for hard rules and `.github/copilot-instructions.md` for the agent context.

## Changelog

### v{{FW_VERSION}} — {{DATE}}
*   Initial scaffold from ESP32S3-TFT Framework.
'@

$readmeContent = $readmeTemplate.
Replace('{{PROJECT_NAME}}', $ProjectName).
Replace('{{FW_VERSION}}', $FwVersion).
Replace('{{PRESETS}}', $presetList).
Replace('{{DATE}}', (Get-Date -Format 'yyyy-MM-dd'))
$readmeContent | Set-Content -LiteralPath (Join-Path $ProjectPath 'README.md') -Encoding UTF8

# ---------------------------------------------------------------------------
# Optional: git init
# ---------------------------------------------------------------------------
if ($InitGit) {
    Push-Location $ProjectPath
    try {
        git init -q
        git add -A
        git -c user.email='scaffold@local' -c user.name='scaffold' commit -q -m "chore: initial scaffold from ESP32S3-TFT Framework"
        Write-Host "[scaffold] git initialized + first commit." -ForegroundColor Green
    }
    catch {
        Write-Host "[scaffold] git init skipped: $_" -ForegroundColor Yellow
    }
    finally {
        Pop-Location
    }
}

Write-Host ""
Write-Host "[scaffold] DONE." -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. cd `"$ProjectPath`""
Write-Host "  2. cp src/core/Config_Template.h src/core/Config.h  (and fill credentials)"
Write-Host "  3. ./build.ps1"
Write-Host "  4. ./upload.ps1"
