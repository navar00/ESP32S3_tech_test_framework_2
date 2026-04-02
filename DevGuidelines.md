# DIRECTRICES DE INGENIERÍA & GOBIERNO DEL CÓDIGO
> **Estatus**: OBLIGATORIO
> **Propósito**: Definir el estándar de calidad para la colaboración Humano-IA.

El desarrollo en este proyecto no busca solo funcionalidad ("que funcione"), sino robustez, mantenibilidad y observabilidad ("que perdure y se entienda").

---

## 0. ESTÁNDARES DE REFERENCIA Y ARQUITECTURA
El código debe aspirar a cumplir con los principios de:
*   **C++ Core Guidelines**: Enfoque en seguridad de tipos, inmutabilidad (const) y gestión de recursos (RAII).
*   **Embedded C++**: Énfasis en determinismo, control estricto de memoria y eficiencia de ciclos.

### 0.1 Arquitectura "Virtuoso" (Abstracta, Modular, Estable)
El proyecto sigue una arquitectura estricta de capas para garantizar escalabilidad y testeabilidad:
1.  **HAL (Hardware Abstraction Layer)**: (`/src/hal`)
    *   Aísla el código de drivers específicos (TFT_eSPI, FastLED).
    *   Uso de **Singleton/Facade** para acceso centralizado al hardware.
2.  **Core Logic**: (`/src/core`)
    *   Lógica pura de negocio y gestión de estado. Independiente de la UI.
    *   Modularidad basada en responsabilidad única (Logger, ScreenManager).
3.  **UI Strategy**: (`/src/screens`)
    *   Implementación del patrón **Strategy** (`IScreen`).
    *   Cada pantalla es un módulo autocontenido con ciclo de vida propio (`onEnter`, `onLoop`, `onExit`).
    *   **Inversion of Control**: El `ScreenManager` orquesta el flujo, las pantallas no se conocen entre sí.

### 0.2 Clasificación de Reglas
Este documento contiene dos tipos de directrices:

1.  **REGLAS DURAS (MANDATORY)**
    *   Su incumplimiento invalida una entrega.
    *   *Ejemplos*: Compilación limpia, no bloquear el loop principal, gestión de memoria RAII.
2.  **GUÍAS HEURÍSTICAS (STRONGLY RECOMMENDED)**
    *   No son absolutas, pero romperlas requiere justificación explícita.
    *   *Ejemplos*: Estilo de logs, optimizaciones gráficas agresivas.
    *   **Norma**: Si una heurística se rompe, debe documentarse el *porqué* (comentario + README).

---

## 1. ROBUSTEZ Y DEBUGGING (Design for Observability)
Un software robusto no es el que nunca falla, sino el que (A) se recupera del fallo de forma segura y (B) explica claramente por qué falló para facilitar su diagnóstico.

### 1.1 Estrategia de Logging (Runtime)
El log es la única herramienta de diagnóstico en tiempo real. Debe ser estructurado, parseable y estético.
*   **Formato Único**: `[Timestamp] [TAG   ] Mensaje`
*   **Uso de TAGs**: Identificadores alineados de longitud fija (3-5 chars) para facilitar lectura vertical.
    *   `[BOOT]` -> Inicialización del sistema.
    *   `[MGR ]` -> Orquestador de Pantallas (State Machine).
    *   `[BLE ]` -> Eventos del Stack Bluetooth.
    *   `[PAD ]` -> Lógica específica de Gamepad/Input.
    *   `[GFX ]` -> Motor gráfico y alertas de alocación de memoria.
*   **Criterio de Contenido**:
    *   **Info**: Cambios de estado relevantes.
    *   **Debug**: Dumps hexadecimales de tramas de datos (imprescindible para protocolos).
    *   **Error**: Fallos críticos. *Nunca* imprimir solo "Error", siempre adjuntar código de error, estado o contexto.
    *   *Ejemplo*: Evitar `Connect failed`; Preferir `Connect Failed: Timeout (Err 0x12)`.

### 1.2 Debugging en Compilación
El compilador es la primera línea de defensa.
*   **Política de Cero Warnings**: Un warning suele ser un bug latente. En este proyecto se tratan como errores bloqueantes.
*   **Análisis Estático**: Si una estructura de datos requiere casts complejos o punteros void inseguros para compilar, revísala. Probablemente el diseño esté mal.

