---
name: ble-input
description: "Use when reintroducing Bluetooth/BLE in ESP32S3-TFT Framework. BLE was retired in F1 (Apr 2026) — the framework no longer ships Bluepad32, InputHAL, ScreenGamepad or ScreenBLEScan. Triggers: 'añadir BLE', 'reintroducir gamepad', 'NimBLE-Arduino', 'ESP-HID host', 'mando PS5 ESP32', 'BLE scan', 'dual stack BLE'. Provides decision matrix and migration paths."
---

# Skill: BLE & Input — REINTRODUCTION GUIDE (post F1)

## Estado actual del repo
**BLE retirado del binario** (F1 commit). Lo que se eliminó:
- `src/hal/InputHAL.{h,cpp}` (Bluepad32).
- `src/screens/ScreenGamepad.h`, `src/screens/ScreenBLEScan.h`, `src/screens/ScreenBLE.h.disabled`.
- `platform_packages` (fork `ricardoquesada/esp32-arduino-lib-builder`) → ahora arduino-esp32 stock.
- `REQUIRES bluepad32` en `src/CMakeLists.txt`.

**Ahorro logrado**: −43 KB RAM, −256 KB Flash, toolchain alineada con upstream.

## Cuándo aplica esta skill
- El usuario quiere **volver a tener**: gamepads, scan BLE, periféricos HID, o un GATT server.

## Árbol de decisión

```
¿Qué tipo de dispositivo Bluetooth?
│
├─ Sólo HID estándar (PS4/PS5 modo BLE-HID, teclado/ratón BLE)
│   → Opción C: esp_hidh nativo + NimBLE-Arduino
│   → ~30 KB RAM, ~120 KB Flash
│
├─ Mandos propietarios (Switch Pro, Wii, DualSense rumble, Xbox One BLE)
│   → Opción A: Reintroducir Bluepad32 (re-añadir platform_packages)
│   → ~80 KB RAM, ~250 KB Flash
│
└─ Solo escanear/conectar a periféricos genéricos (heart-rate, sensores, beacons)
    → Opción B: NimBLE-Arduino (h2zero/NimBLE-Arduino)
    → ~30 KB RAM, ~120 KB Flash
```

## Opción B — NimBLE-Arduino (recomendada por defecto)

```ini
; platformio.ini
lib_deps =
  h2zero/NimBLE-Arduino @ ^1.4.1
build_flags =
  -D CONFIG_BT_NIMBLE_ENABLED=1
```

```cpp
#include <NimBLEDevice.h>
NimBLEDevice::init(Config::SYS_NAME);
auto* pScan = NimBLEDevice::getScan();
pScan->setActiveScan(true);
NimBLEScanResults res = pScan->getResults(5000, false);  // 5 s, valor
```

API casi idéntica a `BLEDevice` Arduino: prefijo `NimBLE` y `getResults(ms, restart)` en vez de `start(s, isContinue)`.

## Opción A — Reintroducir Bluepad32

```ini
[env:esp32-s3-devkitc-1]
platform_packages =
    framework-arduinoespressif32 @ https://github.com/ricardoquesada/esp32-arduino-lib-builder/releases/download/4.1.0/esp32-bluepad32-4.1.0.zip
    toolchain-xtensa-esp32s3 @ 12.2.0+20230208
```
Restaurar `src/CMakeLists.txt` con `REQUIRES bluepad32`. Recrear `InputHAL` (ver historial git pre-F1).

**Coste**: vuelves a arrastrar Bluedroid + BT-Classic, toolchain rezagada ~6 meses respecto al upstream Espressif (perderás `esp_task_wdt_config_t` y otras APIs IDF v5 modernas).

## Opción C — esp_hidh + NimBLE (avanzada)

Componente IDF oficial `esp_hidh` (`#include <esp_hidh.h>`). Funciona sobre NimBLE-only. Requiere escribir un `InputHAL` nuevo (~300 LOC) que escuche callbacks HID y mapee el descriptor a estado de gamepad. **No** soporta Switch Pro nativo (handshake propietario).

## Reglas duras heredadas (vigentes si se reactiva)

- **Nunca** `BLEDevice::deinit(true)` / `NimBLEDevice::deinit(true)` en runtime → panic en re-init en `esp_bt_controller_mem_release`. Mantener stack vivo.
- **`BLEClient::connect()` bloqueante > 8 s** → SIEMPRE en `xTaskCreatePinnedToCore` Core 0:

```cpp
xTaskCreatePinnedToCore(
    [](void* arg) {
        auto* self = static_cast<MyScreen*>(arg);
        self->connectAsync();
        vTaskDelete(NULL);
    },
    "BLEConnect", 8192, this, 1, &_taskHandle, 0    // Core 0
);
```
- **Scan BLE máximo 5 s consecutivos** (margen WDT 8 s boot / 3 s loop).
- En `onExit()`: `pScan->stop()` + `pScan->clearResults()` antes de soltar la pantalla.

## Coexistencia RF (Wi-Fi + BLE comparten radio 2.4 GHz)

Mientras `WebService` sirva HTTP y haya scan/connect BLE concurrente, los slots se reparten y aparecen:
- Scan devolviendo menos resultados.
- Connect time-out aleatorio.
- HTTP latencia errática (200-2000 ms).

Mitigación cuando se reactive BLE:
```cpp
WiFi.setSleep(WIFI_PS_NONE);   // antes de connect/scan denso
// ... operación BLE ...
WiFi.setSleep(WIFI_PS_MIN_MODEM);   // restaurar
```

Referencia: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/coexist.html>.

## Clasificación HID estándar (para UI/log)
- HID: Service UUID `0x1812` o Appearance ≥ 960.
- Audio: Service UUID `0x180E` / `0x110B`.
- Formato log obligatorio:
  ```
  [BLE] Found: <MAC> | RSSI: <dBm> | Nature: <HID|AUDIO|GENERIC> | Name: <NAME>
  ```

## TAGs de logger (cuando se reactive)
- `BLE` — stack BLE genérico (NimBLE/Bluedroid).
- `BP32` — Bluepad32 (sólo si Opción A).
- `PAD` — lógica de gamepad/HID por encima del stack.
