---
name: runtime-debug
description: "Use when diagnosing runtime issues in TechTest v2: panics, watchdog resets, heap fragmentation, sprite allocation failures, brownouts, BLE re-init crashes, NTP sync failures, or any abnormal boot reason. Triggers: 'panic ESP32', 'watchdog reset', 'WDT', 'esp_reset_reason', 'Sprite Alloc Failed', 'heap fragmentation', 'black screen bug', 'boot counter NVS', 'brownout', 'StorageHAL'."
---

# Skill: Runtime Debug (TechTest v2)

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
  Get-Content 'C:\Users\egavi\pio_temp_build\TechTest_v2\.logs\monitor_latest.log' -Tail 80
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
