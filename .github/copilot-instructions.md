# TechTest v2 — Instrucciones base de Copilot

Proyecto: **ESP32-S3 Engineering Sandbox "Virtuoso Edition"** (PlatformIO + Arduino).
**No cargues `README.md` por defecto**: es la especificación viva (~38 KB). Para detalle, invoca la skill aplicable o lee la sección concreta (`README.md §X`).

## Stack y arquitectura
- **MCU**: ESP32-S3 DevKitC-1, 16 MB flash, 240 MHz. **PSRAM deshabilitada** (`platformio.ini` la tiene comentada).
- **Capas** (estricto, sin saltos):
  1. `src/hal/` — HAL singleton (DisplayHAL, LedHAL, WatchdogHAL, StorageHAL, InputHAL).
  2. `src/core/` — Lógica pura: `Logger`, `ScreenManager`, `BootOrchestrator`, `Config`, `GOLConfig` (estado compartido), `WatchdogManager`.
  3. `src/services/` — `NetService`, `TimeService`, `GeoService`, `WebService`.
  4. `src/screens/` — Implementaciones de `IScreen` (Strategy). Helper `BaseSprite` con degradación 16→8→4 bpp.
- **Boot**: `BootOrchestrator::run()` → WiFi → GeoIP → NTP → WebService → Ready.
- **Loop**: WDT feed → timer flag (1 Hz ISR) → botón GPIO0 → `ScreenManager::loop()` → `WebService::handleClient()` → `delay(5)`.

## Reglas duras (MANDATORY)
- Prohibido estado global. Todo en singletons o miembros de clase.
- `new` en `onEnter()`, `delete` en `onExit()` para sprites pesados.
- **Loop nunca bloquea > 50 ms**. Operaciones bloqueantes → `xTaskCreatePinnedToCore` Core 0.
- Cero warnings de compilación.
- Estado compartido TFT↔Web vía `GOLConfig` (portMUX) o patrón equivalente — nunca variables globales sin protección.

## Hardware fijado
- TFT ILI9341: MOSI 11, SCLK 12, MISO 13, CS 48, DC 47, RST 21 (HSPI, 27 MHz).
- LED RGB (NeoPixel): GPIO 38 (brillo 5%).
- Botón BOOT: GPIO 0 (active LOW, debounce 250 ms).

## Logging
- Formato: `[Timestamp] [TAG] Mensaje` mediante `Logger::log(TAG, fmt, ...)`.
- TAGs canónicos en uso real: `BOOT`, `BOOT_UI`, `MAIN`, `MGR`, `GFX`, `WDT`, `BP32`, `BLE`, `WebSrv`, `STATUS`, `TimeService`, `NetService`, `GeoService`, `DISPLAY`.
- Errores: incluir código/contexto, nunca solo "Error".

## Watchdog (estado real, no el del README)
- `WatchdogHAL` configurado en `BootOrchestrator`, alimentado en `loop()` cada iteración.
- Si causa de reset fue WDT/Panic, parpadeo rojo 2 s antes de arrancar.

## Build y deploy (Windows / OneDrive)
- **Compilar**: `./build.ps1` → copia a `C:\Users\egavi\pio_temp_build\TechTest_v2` (preserva `.pio/`), logs en `<BuildDir>\.logs\build_latest.txt` (+ timestamped, rotación 10).
- **Subir**: `./upload.ps1` (no recompila, reutiliza `firmware.bin`), logs en `<BuildDir>\.logs\upload_latest.txt`.
- **Monitor serie**: `./monitor.ps1 [-Port COMx] [-Baud 115200]` → logs en `<BuildDir>\.logs\monitor_latest.log` (+ timestamped, rotación 10). Útil para `runtime-debug`.
- Ambos scripts excluyen `.pio .git .vscode *.ps1` al copiar.

## Calidad y CI
- **Tests host**: `pio test -e native` (Unity, sólo lógica pura — sin Arduino). Tests bajo `test/test_native_*`. Requiere `gcc/g++` en PATH (CI Ubuntu lo trae; Windows requiere MinGW).
- **Análisis estático**: `pio check -e check` (cppcheck — warning/style/performance/portability). No bloqueante en CI pero limpiar antes de release.
- **CI**: `.github/workflows/ci.yml` ejecuta `build` (esp32-s3-devkitc-1) + `test` (native) + `check` (cppcheck) en cada push/PR a `main`. Stubea `src/core/Config.h` desde `Config_Template.h`.

## WebService — restricciones críticas
- Ventana TCP del ESP32-S3: **~5744 bytes**. Toda respuesta `send()`/`send_P()` debe ser < 5 KB.
- Para payloads > 5 KB → patrón **WiFiClient backpressure** (chunks 256 B + retry + `delay(5)` + `yield()`).
- Prohibido `sendContent()` / chunked encoding.
- `WebServer` es **single-threaded**: el cliente debe encadenar `fetch().then().then()`, nunca paralelo.
- Patrón **Split-Payload**: HTML y JS en blobs PROGMEM independientes < 5 KB cada uno.

## BLE
- Coexisten **Bluepad32** (`InputHAL`, gamepads) y **BLEDevice** Arduino (`ScreenBLEScan`).
- **No** llamar `BLEDevice::deinit(true)` en runtime → panic en re-init.
- `BLEClient::connect()` es bloqueante > 8 s → siempre en task pinned a Core 0.

## Skills disponibles para invocar
- `tft-screen` — crear/editar pantallas (IScreen + BaseSprite).
- `webservice-http` — endpoints, GOLConfig, restricciones TCP.
- `ble-input` — Bluepad32 / BLE scan / dual stack.
- `runtime-debug` — panics, WDT resets, heap, parsear `monitor_latest.log`.
- `iteration-close` — cerrar iteración, changelog, sync README.
- `git-workflow` — commits, tags, release, recovery.
- `build-pipeline` — build/upload, regenerar `PalettesData.h`.
- `scaffold-new-project` — clonar este framework como nuevo proyecto.

## Prompts invocables (`.github/prompts/`)
- `release.prompt.md` — orquesta cierre de iteración + commit/tag/push (bajo confirmación).
- `audit-screen.prompt.md` — audita una pantalla `IScreen` contra reglas duras (read-only).
- `dump-status.prompt.md` — snapshot estructurado del proyecto (versión, pantallas, sizes, endpoints).

## Documentación local
- `README.md` — especificación viva (no cargar entera; consultar sección concreta).
- `DevGuidelines.md` — reglas duras y heurísticas extendidas.
- `aux_/legacy_prompts/` — prompts previos archivados (cubiertos ahora por skills).
