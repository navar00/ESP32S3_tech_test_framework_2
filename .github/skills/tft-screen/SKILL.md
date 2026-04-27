---
name: tft-screen
description: "Use when creating, editing or registering a new TFT screen in ESP32S3-TFT Framework (ESP32-S3 ILI9341 240x320). Triggers: 'nueva pantalla', 'crear pantalla TFT', 'IScreen', 'BaseSprite', 'sprite 8bpp', 'retro terminal UI', 'registrar en ScreenManager', 'fillTriangle agujas', 'paleta 332'. Provides exact IScreen signature, BaseSprite usage with graceful 16→8→4 bpp degradation, retro green UI standard, and registration in main.cpp."
---

# Skill: TFT Screen (ESP32S3-TFT Framework)

## Cuándo aplica
Crear o modificar archivos en `src/screens/` que implementen `IScreen` para el TFT ILI9341 240×320.

## Firma real de `IScreen` (NO inventar otros métodos)
```cpp
class IScreen {
public:
    virtual const char* getName() = 0;
    virtual void onEnter() { DisplayHAL::getInstance().clear(); } // ya borra TFT por defecto
    virtual void onLoop() = 0;
    virtual void onExit() {}
    virtual void on1SecTimer() {}   // disparado por ISR 1 Hz vía ScreenManager::dispatchTimer()
    virtual ~IScreen() {}
};
```
- `onEnter()` ya llama a `clear()` — no duplicar.
- `on1SecTimer()` corre desde `loop()` (no ISR), seguro para SPI.
- Ojo: no existe `onShow`, `onUpdate`, `onTick` ni `setup`.

## Plantilla mínima

```cpp
// src/screens/ScreenMyView.h
#pragma once
#include "IScreen.h"
#include "BaseSprite.h"
#include "../core/Logger.h"
#include "../hal/Hal.h"

class ScreenMyView : public IScreen, public BaseSprite {
public:
    const char* getName() override { return "MYVIEW"; }

    void onEnter() override {
        IScreen::onEnter();                       // clear TFT
        init(DisplayHAL::getInstance().getRaw()); // alloc sprite (16→8→4 bpp auto)
        Logger::log("MYVIEW", "Entered");
    }

    void onLoop() override {
        if (!isReady()) return;                   // graceful degradation
        clear();
        drawHeader("MY VIEW");
        // ... dibujado en sprite ...
        push();                                   // pushSprite(0,0) ÚNICO por frame
    }

    void on1SecTimer() override { /* refresh datos cada 1s si aplica */ }

    void onExit() override {
        free();                                   // libera sprite (RAII)
        Logger::log("MYVIEW", "Exited");
    }
};
```

## Reglas duras
- **Sprite obligatorio**: prohibido `tft.drawString()` directo sobre el TFT. Todo en el sprite, un único `push()` por frame.
- **8bpp usa mapeo nativo 332** — NUNCA llamar `createPalette()` (causa colores invisibles, ver README §6.8).
- **Líneas finas** (agujas, bordes 1 px): usar `fillTriangle` con base ≥ 2 px, no `drawLine` (ver §6.9).
- **`new`/`delete`** sólo dentro de `onEnter`/`onExit`. Nada estático grande.
- **No bloquear > 50 ms** en `onLoop()`. Si necesitas red/BLE → `xTaskCreatePinnedToCore` Core 0.

## Estética "Retro Terminal" (obligatoria)
| Elemento | Valor |
|---|---|
| Fondo | `TFT_BLACK` |
| Datos/texto | `TFT_GREEN` |
| Cabecera | inverso (`TFT_GREEN` fondo + `TFT_BLACK` texto) — `drawHeader()` lo hace |
| Separadores | `TFT_DARKGREEN` — `drawSeparator(y)` |
| Tipografía | `setTextFont(2)` (sans pixelada ~16 px) |
| Leading | 18 px fijos |
| Margen X | 4 px |

Grid estándar para listas densas (ancho 320):
- Col 1 (ID/MAC): x=4 · Col 2 (métrica): x=90 · Col 3 (tipo): x=140 · Col 4 (detalle): x=190.

## Registro en `main.cpp`
Añadir en la sección 7 de `setup()`, antes de `switchTo(0)`:
```cpp
ScreenManager::getInstance().add(new ScreenMyView());
```
El orden determina la posición en el carrusel (botón BOOT GPIO 0 → `next()`).

## Logging
Usa `Logger::log("TAG", "fmt", ...)`. TAGs canónicos del proyecto: `BOOT`, `BOOT_UI`, `MAIN`, `MGR`, `GFX`, `WDT`, `BP32`, `BLE`, `WebSrv`, `STATUS`, `TimeService`, `NetService`, `GeoService`, `DISPLAY`. Crea uno nuevo corto y descriptivo si tu pantalla lo merece (ej. `GOL`, `CLK`).

## Referencias en código
- Ejemplo simple: `src/screens/ScreenStatus.h`
- Ejemplo con compartir estado con Web: `src/screens/ScreenGameOfLife.h` + `src/core/GOLConfig.h` (ver skill `webservice-http`).
- Ejemplo 7-segmentos custom: `src/screens/ScreenFlipClock.h` (README §6.6).
- Ejemplo doble vista (reloj+calendario): `src/screens/ScreenAnalogClock.h`.

## Antipatrones documentados (no repetir)
- `createPalette(nullptr, 16)` con sprite 8bpp → elementos invisibles aleatorios. Ver README §6.8.
- `drawLine` 1 px en 8bpp → flickering / invisibilidad. Usar `fillTriangle`.
- `Font 7` LCD sobre 8bpp con `setTextSize(2)` → colors clipped. Renderizar 7-seg manual con `fillRect`.