---

## 2. EL RITUAL DE COMPILACIÓN (Design by Compilation)
**Principio**: El ciclo de compilación es una herramienta de diseño, no un trámite.

1.  **Validación Binaria**:
    *   Todo cambio se valida ejecutando `./build.ps1`.
    *   Es obligatorio analizar el éxito *y* el tiempo de compilación.
2.  **Auditoría de Recursos**:
    *   Revisar siempre el footprint de RAM y Flash al final de la compilación.
    *   Si el uso de RAM estática salta drásticamente (>5%), es obligatorio justificarlo o refactorizar para usar Heap dinámico bajo demanda.

---

## 3. LA CONSTITUCIÓN DEL PROYECTO (README.md)
**Principio**: Código y Documentación son atómicos. No se actualiza uno sin el otro.

1.  **Single Source of Truth**:
    *   El `README.md` es la especificación técnica viva.
    *   Antes de codificar, **lee** la arquitectura definida.
    *   Si la arquitectura en el README es inviable, **actualiza el README primero**, luego el código.
2.  **Historia Incremental**:
    *   No sobrescribas decisiones pasadas sin dejar rastro.
    *   Usa el README para marcar funciones antiguas como `[Deprecated]`, `[Latent]` o `[Active]`.

---

## 4. ARQUITECTURA DEFENSIVA (ESP32-S3 Specifics)
**Principio**: El hardware es finito y hostil. El software nunca debe asumir recursos infinitos.

1.  **Gestión de Memoria (RAM/PSRAM)**:
    *   **Prohibido** crear objetos pesados (Sprites > 1KB) en el Global Scope o Stack.
    *   Usar siempre asignación dinámica controlada (`new` en `onEnter`, `delete` en `onExit`).
    *   Implementar siempre **Graceful Degradation**: Si `alloc` falla, el sistema debe funcionar en modo reducido (e.g., 8bpp en lugar de 16bpp), nunca reiniciar.
2.  **Gestión de Tiempo (Watchdogs)**:
    *   El Loop Principal (`onLoop`) es sagrado para el refresco de UI (60 FPS).
    *   Cualquier operación bloqueante > 50ms (Connect BLE, WiFi Init, File Write) DEBE delegarse a una **Task FreeRTOS** separada (Pinned to Core 0).
    *   El Watchdog (8s) no se "parchea" alimentándolo más rápido; se evita no bloqueando el hilo principal.
3.  **Zona de Peligro (Hardware Risks)**:
    *   **NO TOCAR**: Pines de arranque (Bootstrap) como GPIO0 (Boot) o GPIO46 (Log).
    *   **Flash API**: Prohibida la escritura directa en particiones de sistema o NVS sin usar las APIs de abstracción (`Preferences` o `LittleFS`).
    *   **PSRAM**: No asumir que existe. Usar `psramFound()` antes de `ps_malloc`.

4.  **Backend - Frontend Web UI**:
    *   **Desacoplamiento Estricto**: Las pantallas TFT (`IScreen`) NO conocen el servidor web. Leen estados desde un Contexto Compartido en la capa de Servicios o Core.
    *   **Mutaciones Asíncronas**: Los parámetros desde la UI Web se mandan vía API REST en formato JSON (ej. `fetch('/api/config', {method: 'POST'})`). Nunca forzar recargas completas (SPA de facto).
    *   **Resiliencia Cliente**: Todo el JavaScript servido al cliente debe manejar reconexiones o tokens expirados (401) guiando suavemente al login de nuevo.

