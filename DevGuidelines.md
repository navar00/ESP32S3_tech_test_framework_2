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
    *   → Provoca panics difíciles de reproducir. Preferir mantener el stack vivo si hay memoria.*   **Matar tareas BLE con `vTaskDelete` mientras operan**:
    *   → Causa `LoadProhibited` Panics. Usar `disconnect()` + Self-Terminate.
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



## 10. PATRONES WEB Y CONCURRENCIA (NUEVO)
Basado en la integración del Web Dashboard (SPA) y The Game of Life:

1. **Web UI como SPA (Single Page Application)**:
   * **Prohibido el refresco de página**: Las acciones desde la web hacia el dispositivo (ej. cambiar colores, botones) deben hacerse obligatoriamente mediante peticiones asíncronas (etch()) a endpoints REST (ej. /api/gol/action).
   * **Ventaja**: Evita parpadeos en el teléfono del usuario, ahorra memoria y ancho de banda en el ESP32, y separa la lógica de presentación HTML del estado del backend.

2. **Compartición de Estado Seguro (Thread-Safety)**:
   * La comunicación entre el servidor Web asíncrono (AsyncWebServer) y el hilo principal del TFT (loop()) DEBE realizarse a través de **Singletons de Estado**.
   * Es **OBLIGATORIO** usar portMUX_TYPE y bloqueos críticos (portENTER_CRITICAL(&_mux)) al leer o transmutar variables compartidas para evitar corrupción de memoria (Race Conditions).

3. **Optimización Extrema de RAM en Pantallas (Graceful Degradation)**:
   * Las matrices grandes (como la del autómata celular) no deben usar arrays estáticos de bytes (uint8_t grid[320*240]). Para mitigar el temido Alloc 16bpp Failed, se DEBEN usar **estructuras Bitwise** (1 celda = 1 bit de RAM), reduciendo el peso de la memoria dinámica de la pantalla en casi un 88%.


## 10. PATRONES WEB Y CONCURRENCIA
Basado en la integracion del Web Dashboard (SPA) y The Game of Life:

1. **Web UI como SPA (Single Page Application)**:
   * **Prohibido el refresco de pagina**: Las acciones desde la web hacia el dispositivo (ej. cambiar colores, botones) deben hacerse obligatoriamente mediante peticiones asincronas (etch()) a endpoints REST (ej. /api/gol/action).
   * **Ventaja**: Evita parpadeos en el telefono del usuario, ahorra ancho de banda en el ESP32, y separa la logica de presentacion HTML del estado real del backend en C++.

2. **Comparticion de Estado Seguro (Thread-Safety)**:
   * La comunicacion cruzada entre el AsyncWebServer y el hilo principal del TFT (loop()) DEBE realizarse a traves de **Singletons Compartidos**.
   * Es **OBLIGATORIO** usar portENTER_CRITICAL al leer/escribir variables compartidas para evitar corrupcion de memoria.

3. **Optimizacion Extrema de RAM (Graceful Degradation)**:
   * Las matrices gigantes en pantalla NO DEBEN usar variables enteras o chars, sino arreglos de bits (1 uint8_t = 8 celdas de pantalla) para no aplastar el heap del PSRAM.
