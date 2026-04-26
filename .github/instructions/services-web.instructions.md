---
applyTo: "src/services/WebService.*"
description: "Hard TCP/HTTP rules for the embedded WebServer in TechTest v2."
---

# WebService — reglas locales

- Ventana TCP del ESP32-S3 ≈ **5744 bytes**. `_server.send()` / `send_P()` SOLO con cuerpo < 5 KB.
- Para payloads > 5 KB: patrón **WiFiClient backpressure** (chunks 256 B + retry hasta 50 con `delay(5)` + `yield()`).
- **Prohibido** `sendContent()` y chunked encoding.
- WebServer es **single-threaded**: handlers no concurrentes. Cliente JS encadena `fetch().then().then()` — nunca paralelo.
- Patrón **Split-Payload** para HTML+JS: dos blobs PROGMEM independientes < 5 KB cada uno; HTML referencia `<script src='/d.js'>`.
- Toda ruta no pública valida cookie `auth=<sessionToken>` con `checkAuth()`.
- Estado compartido con `loop()` / pantallas TFT vía `GOLConfig` (portMUX) o equivalente — nunca globales sin protección.
- Logger TAG: `WebSrv`. 404 logean método, URI, IP del cliente.
- Para detalle: skill `webservice-http`.
