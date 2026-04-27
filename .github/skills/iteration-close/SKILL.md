---
name: iteration-close
description: "Use when closing a development iteration in ESP32S3-TFT Framework: bumping version, writing the changelog entry, validating that README/DevGuidelines match the code, and ensuring all hard rules pass before tagging. Triggers: 'cerrar iteración', 'changelog', 'release notes', 'bump versión', 'sincronizar README', 'antes de commit final'."
---

# Skill: Iteration Close (ESP32S3-TFT Framework)

## Cuándo aplica
Antes de declarar terminada una iteración (bump de versión, commit final, opcional tag).

## Pre-flight checklist (TODOS deben pasar)

### 1. Build limpio
```powershell
./build.ps1
```
- Resultado `SUCCESS`.
- **Cero warnings** (política dura del proyecto).
- Comparar uso RAM/Flash con la iteración anterior:
  - Salto > 5 % en RAM estática → justificar en changelog o refactorizar a heap dinámico bajo demanda.
  - Salto > 10 % en Flash → revisar PROGMEM blobs grandes (`PALETTES_JSON`, `DASH_HTML`, `DASH_JS`).

### 1.b Tests host (Unity)
```powershell
pio test -e native
```
- Todos los tests bajo `test/test_native_*` deben pasar.
- Si falla por `gcc no se reconoce` en Windows, no bloquea localmente (CI lo cubre); documentar en el commit que se delegó a CI.
- Si introduces helpers puros nuevos, valorar añadir test correspondiente.

### 1.c Análisis estático (cppcheck)
```powershell
pio check -e check
```
- **Cero defects HIGH** (gate duro antes de tag).
- Defects MEDIUM permitidos solo si están justificados o son legacy conocido.
- Si introduce defects nuevos vs iteración anterior, resolverlos o suprimirlos con `// cppcheck-suppress <id>` justificado en línea.

### 2. Smoke test físico
```powershell
./upload.ps1
```
- Boot completo sin LED rojo de panic.
- `ScreenStatus` muestra IP, RSSI, MAC, URL, PIN.
- Web dashboard accesible: login con PIN → 3 pestañas cargan secuencialmente.
- Cada pantalla del carrusel renderiza al menos 5 s sin black screen.

### 3. Sincronización doc ↔ código
Revisar diff de la iteración y propagar:
| Si tocaste… | Actualizar… |
|---|---|
| Una `IScreen` (nueva/modificada) | `README.md §1.1` (lista de pantallas, estado `[Activo/Latente]`) |
| `WebService` (rutas, tamaños) | `README.md §6.11` (tabla de rutas + tamaños) |
| `platformio.ini` (flags, libs) | `README.md §2.2 / §2.3` |
| Pinout o brillo LED | `README.md §2.1 / §3.5` |
| Patrón nuevo (TCP, BLE, sprite) | `DevGuidelines.md` sección correspondiente |
| TAGs de logs nuevos | `.github/copilot-instructions.md` lista de TAGs |

### 4. Changelog
Añadir entrada en `README.md §7` siguiendo formato exacto:
```markdown
### vX.Y.Z — YYYY-MM-DD
**Título resumen:**
*   Cambio 1.
*   Cambio 2.

**Bugs Resueltos:**
*   Fix: <síntoma> — <causa raíz>.
```
- Incrementar versión según semver (proyecto sandbox: `0.MINOR.PATCH`).
- Si la iteración rompe API/ABI interna entre módulos → bump de MINOR.
- Solo fixes / pulido → bump de PATCH.

### 5. Estado de pantallas
Marcar en README §1.1 con etiqueta correcta:
- `[Activo]` — funcional, en carrusel.
- `[Latente]` — código presente, no registrado en `main.cpp`.
- `[Boot]` — sólo durante arranque.
- `[Deprecated]` — pendiente de eliminar.

### 6. Skills y prompts
- Si introdujiste un patrón nuevo (gestión de memoria, comunicación, protocolo), valorar si requiere actualizar una skill (`tft-screen`, `webservice-http`, `ble-input`) o crear una nueva.
- Si una skill quedó obsoleta por refactor, actualizarla en el mismo PR.

### 7. Limpieza
- Borrar logs grandes generados durante debug que no aportan (`pio_monitor.log` > 100 KB).
- Mover scripts experimentales `.py`/`.ps1` que ya no se usan a `aux_/` (no a la raíz).
- Comentarios `// TODO`, `// FIXME`, `// HACK` → o se resuelven o se documentan en README.

### 8. CI verde
- Confirmar que el último run de `.github/workflows/ci.yml` en `main` está en verde (jobs `build` y `test`).
- Si el job `check` (cppcheck) introduce HIGH nuevos, resolver antes del tag.
- Si el push a `main` aún no se ha hecho, anotarlo y validar tras el push.

## Comando de cierre sugerido
```powershell
git status
git add -A
git commit -m "chore: close iteration v0.X.Y — <resumen>"
# Tag anotado opcional (no push automático):
git tag -a v0.X.Y -m "v0.X.Y — <título del changelog>"
```
- **No** hacer `git push` ni `git push --tags` automáticamente — confirmar con el humano.
- Detalle completo del flujo commit + tag + push en la skill **`git-workflow`**.

## Antipatrones
- Cerrar iteración con warnings activos "los miramos luego".
- Subir versión sin tocar changelog.
- Cambiar firma de `IScreen` o `BaseSprite` sin actualizar la skill `tft-screen`.
- Documentar funcionalidad que aún no está en `main.cpp` registrada.
