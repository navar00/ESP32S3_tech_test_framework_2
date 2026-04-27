---
name: runtime-debug
description: "Use when diagnosing runtime issues in ESP32S3-TFT Framework: panics, watchdog resets, heap fragmentation, sprite allocation failures, brownouts, BLE re-init crashes, NTP sync failures, or any abnormal boot reason. Triggers: 'panic ESP32', 'watchdog reset', 'WDT', 'esp_reset_reason', 'Sprite Alloc Failed', 'heap fragmentation', 'black screen bug', 'boot counter NVS', 'brownout', 'StorageHAL'."
---

# Skill: Runtime Debug (ESP32S3-TFT Framework)

## Cuándo aplica
Diagnosticar fallos en runtime: panics, resets inesperados, pantalla negra, falta de memoria, problemas de boot.

## Estado real del Watchdog (ojo: README desactualizado)
- README §3.6 dice 8 s. **El código real usa `Config::WDT_TIMEOUT_LOOP`** y se aplica desde `WatchdogHAL::begin()` tras boot. Verificar valor en `src/core/Config.h`.
- `loop()` llama `WatchdogHAL::feed()` en cada iteración (línea 1).
- Si causa de reset es WDT/Panic, `BootOrchestrator` parpadea LED rojo 2 s antes de arrancar (lectura vía `esp_reset_reason()`).
- Boot counter persistente en NVS vía `StorageHAL::incrementBootCounter()` — útil para detectar reset-loops.

## Estado real de PSRAM (ojo: README desactualizado)
- README §6.3 propone activar PSRAM. **`platformio.ini` la tiene COMENTADA**:
  ```ini
  ; -D BOARD_HAS_PSRAM
  ; -mfix-esp32-psram-cache-issue
  ```
- La mitigación operativa es **Graceful Degradation** en `BaseSprite` (16→8→4 bpp) + `pBLEScan->clearResults()` agresivo.
- Antes de re-activar PSRAM: confirmar variante hardware real (debe ser N8R2 / N8R8). En N8 sin R no hay PSRAM y `psramFound()` devuelve `false`.

## Checklist de diagnóstico

### A) Pantalla negra al cambiar de screen ("Black Screen Bug", README §6.3)
1. Buscar log `GFX: CRITICAL: Sprite Alloc Failed!`.
2. Causa típica: heap fragmentado tras BLE / WebService / JSON grande.
3. Comprobar: ¿`onExit()` de la pantalla previa libera sprite y resultados BLE?
4. ¿Sprite cabe en 8bpp? (≈75 KB contiguos). En 4bpp ≈38 KB.
5. Si ocurre tras servir `/api/palettes`: confirmar que el cliente cerró conexión (`Connection: close` en cabeceras).

### B) WDT reset / reset loop
1. `esp_reset_reason()` reporta `ESP_RST_TASK_WDT` o `ESP_RST_PANIC`.
2. Buscar bloqueos > 50 ms en `loop()`:
   - `BLEClient::connect()` síncrono → mover a task Core 0 (skill `ble-input`).
   - HTTP backpressure sin `yield()` dentro del loop write.
   - `delay()` largos.
3. Verificar `WatchdogHAL::feed()` se llame en TODAS las ramas del `loop()`.
4. Boot counter creciente sin actividad humana → reset-loop confirmado.

### C) `Sprite Alloc Failed` específicamente en 8bpp
- BUG histórico: `createPalette(nullptr, 16)` con sprite 8bpp → 16 entradas para 256 índices → colores aleatoriamente invisibles. **Eliminado del código**; si reaparece en una nueva pantalla, **eliminar la llamada** y dejar mapeo nativo 332 (README §6.8).

### D) Brownout / reset al iniciar WiFi
- Cable USB/fuente insuficiente. ESP32-S3 + TFT + NeoPixel pueden picos > 500 mA.
- Reducir brillo NeoPixel (ya limitado a 5%, valor 12/255).
- Comentar momentáneamente `NetService::connectBestRSSI()` para confirmar.

### E) BLE panic en re-init
- Síntoma: `esp_bt_controller_mem_release` panic.
- Causa: alguien llamó `BLEDevice::deinit(true)`. Buscar en repo y eliminar. **Mantener el stack siempre vivo.**

### F) NTP no sincroniza (hora `N/A` intermitente)
- Verificar `TimeService::getSecsSinceSync()` > 0.
- Confirmar `NTP_SYNC_INTERVAL = 3600s` y `pool.ntp.org` accesible desde la red local.
- Offset zona horaria viene de `GeoService::fetchAndApply()` (IP geo). Sin red → 0.

