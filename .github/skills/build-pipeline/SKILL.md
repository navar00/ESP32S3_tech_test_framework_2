---
name: build-pipeline
description: "Use when working with the build/upload pipeline of TechTest v2 on Windows + OneDrive: incremental builds, regenerating PalettesData.h, debugging build/upload scripts, or understanding why a clean build is slow. Triggers: 'build.ps1', 'upload.ps1', 'compilación incremental', 'pio_temp_build', 'PalettesData.h', 'OneDrive lentitud PlatformIO', 'compile_commands.json'."
---

# Skill: Build Pipeline (TechTest v2)

## Cuándo aplica
- Modificar/depurar `build.ps1`, `upload.ps1`.
- Regenerar `src/core/PalettesData.h` desde paletas fuente.
- Diagnosticar builds lentos / IntelliSense roto.

## Por qué existen los scripts
PlatformIO sobre OneDrive es lento e inestable: OneDrive sincroniza `.pio/` (cientos de miles de archivos), corrompe locks y revierte timestamps. Solución: **build fuera de OneDrive con caché preservada**.

## Directorios canónicos
| Variable | Ruta |
|---|---|
| Source (origen, OneDrive) | `c:\Users\egavi\OneDrive\Documents\PlatformIO\Projects\ESP32S3_tech_test_framework_2\` |
| Build dir (fuera OneDrive) | `C:\Users\egavi\pio_temp_build\TechTest_v2\` |
| PlatformIO env | `esp32-s3-devkitc-1` |

## `./build.ps1` — flujo
1. Copia source → build dir excluyendo `.pio .git .vscode *.ps1`.
2. **Preserva** `.pio/` existente en build dir (caché incremental).
3. Lanza `platformio run --environment esp32-s3-devkitc-1` desde build dir.
4. Salida resumida en pantalla; detalle completo en `<BuildDir>\.logs\build_<timestamp>.txt`.
5. Si hay errores: extrae líneas con `error:` y las muestra.

Tiempos esperados:
- Incremental tras edit pequeño: ~18-30 s.
- Limpio (`.pio/` vacío): ~100 s.

## `./upload.ps1` — flujo
1. **No recompila** — verifica que existe `firmware.bin` en build dir.
2. `platformio run --target upload --project-dir <build_dir>` (reutiliza binario).
3. Tras upload: abre monitor serie automáticamente.
4. Log en `<BuildDir>\.logs\upload_<timestamp>.txt`.
5. Tiempo total: ~33 s sin recompilación (vs ~150 s si recompila).

## Logs (rotación + alias)
Ubicación: `C:\Users\egavi\pio_temp_build\TechTest_v2\.logs\` (fuera de OneDrive).

| Archivo | Contenido |
|---|---|
| `build_latest.txt` / `upload_latest.txt` | Copia del último log (estable para parsers/skills). |
| `build_<yyyy-mm-dd_HH-mm-ss>.txt` | Histórico timestamped. |
| `upload_<yyyy-mm-dd_HH-mm-ss>.txt` | Histórico timestamped. |

Rotación automática: se conservan los **10 más recientes** de cada tipo; el resto se elimina al final del script.

Para consultar el último log desde una skill:
```powershell
Get-Content 'C:\Users\egavi\pio_temp_build\TechTest_v2\.logs\build_latest.txt' -Tail 40
```

## Si necesitas build totalmente limpio
```powershell
Remove-Item -Recurse -Force "C:\Users\egavi\pio_temp_build\TechTest_v2\.pio"
./build.ps1
```

## IntelliSense (`compile_commands.json`)
- Generado vía `pio run -t compiledb` desde el build dir.
- Configurado en `.vscode/c_cpp_properties.json`.
- Si IntelliSense marca includes como rojos:
  ```powershell
  Push-Location "C:\Users\egavi\pio_temp_build\TechTest_v2"
  platformio run -t compiledb
  Pop-Location
  Copy-Item "C:\Users\egavi\pio_temp_build\TechTest_v2\compile_commands.json" .
  ```

## Regenerar `src/core/PalettesData.h`
Generado desde imágenes/paletas en `include/colores/` por scripts de `aux_/`:
- `aux_/refactor_dashboard.py` o `aux_/fix_dashboard.py` (verificar el activo en cada momento).
- Output: array PROGMEM `PALETTES_JSON[]` ~27 KB con estructura `{folder: {palette: [{hex, freq}, ...]}}`.

Pasos:
1. Modificar/añadir paletas en `include/colores/`.
2. Ejecutar el script Python desde la raíz del proyecto.
3. Verificar que el array generado **no excede el plan de transferencia** (servido vía WiFiClient backpressure, ver skill `webservice-http`).
4. `./build.ps1` para validar.

## `platformio.ini` clave (no romper)
```ini
platform = espressif32
board = esp32-s3-devkitc-1
platform_packages =
    framework-arduinoespressif32 @ https://github.com/ricardoquesada/esp32-arduino-lib-builder/releases/download/4.1.0/esp32-bluepad32-4.1.0.zip
    toolchain-xtensa-esp32s3 @ 12.2.0+20230208
lib_deps =
    bodmer/TFT_eSPI
    adafruit/Adafruit NeoPixel
    bblanchon/ArduinoJson
lib_ignore = ESPAsyncTCP
```
- El fork de Bluepad32 es necesario para que coexistan `Bluepad32.h` y `BLEDevice.h` (skill `ble-input`).
- `lib_ignore = ESPAsyncTCP` evita conflictos con el `WebServer` síncrono usado.
- PSRAM flags **comentadas** intencionalmente (ver skill `runtime-debug`).

## Pinout y SPI fijados (no cambiar sin actualizar README §2.1)
```
TFT MOSI 11 · SCLK 12 · MISO 13 · CS 48 · DC 47 · RST 21
USE_HSPI_PORT=1 · SPI_FREQUENCY 27 MHz
```

## Antipatrones
- Compilar directamente desde OneDrive (`platformio run` en el directorio source). OneDrive corrompe `.pio/`.
- Borrar `.pio/` por costumbre: pierde la caché incremental.
- Subir `pio_temp_build/` a git (ya está en otra ruta, pero verificar `.gitignore`).
- Modificar `compile_commands.json` a mano: se regenera.