5.  **Transferencia HTTP (TCP Buffer Constraints)**:
    *   **Límite Duro**: La ventana TCP del ESP32-S3 es de **~5744 bytes**. Cualquier respuesta HTTP cuyo cuerpo supere este tamaño será truncada silenciosamente por `WiFiClient::write()` (flag `MSG_DONTWAIT`).
    *   **Regla de Oro**: Todo recurso servido vía `send()` o `send_P()` debe ser **< 5 KB** de payload. Si es mayor, usar el **patrón WiFiClient con Backpressure** (ver README §6.12).
    *   **Chunked Transfer Encoding PROHIBIDO**: `sendContent()` realiza 3 writes internos por chunk (size line + data + CRLF). Si cualquiera falla parcialmente, el stream chunked entero se corrompe → `ERR_INVALID_CHUNKED_ENCODING`. No usar nunca `CONTENT_LENGTH_UNKNOWN` con `sendContent()`.
    *   **Concurrencia INEXISTENTE**: El `WebServer` nativo es **single-threaded**. Solo puede procesar una petición TCP a la vez. Peticiones concurrentes → `ERR_CONNECTION_RESET`. El JS del cliente DEBE encadenar `fetch()` secuencialmente con `.then()`.
    *   **Patrón Split-Payload**: Para dashboards con HTML+JS+CSS, dividir en archivos PROGMEM independientes (ej. `DASH_HTML` + `DASH_JS`), cada uno bajo 5KB, cargados secuencialmente por el navegador (`<script src='/d.js'>`).
    *   **Patrón WiFiClient Backpressure**: Para payloads > 5KB (ej. JSON de paletas de 27KB), enviar cabeceras con `Content-Length` exacto vía `_server.send(200, type, "")`, luego iterar con `client.write()` en chunks de 256 bytes con reintentos (`delay(5)` entre fallos, máximo 50 reintentos por chunk). Llamar `yield()` entre chunks para no disparar el WDT.

---

## 5. MEMORIA TÉCNICA
**Principio**: El error es aceptable; no documentarlo es negligencia.

1.  **Registro de Batalla**:
    *   Cuando una implementación falla (e.g., "El BLE bloquea el WDT"), debe documentarse en el README como una "Lección Aprendida".
    *   Esto evita bucles infinitos donde futuros agentes intentan soluciones que ya sabemos que fallan.
2.  **Justificación de Hacks**:
    *   Si es necesario aplicar una solución poco ortodoxa, debe ir acompañada de un comentario en código `// NOTE:` y una entrada en el README explicando *por qué* fue la única opción viable.

### 5.3 Anti-Patterns Conocidos (NO HACER)
Los siguientes patrones han demostrado ser problemáticos en este proyecto:
*   **"Optimización temprana" del render parcial**:
    *   → Introduce ghosting y complejidad sin beneficio real. Preferir "Redraw Completo".
*   **`BLEClient::connect()` en el hilo principal**:
    *   → Provoca WDT resets intermitentes. Usar Tasks.
*   **Crear Sprites en el constructor**:
    *   → Fragmenta heap y rompe el ciclo de vida `onEnter`/`onExit`.
*   **Re-inicializar el stack BLE en runtime**:
    *   → Provoca panics difíciles de reproducir. Preferir mantener el stack vivo si hay memoria.
*   **Matar tareas BLE con `vTaskDelete` mientras operan**:
    *   → Causa `LoadProhibited` Panics. Usar `disconnect()` + Self-Terminate.
*   **`_server.send()` con payloads > 5KB**:
    *   → `WiFiClient::write()` usa `MSG_DONTWAIT` y descarta bytes silenciosamente cuando el buffer TCP está lleno (~5744 bytes). Resultado: `ERR_CONTENT_LENGTH_MISMATCH` en el navegador. Usar **Split-Payload Pattern** o **WiFiClient Backpressure**.
*   **Chunked Transfer Encoding (`sendContent()`) en ESP32 WebServer**:
    *   → `sendContent()` hace 3 writes internos por chunk. Fallos parciales corrompen la trama chunked → `ERR_INVALID_CHUNKED_ENCODING`. Usar `Content-Length` exacto + stream manual.
*   **`fetch()` paralelos contra ESP32 WebServer**:
    *   → El WebServer es single-threaded. Solo procesa 1 conexión TCP a la vez. Peticiones concurrentes obtienen `ERR_CONNECTION_RESET`. Encadenar con `.then()` en JS.
*   **Escribir headers HTTP crudos directamente al `WiFiClient`**:
    *   → `client.print("HTTP/1.1 200 OK\r\n...")` conflicta con el estado interno del `WebServer`, que ya ha procesado la request line y headers del cliente. Resultado: `errno 104, Connection reset by peer`. Usar siempre `_server.send(code, type, "")` para headers y luego `client.write()` solo para el body.

