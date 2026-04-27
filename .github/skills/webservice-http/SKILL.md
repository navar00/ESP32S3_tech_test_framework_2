---
name: webservice-http
description: "Use when adding/modifying HTTP endpoints, handling the embedded WebServer, sharing state between TFT screens and the web dashboard, or hitting TCP buffer limits in ESP32S3-TFT Framework. Triggers: 'WebService', 'endpoint HTTP', 'TCP 5744', 'ERR_CONTENT_LENGTH_MISMATCH', 'ERR_CONNECTION_RESET', 'PROGMEM dashboard', 'split-payload', 'GOLConfig', 'estado compartido TFT web', 'portMUX', 'cookie auth', 'PIN dashboard'. Encodes the hard-won constraints of ESP32-S3 single-threaded WebServer."
---

# Skill: WebService HTTP + Shared State (ESP32S3-TFT Framework)

## Cuándo aplica
- Añadir/editar rutas en `src/services/WebService.{h,cpp}`.
- Compartir estado entre `loop()` (TFT) y handlers HTTP.
- Diagnosticar `ERR_CONTENT_LENGTH_MISMATCH`, `ERR_CONNECTION_RESET`, `ERR_INVALID_CHUNKED_ENCODING`.

## Restricciones críticas del hardware (NO negociables)
1. **Ventana TCP del ESP32-S3 ≈ 5744 bytes**. Toda respuesta servida con `_server.send()` o `send_P()` debe tener cuerpo **< 5 KB**. Por encima → short-write silencioso → `ERR_CONTENT_LENGTH_MISMATCH`.
2. **Prohibido `sendContent()` y chunked encoding**. Hace 3 writes internos por chunk; cualquiera parcial corrompe el stream.
3. **WebServer es single-threaded**. JS cliente: encadenar `fetch().then().then()`, jamás peticiones paralelas.
4. **No escribir directamente al `WiFiClient`** mezclado con `_server.send()` en la misma respuesta.

## Patrón Split-Payload (HTML + JS separados)
HTML referencia `<script src='/d.js'>` para que el navegador haga 2 peticiones secuenciales:
```cpp
_server.on("/dashboard", HTTP_GET, [this]() {
    if (!checkAuth()) return;
    _server.send_P(200, "text/html", DASH_HTML);   // < 5 KB PROGMEM
});
_server.on("/d.js", HTTP_GET, [this]() {
    if (!checkAuth()) return;
    _server.send_P(200, "application/javascript", DASH_JS); // < 5 KB PROGMEM
});
```

## Patrón WiFiClient Backpressure (payload > 5 KB)
Para JSON grandes (ej. `PALETTES_JSON` ~27 KB):
```cpp
_server.setContentLength(len);
_server.sendHeader("Connection", "close");
_server.send(200, "application/json", "");        // sólo headers

WiFiClient client = _server.client();
char buf[256];
size_t pos = 0;
while (pos < len && client.connected()) {
    size_t toSend = min(len - pos, (size_t)256);
    memcpy_P(buf, DATA + pos, toSend);
    size_t sent = 0; uint8_t retries = 0;
    while (sent < toSend && client.connected() && retries < 50) {
        int w = client.write((const uint8_t*)(buf + sent), toSend - sent);
        if (w > 0) { sent += w; retries = 0; }
        else       { retries++; delay(5); }
    }
    pos += sent;
    yield();   // alimenta WDT
}
```

## Patrón GOLConfig — estado compartido TFT ↔ Web
Plantilla de oro para cualquier nueva sección configurable desde la web (`src/core/GOLConfig.h`):

```cpp
class MyConfig {
public:
    static MyConfig& getInstance();
    struct State { /* solo POD: enteros, bool, uint16_t */ };

    State getState() {
        portENTER_CRITICAL(&_mux);
        State s = _state;
        portEXIT_CRITICAL(&_mux);
        return s;                    // copia, no referencia
    }
    void updateX(uint16_t v) { portENTER_CRITICAL(&_mux); _state.x = v; portEXIT_CRITICAL(&_mux); }

    // Triggers efímeros: handlers HTTP los activan, loop los consume y reset
    void triggerReset()             { portENTER_CRITICAL(&_mux); _state.tReset = true; portEXIT_CRITICAL(&_mux); }
    void consumeTriggers(bool& r)   { portENTER_CRITICAL(&_mux); r = _state.tReset; _state.tReset = false; portEXIT_CRITICAL(&_mux); }
private:
    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    State _state;
};
```
- HTTP handler escribe vía `updateX()` / `triggerReset()`.
- `IScreen::onLoop()` lee con `getState()` y consume triggers — **nunca** retiene la referencia entre frames.
- Sólo POD; nada de `String`, `std::vector`, etc. dentro de `State` (no `portMUX`-safe para construir/copiar).

## Esqueleto de nueva ruta API
```cpp
// En WebService::registerHandlers()
_server.on("/api/mymodule/state", HTTP_GET, [this]() {
    if (!checkAuth()) return;
    auto s = MyConfig::getInstance().getState();
    char json[256];
    snprintf(json, sizeof(json), "{\"x\":%u,\"running\":%s}",
             s.x, s.running ? "true" : "false");
    _server.send(200, "application/json", json);
});

_server.on("/api/mymodule/config", HTTP_POST, [this]() {
    if (!checkAuth()) return;
    if (!_server.hasArg("x")) { _server.send(400, "text/plain", "missing x"); return; }
    MyConfig::getInstance().updateX(_server.arg("x").toInt());
    _server.send(200, "application/json", "{\"ok\":true}");
});
```
- `checkAuth()` valida cookie `auth=<sessionToken>`.
- Para acciones puntuales usar `triggerXxx()`, no `updateXxx()`.

## Cliente JS (DASH_JS) — encadenar peticiones
```javascript
function loadAll() {
  return fetch('/api/system').then(r=>r.json()).then(s=>render(s))
    .then(() => fetch('/api/gol/state')).then(r=>r.json()).then(g=>renderGOL(g))
    .then(() => fetch('/api/palettes')).then(r=>r.json()).then(p=>renderPalettes(p));
}
```
- ❌ Mal: `loadSystem(); loadGOL(); loadPalettes();` (paralelo → 2 fallan con `ERR_CONNECTION_RESET`).

## Seguridad
- PIN 4 dígitos via `esp_random()` en boot — visible en `ScreenStatus` y log `[WebSrv]`.
- Token sesión por boot (cookie `auth=`) — invalida sesiones tras reset.
- Toda ruta que no sea `/`, `/login`, `/favicon.ico` debe llamar `checkAuth()`.

## Logging y diagnóstico
- TAG `WebSrv` para todo handler. Logear método, URI e IP cliente en 404.
- Si en navegador aparece `ERR_CONTENT_LENGTH_MISMATCH`: medir tamaño exacto del payload — si > 5500 B, dividir o pasar a backpressure.
- Tabla completa de fallos→solución en README §6.12.

## Antipatrones (registrados en proyecto)
| Intento | Falla con |
|---|---|
| `_server.send(200, "text/html", page)` con page > 5,7 KB | `ERR_CONTENT_LENGTH_MISMATCH` |
| `sendContent()` en loop | `ERR_INVALID_CHUNKED_ENCODING` |
| Raw `WiFiClient::write()` con headers manuales mezclado con `_server.send()` | `errno 104 Connection reset by peer` |
| `BLEDevice::deinit(true)` para liberar RAM antes de servir HTTP | Panic en re-init BLE |
| `fetch()` paralelos en JS | `ERR_CONNECTION_RESET` |
