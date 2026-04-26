---
applyTo: "src/services/**"
description: "General rules for service-layer singletons (Net, Time, Geo)."
---

# Services — reglas locales

- Todos los servicios son **singletons** con `getInstance()`. Sin estado global suelto.
- Métodos públicos exponen sólo POD (`int`, `bool`, `String` por valor) — nunca punteros internos mutables.
- Operaciones bloqueantes (`HTTP fetch`, `NTP sync`, `WiFi connect`):
  - Nunca dentro de `loop()`. Llamarlas desde `BootOrchestrator::run()` o desde tasks pinned a Core 0.
  - Implementar timeout duro y log de error con código (no `"Error"` solo).
- `TimeService::sync()` usa `pool.ntp.org` + `time.nist.gov`. Resync cada `NTP_SYNC_INTERVAL` (3600 s).
- `GeoService::fetchAndApply()` aplica TZ offset basado en IP — sin red ⇒ offset 0.
- TAGs reales: `NetService`, `TimeService`, `GeoService`. Mantenerlos.
- Cualquier servicio nuevo debe poder fallar de forma graceful: el `BootOrchestrator` continúa al siguiente paso.