Antes de proponer una solución, **comprobar que no encaja en esta lista**.

---

## 6. DEFINICIÓN DE "DONE" (Criterios de Cierre)
Una tarea o iteración solo se considera cerrada si cumple **TODOS** los puntos:

- [ ] **Compila** limpiamente (`exit code: 0`).
- [ ] **Estabilidad**: La feature soporta ciclos de vida completos (entrar / salir / repetir) sin leaks.
- [ ] **Observabilidad**: Los logs en tiempo de ejecución permiten entender qué está pasando sin adivinar.
- [ ] **Limpieza**: No quedan tareas FreeRTOS "zombies" ni punteros colgantes (`onExit` verificado).
- [ ] **Reinicios**: No provoca WDT Resets ni Core Panics.
- [ ] **Reflejo Documental**: El `README.md` describe con precisión el estado actual del sistema.

> **En resumen:** Si el código funciona pero el README miente, la tarea está incompleta.

---

## 7. DOCUMENTACIÓN EN CÓDIGO (LLM-FRIENDLY)
**Objetivo**: Convertir el código fuente en un contexto rico para futuros agentes de IA (Humanos o LLMs).

### 7.1 Encabezados de Clase (El "Por Qué")
Cada clase mayor debe comenzar con un bloque que explique no solo qué hace, sino las decisiones de diseño tomadas.
```cpp
/**
 * @class ScreenGamepad
 * @brief Monitor de Input BLE para depuración.
 * 
 * DESIGN DECISIONS:
 * - Uses Async Task (Core 0) to prevent Main Loop WDT starvation during connection.
 * - ZERO-COPY: Uses static buffers to avoid Heap Fragmentation.
 * - ERROR-PROOFING: Graceful degradation if HID service 0x1812 is missing (Discovery Mode).
 */
class ScreenGamepad : ...
```

### 7.2 Contratos de Función
Define explícitamente qué espera la función y qué garantiza.
```cpp
// @brief Conecta al dispositivo remoto.
// @blocking true (~3000ms) - MUST BE CALLED FROM WORKER TASK
// @return false if timeout or target not found. NO EXCEPTIONS.
bool connectFunc() ...
```

### 7.3 Comentarios Narrativos (Thinking Steps)
Usa comentarios para guiar la "cadena de pensamiento" del lector sobre bloques complejos.
*   `// STEP 1: Validate Pre-conditions`
*   `// STEP 2: Critical Section (Disable Interrupts)`
*   `// NOTE: We deliberately skip verification here to save 20ms`

Estos comentarios permiten a una IA entender si un comportamiento "extraño" es un bug o una optimización intencional.

### 7.4 Contexto Mínimo Viable (Code Snippets)
Al solicitar u ofrecer cambios parciales en código, nunca proporcionar funciones "huérfanas".
*   Incluir siempre los **includes relevantes**.
*   Incluir siempre el **scope de la clase** (`class XYZ { ... }`).
*   Esto evita alucinaciones de la IA sobre variables miembro inexistentes.

---

## 8. GESTIÓN DE DEPENDENCIAS Y ENTORNO
**Principio**: La reproducibilidad es más importante que estar "a la última".

1.  **Bibliotecas Congeladas (Version Pinning)**:
    *   En `platformio.ini`, es **OBLIGATORIO** especificar la versión exacta de las librerías `lib_deps`.
    *   ✅ BIEN: `bodmer/TFT_eSPI @ 2.5.43`
    *   ❌ MAL: `bodmer/TFT_eSPI` (asume `latest`) o `bodmer/TFT_eSPI @ ^2.5.0`.
    *   Esto previene que una actualización silenciosa rompa el código meses después.
2.  **Entorno Python**:
    *   Mantener el entorno de scripts (`upload.ps1`, `build.ps1`) independiente de la versión global de Python del sistema.

---

## 9. MODOS DE TRABAJO (Meta-Instrucción)
Este proyecto distingue tres modos conceptuales de operación:

