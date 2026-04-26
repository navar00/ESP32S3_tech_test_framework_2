---
applyTo: "src/core/GOLConfig.h"
description: "Shared-state contract pattern for state shared between TFT loop and HTTP handlers."
---

# Shared State — reglas locales (GOLConfig pattern)

- Solo POD dentro de `State` (uint16_t, bool, int). Nada de `String`, `std::vector`, punteros — no `portMUX`-safe.
- Toda lectura/escritura entre `portENTER_CRITICAL(&_mux)` / `portEXIT_CRITICAL(&_mux)`.
- `getState()` devuelve **copia**, no referencia. El caller no retiene referencias entre frames.
- Acciones puntuales (reset, clear, trigger) → flags efímeros consumidos por el loop con `consumeTriggers(out...)` — el handler HTTP solo activa.
- `_mux = portMUX_INITIALIZER_UNLOCKED` en el constructor.
- Patrón replicable para cualquier configurable web nueva: crear `MyConfig` análogo en `src/core/`, registrarlo desde `WebService` y consumirlo desde la pantalla.
- Para detalle: skill `webservice-http` sección "Patrón GOLConfig".
