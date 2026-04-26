---
name: ble-input
description: "Use when working with Bluetooth in TechTest v2: Bluepad32 gamepads (InputHAL), BLE Arduino scanning (ScreenBLEScan/ScreenGamepad), HID classification, async BLE connections, or dealing with the dual BLE stack coexistence. Triggers: 'Bluepad32', 'gamepad ESP32', 'BLE scan', 'HID 0x1812', 'BLEClient connect bloqueante', 'BLEDevice deinit panic', 'pinned core 0 BLE', 'dual stack BLE'."
---

# Skill: BLE & Input (TechTest v2)

## Cuándo aplica
- Trabajar con `src/hal/InputHAL.{h,cpp}` (Bluepad32) o pantallas BLE: `ScreenBLEScan`, `ScreenGamepad`.
- Conectar a dispositivos HID, escanear, clasificar.

## Coexistencia obligatoria de dos stacks
TechTest v2 tiene **dos stacks BLE en el mismo binario**:
1. **Bluepad32** (fork `ricardoquesada/esp32-arduino-lib-builder`) — usado por `InputHAL` para gamepads HID. Hace `#include <Bluepad32.h>`.
2. **BLE Arduino estándar** (`BLEDevice`, `BLEClient`, `BLEScan`) — usado por `ScreenBLEScan` y `ScreenGamepad` (modo explorador).

`platform_packages` en `platformio.ini` apunta al fork de Quesada para que ambos compilen.

## Reglas duras
- **Nunca** llamar `BLEDevice::deinit(true)` en runtime. La re-inicialización es inestable → panic en `esp_bt_controller_mem_release`. Mantener stack vivo y confiar en degradación de RAM (`BaseSprite` 16→8→4 bpp).
- **`BLEClient::connect()` es bloqueante** y puede tardar > 8 s. Si se ejecuta en el loop principal → WDT reset. Ejecutar SIEMPRE en task FreeRTOS pinned a Core 0:

```cpp
xTaskCreatePinnedToCore(
    [](void* arg) {
        auto* self = static_cast<ScreenGamepad*>(arg);
        self->connectAsync();   // operación bloqueante aislada
        vTaskDelete(NULL);
    },
    "BLEConnect", 8192, this, 1, &_taskHandle, 0    // último arg = Core 0
);
```
- El `onLoop()` UI sigue refrescando a 60 FPS mientras la conexión ocurre en background.

## Bluepad32 — `InputHAL` (gamepads)
- Singleton. `init()` durante setup tras WiFi (o como tu `main.cpp` decida; ahora está desactivado por defecto).
- `update()` debe llamarse en `loop()` cada iteración si está activo.
- Implementa stubs de callbacks aunque no los uses: callbacks nulos provocan crash.
- MAC Bluetooth visible vía `InputHAL::getInstance().getLocalMacAddress()` — necesaria para emparejar con `SixaxisPairTool`.

## Escaneo BLE (`ScreenBLEScan`)
- Escaneo bloqueante 5 s (`pBLEScan->start(5, false)`) — entra en margen del WDT (3 s feed + 8 s real). No subir.
- En `onExit()`: `pBLEScan->clearResults()` para liberar RAM antes de la siguiente pantalla (mitiga heap fragmentation).
- Optimizar el render del listado evitando `std::string::substr()` y truncados dinámicos. Usar aritmética de `char*` y `snprintf("%.6s", ...)` (ver README §6.1).

## Clasificación HID estándar
Para etiquetar dispositivos en logs y UI:
- HID: Service UUID `0x1812` o Appearance ≥ 960.
- Audio: Service UUID `0x180E` / `0x110B`.
- Formato log obligatorio:
  ```
  [BLE] Found: <MAC> | RSSI: <dBm> | Nature: <HID|AUDIO|GENERIC> | Name: <NAME>
  ```

## Pantalla en loop con conexión async — checklist
- `onEnter()`: lanzar task pinned Core 0, mostrar "Connecting..." en sprite.
- `onLoop()`: leer estado vía variable atómica/mutex, animar mientras `_connecting == true`.
- `onExit()`: `vTaskDelete` si la task sigue viva, `BLEClient::disconnect()` (NO `deinit`), `pBLEScan->clearResults()` si aplicable.

## Logging
- TAGs canónicos: `BP32` (Bluepad32), `BLE` (stack Arduino), `PAD` (lógica gamepad).
- Errores siempre con código: `Connect Failed: Timeout (Err 0x12)` — nunca `"Connect failed"` sólo.

## Estado actual real en repo
- `InputHAL::update()` está **comentado en `main.cpp`** ("DESACTIVADO BLE: Nos centramos en WiFi"). Si vas a reactivarlo:
  1. Descomentar `InputHAL::getInstance().init()` en `setup()`.
  2. Descomentar `InputHAL::getInstance().update()` en `loop()`.
  3. Descomentar registro de `ScreenGamepad`.
  4. Verificar que el WDT (3 s) sigue alimentado durante el handshake inicial.

## Antipatrones
| Hacer | NO hacer |
|---|---|
| Task pinned Core 0 para `connect()` | `connect()` desde `onLoop()` |
| Mantener stack BLE init | `BLEDevice::deinit(true)` |
| Stubs de callbacks vacíos | Pasar `nullptr` como callback |
| `clearResults()` en `onExit` | Acumular escaneos entre pantallas |