1.  **MODO DISEÑO**:
    *   Se discuten enfoques.
    *   Se evalúan riesgos (RAM, WDT, Stack Size).
    *   Se decide arquitectura.
    *   **Documentación Preventiva (README)**:
        *   **Autonomía**: Definir cómo la feature gestionará sus recursos sin bloquear hilos compartidos.
        *   **Aislamiento**: Garantizar por diseño que desactivar esta feature no deja "huérfanos" que rompan otras partes del sistema.
    *   **NO** se escribe código definitivo.
2.  **MODO IMPLEMENTACIÓN**:
    *   Se escribe código alineado estrictamente con revisiones previas.
    *   Se compila e itera.
    *   Se documenta.
3.  **MODO CIERRE DE ITERACIÓN**:
    *   Validación final y limpieza.
    *   **Documentación de Reversibilidad (README)**: Es OBLIGATORIO describir cómo **desactivar** la funcionalidad (e.g., qué línea comentar en `main.cpp`) para evitar contaminación cruzada en el futuro.

**Regla de Oro**: Ninguna feature que toque **Concurrencia (Tasks), BLE o Gestión de Memoria** puede pasar a Implementación sin haber sido validada en Modo Diseño.



## 10. PATRONES WEB Y CONCURRENCIA
Basado en la integración del Web Dashboard (Shell + AJAX) y The Game of Life:

### 10.1 Arquitectura Web: Shell + AJAX (NO monolítica)
*   **Prohibido generar HTML dinámico grande**: El ESP32 WebServer nativo es single-threaded y tiene un buffer TCP de ~5744 bytes. Cualquier respuesta mayor se corrompe silenciosamente.
*   **Patrón obligatorio**: Servir un HTML shell estático pequeño (< 5KB PROGMEM) que carga datos via `fetch()` a endpoints REST JSON.
*   **Ventaja**: Evita parpadeos, ahorra memoria y ancho de banda, y separa la presentación HTML del estado del backend C++.
*   **Renderizado client-side**: Toda lógica de presentación (ej. previews visuales, cálculos de porcentajes, formateo) debe hacerse en JavaScript con los datos ya cargados. **No crear endpoints adicionales** si la información ya está disponible en el cliente. Ejemplo: el preview de colores de paletas se renderiza a partir del JSON de `/api/palettes` sin peticiones extra.

### 10.2 Transferencia HTTP en ESP32 WebServer
*   **Split-Payload Pattern**: Si el contenido estático total supera 5KB, dividir en múltiples recursos (ej. `/dashboard` → HTML + `/d.js` → JavaScript), cada uno < 5KB.
*   **WiFiClient Backpressure Pattern**: Para payloads > 5KB (ej. JSON de paletas de 27KB), enviar headers con `_server.send(200, type, "")` y luego hacer stream del body con `client.write()` en chunks de 256 bytes con retry loop + `yield()`.
*   **Prohibido**: Chunked Transfer Encoding (`sendContent()`), `send()`/`send_P()` con payloads > 5KB, headers HTTP manuales vía `client.print()`.

### 10.3 Concurrencia JS → ESP32
*   **Prohibido `fetch()` paralelos**: El WebServer solo procesa 1 conexión TCP a la vez. Múltiples peticiones simultáneas causan `ERR_CONNECTION_RESET`.
*   **Patrón obligatorio**: Encadenar con `.then()` → `lS().then(function(){return lG()}).then(function(){return lP()})`.

### 10.4 Compartición de Estado Seguro (Thread-Safety)
*   La comunicación entre el WebServer y el hilo principal del TFT (`loop()`) DEBE realizarse a través de **Singletons de Estado**.
*   Es **OBLIGATORIO** usar `portMUX_TYPE` y bloqueos críticos (`portENTER_CRITICAL(&_mux)`) al leer o mutar variables compartidas para evitar corrupción de memoria (Race Conditions).

### 10.5 Optimización Extrema de RAM en Pantallas (Graceful Degradation)
*   Las matrices grandes (como la del autómata celular) no deben usar arrays estáticos de bytes (`uint8_t grid[320*240]`). Para mitigar el temido `Alloc 16bpp Failed`, se DEBEN usar **estructuras Bitwise** (1 celda = 1 bit de RAM), reduciendo el consumo de memoria dinámica en ~88%.

---

## 11. SCRIPTS AUXILIARES (`aux_/`)