## Lectura de logs
- **Local**: `<BuildDir>\.logs\build_latest.txt` (compilación), `<BuildDir>\.logs\upload_latest.txt` (flash + verify), `<BuildDir>\.logs\monitor_latest.log` (monitor serie capturado).
- **Live**: `./upload.ps1` abre monitor serie tras flash (sin captura).
- **Captura solo de monitor**: `./monitor.ps1` — ejecuta `pio device monitor` con `Tee-Object` a `<BuildDir>\.logs\monitor_<timestamp>.log`. Útil para reproducir un panic sin reflashear. Rotación automática (últimos 10).
  ```powershell
  ./monitor.ps1                # baud 115200, último puerto
  ./monitor.ps1 -Port COM7     # forzar puerto
  ./monitor.ps1 -NoTee         # deshabilitar captura (solo live)
  ```
  Para que el agente analice un panic reciente:
  ```powershell
  Get-Content 'C:\Users\egavi\pio_temp_build\ESP32S3-TFT_Framework\.logs\monitor_latest.log' -Tail 80
  ```
- Filtrar:
  ```powershell
  Select-String -Path .\upload_log.txt -Pattern '\[(GFX|WDT|BOOT)\]'
  ```

## Logging de errores — formato obligatorio
- ❌ `Logger::log("WDT", "Error")`
- ✅ `Logger::log("WDT", "Feed missed: blocked %lu ms in onLoop()", elapsed)`
- Adjuntar siempre código/contexto numérico para `grep` posterior.

## Inconsistencias README ↔ código (corregir cuando se cierre iteración)
- §3.6 WDT 8 s → ajustar a `Config::WDT_TIMEOUT_LOOP`.
- §6.3 PSRAM activa → marcar como "no aplicable, deshabilitada".
- TAGs documentados con padding 3-5 chars vs reales (`WebSrv`, `TimeService`, `BOOT_UI` no cumplen) → unificar o aceptar.

---

## Runbook ESP-IDF (APIs invocables desde Arduino)

Ver `DevGuidelines.md §11` para el contrato general. Aquí sólo las recetas operativas para diagnóstico.

### Heap snapshot (antes de cualquier `new` grande)
```cpp
#include "esp_heap_caps.h"
size_t f = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
size_t b = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
Logger::log("HEAP", "free=%u largest=%u (need=%u)", f, b, requested);
```
Si `largest < requested` → no intentar 16 bpp; saltar directamente a 8 bpp en `BaseSprite`. Es la mejora natural para evitar el ciclo "fail → log → retry" actual.

Para inventariar todos los pools (interno, DMA, PSRAM si la hubiera):
```cpp
heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
```

### Stack watermark de tasks
Tras un ciclo representativo de la task BLE/HTTP, loggear:
```cpp
UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
Logger::log("BLE", "stack remaining=%u bytes", hwm * sizeof(StackType_t));
```
Si < 512 bytes → ampliar el `usStackDepth` al alocar la task. Si > 4 KB sobrantes → reducir para liberar heap.

### Registrar una task pinned en el TWDT
Cuando una task Core 0 puede tardar > 3 s (BLE connect, HTTPS GET futuro):
```cpp
esp_task_wdt_add(NULL);
while (working) {
    doStep();
    esp_task_wdt_reset();
}
esp_task_wdt_delete(NULL);
vTaskDelete(NULL);
```
Sin esto la task puede colgarse y el TWDT global no la cazará.

### Decodificar un panic capturado (`monitor_latest.log`)
Localizar líneas `Backtrace: 0x40080d3a:0x3ffb1f80 0x40080d70:...` y resolver con `addr2line`:
```powershell
$elf = "C:\Users\egavi\pio_temp_build\ESP32S3-TFT_Framework\.pio\build\esp32-s3-devkitc-1\firmware.elf"
$addr2line = "$env:USERPROFILE\.platformio\packages\toolchain-xtensa-esp-elf\bin\xtensa-esp32s3-elf-addr2line.exe"
& $addr2line -pfiaC -e $elf 0x40080d3a 0x40080d70 0x40080dac
```
Salida: fichero + línea + nombre de función. Imprescindible para diagnosticar `LoadProhibited`, `StoreProhibited`, `IllegalInstruction`.

### Core dump opt-in (sólo sesiones de debug profundo)
Añadir temporalmente a `platformio.ini`:
```ini
build_flags = -DCONFIG_ESP_COREDUMP_ENABLE=1 -DCONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=1 -DCONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1
```
Requiere partición `coredump,data,coredump,,64K` en `partitions.csv`. Tras un panic:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --filter esp32_exception_decoder
```
o extraer el dump con `espcoredump.py info_corefile`. **Recordar quitar los flags al cerrar la iteración** (ocupan flash y ralentizan el panic handler).

### JTAG built-in (USB-Serial/JTAG del propio S3, sin hardware externo)
Para cuelgues que no llegan al monitor serie (early boot, IWDT, deadlock silencioso):
```ini
; en platformio.ini, sólo durante debug
debug_tool = esp-builtin
debug_init_break = tbreak setup
```
Lanzar `Run → Start Debugging` desde VS Code (PlatformIO Debug). Requiere driver `libusb-win32` instalado vía Zadig la primera vez. Permite breakpoints reales y `info threads` para ver tasks FreeRTOS.

Referencia: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/index.html>.
