---
mode: agent
description: "Genera un resumen estructurado del estado del proyecto: versión, presets, tamaños PROGMEM, pantallas activas."
---

# /dump-status

Genera un dump estructurado del estado actual del proyecto **ESP32S3-TFT Framework**. Útil para snapshots antes de release o para abrir conversaciones con contexto pre-cargado.

## Recolección (usar tools de búsqueda, NO modificar nada)

### 1. Versión
- Leer `SYS_VERSION[]` en `src/core/Config.h`.
- Último tag: `git describe --tags --abbrev=0` (si existe).
- Commits desde último tag: `git log <tag>..HEAD --oneline | Measure-Object`.

### 2. Pantallas registradas
- Grep `ScreenManager::registerScreen` en `src/main.cpp`.
- Listar cada pantalla con su clase y estado (activa = registrada, latente = .h presente sin registro).

### 3. Servicios activos
- Verificar cuáles están instanciados en `BootOrchestrator::run()`:
  - `NetService`, `TimeService`, `GeoService`, `WebService`.

### 4. Tamaño build (último log disponible)
- Leer `C:\Users\egavi\pio_temp_build\ESP32S3-TFT_Framework\.logs\build_latest.txt`:
  - Línea `RAM:` → uso estático.
  - Línea `Flash:` → tamaño firmware.

### 5. Blobs PROGMEM grandes (si presets aplicables)
- `PALETTES_JSON[]` en `src/core/PalettesData.h` → tamaño aproximado del array.
- `DASH_HTML[]` y `DASH_JS[]` en `src/services/WebService.cpp` → confirmar < 5 KB cada uno (límite TCP).

### 6. Endpoints HTTP (si WebService activo)
- Grep `server.on("` o `server.on('/` en `src/services/WebService.cpp`.
- Listar ruta + verbo.

### 7. Skills + prompts disponibles
- `Get-ChildItem .github\skills -Directory` → listar nombres.
- `Get-ChildItem .github\prompts\*.prompt.md` → listar nombres.

## Output esperado (formato fijo)

```markdown
# Project Status — <yyyy-mm-dd hh:mm>

## Identidad
- **Nombre:** ESP32S3-TFT Framework
- **Versión:** <SYS_VERSION>
- **Último tag:** <tag o N/A>
- **Commits desde tag:** <N>

## Pantallas
| Clase | Estado | Fichero |
|---|---|---|
| ScreenStatus | [Activa] | src/screens/ScreenStatus.h |
| ... | ... | ... |

## Servicios
- [x] NetService
- [ ] WebService (no instanciado)
- ...

## Build (último)
- RAM: 27.5% (90128 / 327680)
- Flash: 39.9% (1333149 / 3342336)

## PROGMEM blobs
| Símbolo | Tamaño | Límite | Estado |
|---|---|---|---|
| PALETTES_JSON | ~27 KB | n/a (backpressure) | OK |
| DASH_HTML | 4.2 KB | 5 KB | OK |
| DASH_JS | 4.8 KB | 5 KB | ⚠️ cerca de límite |

## Endpoints HTTP
- GET / → login
- POST /api/login
- ...

## Toolchain agente
- Skills: tft-screen, webservice-http, ble-input, runtime-debug, iteration-close, git-workflow, build-pipeline, scaffold-new-project
- Prompts: release, audit-screen, dump-status
```

## Reglas
- **Solo lectura.** No editar archivos.
- Si un dato no es accesible (ej. build log no existe), reportar `N/A` con motivo.
- Devolver el dump completo en un solo mensaje, sin pedir aclaraciones.
