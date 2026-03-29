# PROMPT: Añadir Nuevo Módulo IO (Arquitectura Virtuoso)

Usa este prompt cuando necesites añadir un nuevo sensor, actuador o protocolo de comunicación al framework ESP32-S3 Tech Test.

# PROTOCOLO OBLIGATORIO DE EJECUCIÓN

Antes de cualquier acción, el Asistente debe ejecutar las herramientas de lectura de archivo para leer y confirmar comprensión de:
1.  **`README.md`** (Arquitectura actual, especificación técnica y "The Virtuoso Standard").
2.  **`src/core/Config.h`** (Mapa de hardware y constantes globales).
3.  **`platformio.ini`** (Entorno de compilación y librerías).

El proceso de desarrollo se divide ESTRICTAMENTE en dos fases bloqueantes. La transición de fases requiere la palabra clave explícita del usuario.

---

## FASE 1: DISEÑO (Keyword: "DISEÑO")
**Objetivo**: Validar el enfoque técnico antes de escribir una sola línea de código, asegurando el cumplimiento de la arquitectura en capas (Layered Architecture).

**Instrucciones para el Asistente en Fase 1**:
1.  **Analizar Requisitos**:
    - ¿Qué interfaz usa (I2C, SPI, UART, GPIO, ADC)?
    - ¿Encaja en la capa **Core** (Lógica pura) o **HAL** (Hardware Abstraction Layer)?
    - ¿Necesita una pantalla visual (UI) asociada?
    - ¿Conflictos de pines en el ESP32-S3 (Strapping pins, USB JTAG)?
2.  **Solución Técnica Propuesta**:
    - Detallar cambios en `src/core/Config.h`.
    - Definir estructura de archivos según arquitectura:
        - `src/hal/` para el driver de bajo nivel.
        - `src/core/` para gestores de lógica.
        - `src/screens/` si requiere interfaz gráfica (heredando de `IScreen` y usando `BaseSprite`).
    - Explicar integración en `main.cpp` (Setup/Loop) y `ScreenManager` (si aplica).
3.  **Salida Requerida**: Un plan de implementación detallado en Markdown.

**STOP**: El Asistente no debe generar código final ni editar ficheros en esta fase. Debe preguntar al usuario: *"¿Apruebas este diseño? Responde 'IMPLEMENTACION' para proceder."*

---

## FASE 2: IMPLEMENTACIÓN (Keyword: "IMPLEMENTACION")
**Objetivo**: Escribir, compilar y validar el código aprobado bajo estándares "Virtuoso".

**Instrucciones para el Asistente en Fase 2**:
1.  **Implementación Incremental**:
    - Añadir dependencias en `platformio.ini` (si aplica).
    - Crear clases siguiendo principios SOLID e Inyección de Dependencias (Singletons para HAL).
    - Si hay UI: Implementar `BaseSprite` obligatoriamente para gestión de memoria segura en PSRAM.
2.  **Instrumentación**: Uso estricto de `Logger::log(TAG, ...)` para trazas.
3.  **Compilación e Iteración OBLIGATORIA w/ `build.ps1`**:
    - EJECUTAR `.\build.ps1` tras los cambios.
    - Si falla, LEER los errores, CORREGIR y RE-EJECUTAR `.\build.ps1`.
    - Repetir hasta obtener `[SUCCESS]`.
4.  **Documentación**:
    - Actualizar `README.md` con la nueva funcionalidad, pines usados y lógica operativa.

---

## Contexto de Hardware y Restricciones (ESP32-S3 DevKitC-1)
- **Arquitectura**: Layered, OOP, UI-Centric (TFT + Buttons).
- **Microcontrolador**: ESP32-S3 (Xtensa LX7).
- **Pines Prohibidos/Ocupados**:
    - **TFT Display**: MOSI:11, SCLK:12, MISO:13, CS:48, DC:47, RST:21.
    - **LED RGB (NeoPixel)**: 38 (Usar `LedHAL`).
    - **Botón BOOT**: 0 (Entrada principal).
    - **USB JTAG/Serial**: 19, 20 (Evitar si se usa debug USB).
    - **PSRAM**: Pines 35, 36, 37 (Octal SPI) - **NO TOCAR**.

## Ejemplo de Flujo de Conversación
**Usuario**: "DISEÑO: Necesito leer la temperatura interna del ESP32"
**Asistente**: [Lee docs] -> [Propone plan: Usar `temperatureRead()` en `ScreenStatus`, no requiere HAL extra] -> "Esperando 'IMPLEMENTACION'..."
**Usuario**: "IMPLEMENTACION"
**Asistente**: [Edita ScreenStatus.h] -> [Ejecuta `.\build.ps1`] -> "Compilación Exitosa. Display actualizado."
