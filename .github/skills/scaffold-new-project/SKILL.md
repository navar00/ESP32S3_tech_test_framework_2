---
name: scaffold-new-project
description: "Use when the user wants to create a NEW PlatformIO project cloned from the ESP32S3-TFT Framework (engineering sandbox for ESP32-S3 Lolin S3 Pro + ILI9341) with the proven configuration (HAL singletons, IScreen Strategy, BootOrchestrator, build/upload scripts, retro-terminal UI, optional WebService/BLE/Game-of-Life/Clocks/Palettes). Triggers: 'nuevo proyecto framework', 'scaffold ESP32-S3', 'plantilla ESP32S3-TFT', 'iniciar proyecto desde framework', 'sandbox Lolin S3'. Runs an interactive PowerShell script that copies a curated subset and rewrites placeholders."
---

# Skill: Scaffold New Project (ESP32S3-TFT Framework)

## Cuándo aplica
El usuario quiere arrancar un proyecto nuevo basado en el **ESP32S3-TFT Framework** — *Engineering sandbox for ESP32-S3 Lolin S3 Pro + ILI9341* (evolución de un sandbox previo) — reutilizando la arquitectura y configuración depurada (HAL/Core/Services/Screens, build pipeline OneDrive-safe, retro-terminal UI, opcionalmente WebService/BLE/GOL/Clocks/Palettes).

## Hardware target del scaffold
- **Físico:** ESP32-S3 **Lolin S3 Pro** + display ILI9341 240x320.
- **PlatformIO board ID:** `esp32-s3-devkitc-1` (mantenido a propósito: el variant `lolin_s3` colisiona con los macros de pines de TFT_eSPI).
- Pinout TFT (referencia Lolin S3 Pro): MOSI 11, SCLK 12, MISO 13, CS 48, DC 47, RST 21.

## Workflow

### Paso 1 — Recolectar parámetros
Pregunta al usuario (usar `vscode_askQuestions` si está disponible):
1. **`PROJECT_NAME`** — nombre lógico (ej. `ESP32S3_robot_v1`).
2. **`PROJECT_PATH`** — ruta destino. Sugerencia: `C:\Users\egavi\OneDrive\Documents\PlatformIO\Projects\<PROJECT_NAME>`.
3. **`BUILD_DIR_NAME`** — nombre de la carpeta fuera de OneDrive. Default: derivar del nombre (ej. `Robot_v1`).
4. **`FW_VERSION`** — default `0.1.0`.
5. **Presets** (multi-select):
   - [x] Core base (siempre incluido): `Logger`, `ScreenManager`, `BootOrchestrator`, `Config`, `WatchdogManager`, `WatchdogHAL`, `DisplayHAL`, `LedHAL`, `StorageHAL`, `IScreen`, `BaseSprite`, `BootConsoleScreen`, `ScreenStatus`.
   - [ ] WiFi + NTP (`NetService`, `TimeService`, `GeoService`).
   - [ ] WebService dashboard (`WebService` + `GOLConfig` + plantilla DASH_HTML/JS reducida).
   - [ ] BLE Input — Bluepad32 (`InputHAL` + fork en `platform_packages`).
   - [ ] BLE Scan (`ScreenBLEScan` con `BLEDevice`).
   - [ ] Game of Life screen (`ScreenGameOfLife`).
   - [ ] Clocks (`ScreenAnalogClock` + `ScreenFlipClock`).
   - [ ] Palettes (`ScreenPalette` + `PalettesData.h` + `include/colores/`).
6. **Inicializar git** (sí/no).

### Paso 2 — Ejecutar el script
```powershell
& "<RUTA_SKILL>/scaffold.ps1" `
    -ProjectName "<PROJECT_NAME>" `
    -ProjectPath "<PROJECT_PATH>" `
    -BuildDirName "<BUILD_DIR_NAME>" `
    -FwVersion "<FW_VERSION>" `
    -Presets @("wifi","web","gol","clocks","palettes") `
    -InitGit
```

El script:
1. Crea estructura de carpetas (`src/{core,hal,services,screens}`, `include`, `lib`, `test`, `aux_`).
2. Copia desde el framework los archivos correspondientes a los presets seleccionados.
3. Reescribe placeholders:
   - `{{PROJECT_NAME}}` en `platformio.ini` (usado en `name`/comments si aplica).
   - `{{BUILD_DIR_NAME}}` en `build.ps1` y `upload.ps1` (variable `$BuildDir`).
   - `{{FW_VERSION}}` en `Config.h` (`constexpr const char* FW_VERSION`).
