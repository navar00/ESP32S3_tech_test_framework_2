---
applyTo: "src/hal/**"
description: "Rules for the Hardware Abstraction Layer (singletons + facade)."
---

# HAL — reglas locales

- Cada HAL es **singleton** (`static getInstance()`). Sin instancias múltiples.
- `Hal.h` es facade: solo `#include` de los HALs concretos. No añadir lógica ahí.
- Métodos públicos abstraen completamente la librería subyacente — el caller jamás incluye `TFT_eSPI.h`, `Adafruit_NeoPixel.h` directamente fuera del HAL.
- `init()` idempotente: llamadas repetidas no rompen.
- Operaciones bloqueantes en HAL prohibidas — moverlas a la capa de servicios o a tasks dedicadas.
- `WatchdogHAL::feed()` debe ser barato y sin asignaciones.
- TAGs canónicos: `DISPLAY`, `BP32`. Crea uno corto si añades un HAL nuevo.