La carpeta `aux_/` contiene scripts de desarrollo creados durante sesiones de debugging y refactoring (sufijo `_` por compatibilidad con OneDrive, que reserva el nombre `aux`). **No participan en la compilación** y pueden borrarse sin impactar al proyecto. Se conservan como herramientas reutilizables y como registro histórico de las decisiones tomadas.

### 11.1 Scripts Python — Parcheo y Refactoring de WebService.cpp

| Script | Propósito | Uso |
|--------|-----------|-----|
| `fix_all.py` | Parche combinado: reemplaza `sendContent()` chunked por `send()` directo en dashboard, y reescribe `handlePalettesGet()` con streaming chunked de 1KB desde PROGMEM. | `python aux_/fix_all.py` |
| `fix_dashboard.py` | Parche quirúrgico: convierte `send(200, "text/html", buildDashboardPage())` a chunked con `sendContent()` (512 bytes). *Enfoque descartado* — ver anti-patterns en §5.3. | `python aux_/fix_dashboard.py` |
| `fix_js.py` | Reemplaza el bloque `<script>...</script>` completo dentro del HTML del dashboard por una versión con funciones de paletas (`initPal`, `updPalList`, `setPal`). | `python aux_/fix_js.py` |
| `patch.py` | Inyecta la pestaña "Palettes" en el dashboard monolítico: tab HTML, selectores `<select>` y funciones JS inline. | `python aux_/patch.py` |
| `refactor_dashboard.py` | **Refactoring mayor**: reemplaza el dashboard monolítico por la arquitectura Shell + AJAX. Elimina `buildDashboardPage()`, inyecta `DASHBOARD_HTML` PROGMEM, añade `handleSystemInfo()` y `handleGOLStateGet()`. | `python aux_/refactor_dashboard.py` |

### 11.2 Scripts Python — Diagnóstico y Testing

| Script | Propósito | Uso |
|--------|-----------|-----|
| `read_test.py` | Lee `WebService.cpp` y busca la función `buildDashboardPage` para imprimir un snippet de contexto alrededor. Útil para verificar si un parche se aplicó correctamente. | `python aux_/read_test.py` |
| `test_com.py` | Abre el puerto serie COM4 (115200 baud) con reset RTS y muestra la salida del ESP32 en tiempo real. Alternativa rápida al monitor de PlatformIO. | `python aux_/test_com.py` (requiere `pyserial`) |

### 11.3 Scripts PowerShell — Parches de WebService

| Script | Propósito | Uso |
|--------|-----------|-----|
| `patch_webservice.ps1` | Inyecta las rutas `/api/palettes` y `/api/palettes/set` antes de `/favicon.ico` en `setup()`, y añade el `#include` de `PalettesData.h`. | `.\aux_\patch_webservice.ps1` |
| `patch_webservice2.ps1` | Limpieza: corrige artefactos de doble backtick-n introducidos por el escape de PowerShell en el parche anterior. | `.\aux_\patch_webservice2.ps1` |

### 11.4 Scripts de Pipeline de Paletas (`include/colores/`)
Estos scripts **NO están en `aux_/`** porque forman parte del pipeline de generación de datos del proyecto:

| Script | Propósito | Uso |
|--------|-----------|-----|
| `extract_colors.py` | Extrae paletas de colores (16 colores) de imágenes `.png`/`.jpg` usando Pillow. Genera JSON por carpeta de artista. | `python include/colores/extract_colors.py` |
| `aggregate_palettes.py` | Combina todos los JSON individuales en `master_palettes.json`. | `python include/colores/aggregate_palettes.py` |
| `export_cpp_palettes.py` | Convierte `master_palettes.json` → `src/core/PalettesData.h` (PROGMEM `PALETTES_JSON[]`). | `python include/colores/export_cpp_palettes.py` |
| `simulador_paleta.py` | Visualizador tkinter: simula cómo se verá una paleta en la pantalla TFT del ESP32 (320×240 grid). | `python include/colores/simulador_paleta.py [carpeta] [paleta]` |

> **Nota**: El pipeline completo para regenerar paletas es:
> `extract_colors.py` → `aggregate_palettes.py` → `export_cpp_palettes.py` → compilar.