4. Renombra `Config.h` → `Config_Template.h` para evitar commitear credenciales.
5. Genera `.gitignore` adaptado (excluye `Config.h`, `.pio/`, `*.log`, `_port_backup_*/`, `pio_monitor.log`).
6. Genera `.vscode/{tasks.json,c_cpp_properties.json,extensions.json}` mínimos y limpios.
7. Genera `README.md` esqueleto con secciones (no copia el README v2 completo).
8. Genera `DevGuidelines.md` reducido (heredado).
9. Genera `.github/copilot-instructions.md` adaptado al subset elegido.
10. Copia skills reutilizables a `.github/skills/`:
    - **Siempre**: `git-workflow`, `build-pipeline`, `runtime-debug`, `iteration-close`, `tft-screen`.
    - Preset `web` → añade `webservice-http`.
    - Presets `ble-input` o `ble-scan` → añade `ble-input`.
11. Si `-InitGit`: `git init`, `git add -A`, primer commit.

### Paso 3 — Checklist post-scaffold (mostrar al usuario)
1. `cd <PROJECT_PATH>`
2. Copiar `src/core/Config_Template.h` → `src/core/Config.h` y rellenar SSID/PSK.
3. (Opcional) Abrir `platformio.ini` y ajustar `monitor_speed`, `upload_port` si es necesario.
4. Conectar el ESP32-S3.
5. `./build.ps1` (la primera build será limpia ~100 s).
6. `./upload.ps1`.
7. Iterar añadiendo pantallas con la skill `tft-screen`.

## Estructura de salida (sin presets opcionales)
```
<PROJECT_PATH>/
├── platformio.ini
├── README.md                    # Esqueleto, no el README v2 completo
├── DevGuidelines.md             # Reglas duras heredadas
├── build.ps1
├── upload.ps1
├── .gitignore
├── .github/
│   └── copilot-instructions.md
├── .vscode/
│   ├── tasks.json
│   ├── c_cpp_properties.json
│   └── extensions.json
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Config_Template.h   # NO Config.h (gitignored)
│   │   ├── Logger.h
│   │   ├── ScreenManager.h
│   │   ├── BootOrchestrator.{h,cpp}
│   │   └── WatchdogManager.h
│   ├── hal/
│   │   ├── Hal.h
│   │   ├── DisplayHAL.{h,cpp}
│   │   ├── LedHAL.{h,cpp}
│   │   ├── WatchdogHAL.{h,cpp}
│   │   └── StorageHAL.{h,cpp}
│   └── screens/
│       ├── IScreen.h
│       ├── BaseSprite.h
│       ├── BootConsoleScreen.{h,cpp}
│       └── ScreenStatus.{h,cpp}
├── include/
├── lib/
├── test/
└── aux_/
```

## Ajustes de `platformio.ini` por preset
| Preset | Acción |
|---|---|
| BLE Input (Bluepad32) | Añadir bloque `platform_packages` con fork Quesada |
| BLE Scan | Añadir `lib_deps` `ESP32 BLE Arduino` (si no ya implícito) |
| WebService | Mantener `lib_ignore = ESPAsyncTCP` |
| Palettes | Crear carpeta `include/colores/` vacía |

## Versionado del scaffold
- El script versiona qué versión de TechTest v2 se usó como source en un comentario inicial del `README.md` generado: `<!-- scaffolded from TechTest v2 vX.Y.Z -->`. Leer la versión actual desde el changelog de TechTest v2 (sección §7).

## Antipatrones
- Copiar el `README.md` v2 entero al nuevo proyecto: lo deja desfasado desde día 1.
- Copiar `Config.h` con credenciales reales.
- Heredar `pio_temp_build/` o `.pio/`.
- Heredar `.vscode/tasks.json` con rutas absolutas del proyecto fuente.
- Olvidar adaptar `BUILD_DIR_NAME` (causa colisión con build de TechTest v2).

## Script
Ver `scaffold.ps1` en este mismo directorio para la implementación. La sección "MANIFEST" al inicio del script lista qué archivos copiar por cada preset; mantener ese manifiesto sincronizado con cambios estructurales en TechTest v2.
