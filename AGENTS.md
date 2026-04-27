# AGENTS.md — ESP32S3-TFT Framework

> Cross-tool entry point for AI coding agents (GitHub Copilot, Cursor, Codex,
> Aider, etc.). For the full context hierarchy, follow the references below.

## Project at a glance
- **Name:** ESP32S3 Tech Test Framework v2 — *Engineering sandbox*.
- **Hardware target:** ESP32-S3 Lolin S3 Pro + ILI9341 240x320 (HSPI 27 MHz).
- **PlatformIO board ID:** `esp32-s3-devkitc-1` (mantenido a propósito; el variant `lolin_s3` colisiona con los macros de pines de TFT_eSPI).
- **Stack:** PlatformIO + Arduino-ESP32 (stock). PSRAM **deshabilitada**. BLE/Bluepad32 retirados.
- **Architecture:** 4 capas estrictas — `src/hal/` → `src/core/` → `src/services/` → `src/screens/`.

## How to talk to this project (agent contract)
1. **No cargar `README.md` por defecto** (~38 KB, especificación viva). Consultar secciones puntuales bajo demanda.
2. **Empezar por `.github/copilot-instructions.md`** — contexto base ~1.5 KB, siempre cargado.
3. **Usar las skills de `.github/skills/`** cuando la tarea coincida con sus triggers (descripción YAML).
4. **Respetar `applyTo` de `.github/instructions/`** — instrucciones específicas por tipo de archivo.
5. **Persistir lecciones** en `/memories/repo/` (memoria local) cuando se descubra una regla dura nueva.

## Reglas duras (hard rules — NUNCA violar)
- Sin estado global. Singletons o miembros de clase.
- `new` en `onEnter()`, `delete` en `onExit()` para sprites pesados.
- Loop nunca bloquea > 50 ms; trabajo blocking → `xTaskCreatePinnedToCore` Core 0.
- Cero warnings de compilación.
- Estado compartido TFT ↔ Web vía `GOLConfig` (portMUX) o equivalente — nunca globales sin protección.
- WebServer single-threaded, ventana TCP ~5744 B → respuestas < 5 KB o backpressure manual.

## Skills (invocar por trigger semántico)
| Skill | Domain |
|---|---|
| `tft-screen` | Crear/editar `IScreen` + `BaseSprite` |
| `webservice-http` | Endpoints HTTP, GOLConfig, restricciones TCP |
| `ble-input` | Re-introducir BLE/HID si vuelve a hacer falta (retirado en F1) |
| `runtime-debug` | Panics, WDT, heap, captura serie |
| `iteration-close` | Cerrar iteración, changelog, sync README |
| `git-workflow` | Commits, tags, release, recovery |
| `build-pipeline` | Build/upload, logs, IntelliSense |
| `scaffold-new-project` | Clonar el framework como proyecto nuevo |

## Prompt files invocables (`.github/prompts/`)
- `/release` — orquesta `iteration-close` + `git-workflow` con input de versión.
- `/audit-screen` — audita una `IScreen` contra todas las hard rules.
- `/dump-status` — resumen estructurado del proyecto (versión, presets, tamaño PROGMEM blobs).

## Build / Deploy
- Compilar: `./build.ps1` → log en `<BuildDir>\.logs\build_latest.txt`.
- Subir: `./upload.ps1` → log en `<BuildDir>\.logs\upload_latest.txt`.
- Monitor con captura: `./monitor.ps1` → log timestamped en `<BuildDir>\.logs\monitor_<ts>.log`.
- Tests (lógica pura, host): `pio test -e native`.
- Static analysis: `pio check -e check`.

## Documentación de referencia
- `README.md` — especificación viva (37 secciones). No cargar entera.
- `DevGuidelines.md` — reglas duras y heurísticas extendidas.
- `.github/copilot-instructions.md` — contexto Copilot always-on.
- `.github/skills/<name>/SKILL.md` — workflows especializados.
- `.github/instructions/*.instructions.md` — reglas con `applyTo` por glob.

## Anti-patrones (PROHIBIDOS)
- `git push --force` sobre `main`.
- `git commit --amend` sobre commits ya pusheados.
- `BLEDevice::deinit(true)` en runtime (panic en re-init — vigente si se reintroduce BLE).
- `sendContent()` o chunked encoding en `WebService`.
- Sustituir el board ID por `lolin_s3` (rompe TFT_eSPI).
- Activar PSRAM sin verificar; el sandbox la mantiene OFF.
- Estado compartido TFT/Web sin `portMUX_TYPE`.
