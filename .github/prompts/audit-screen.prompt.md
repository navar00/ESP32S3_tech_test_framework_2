---
mode: agent
description: "Audita un fichero IScreen contra todas las hard rules del proyecto y reporta violaciones."
---

# /audit-screen

Audita el fichero `${input:screenPath:src/screens/...}` contra las **hard rules** del framework ESP32S3-TFT.

## Hard rules a verificar (basadas en `tft-screen` SKILL + `screens.instructions.md`)

### 1. Memoria de sprites
- [ ] **`new` SOLO en `onEnter()`** y `delete` SOLO en `onExit()` para sprites pesados (>10 KB).
- [ ] Si usa `BaseSprite` con degradación 16→8→4 bpp: comprobar fallback chain.
- [ ] **Nunca `createPalette()` en 8 bpp** — usar mapping 332 nativo.
- [ ] No `malloc` / `new` directo en `loop()` o `update()`.

### 2. Estado
- [ ] **Sin variables globales** fuera de la clase.
- [ ] Estado compartido con Web → vía `GOLConfig` con `portMUX_TYPE`. No globales sueltos.
- [ ] Miembros `static` solo si son `constexpr` o sentinels.

### 3. IScreen interface
- [ ] Implementa `onEnter()`, `onExit()`, `update(uint32_t dt)`, `getName()`.
- [ ] `getName()` devuelve string PROGMEM o literal const.
- [ ] Si la pantalla es activa: registrada en `main.cpp` vía `ScreenManager::registerScreen()`.

### 4. Render budget
- [ ] `update()` no bloquea > 50 ms. Si hay operaciones pesadas → trabajo asíncrono o partial draws.
- [ ] No `delay()` dentro de `update()` o `onEnter()`.

### 5. Logging
- [ ] Usa `Logger::log("TAG", fmt, ...)` con TAG canónico (BOOT, GFX, MGR, WDT, etc.).
- [ ] Errores incluyen código/contexto, no solo "Error".

### 6. Estética retro-terminal
- [ ] Fondo `TFT_BLACK`, texto `TFT_GREEN`, separadores `TFT_DARKGREEN`.
- [ ] Fuente `Font 2` (~16 px), leading 18 px, margen X 4 px.
- [ ] Headers en modo inverso si la pantalla los usa.

## Output esperado
Para cada hard rule:
- ✅ **PASS** — referencia a la línea.
- ❌ **FAIL** — línea + cita del código + sugerencia de fix.
- ⚠️ **WARN** — caso ambiguo a revisar manualmente.

Resumen al final:
```
Auditoría de <screenPath>:
- PASS: N/M reglas
- FAIL: <count>
- WARN: <count>

Acción recomendada: [APROBAR | CORREGIR FAILS | REVISAR WARNS]
```

## Reglas del agente
- **NO modificar el archivo** durante la auditoría — solo reportar.
- Si el fichero no existe o no es una `IScreen`, reportar y abortar.
- Citar líneas exactas con formato `[file](path#Lnn)`.
