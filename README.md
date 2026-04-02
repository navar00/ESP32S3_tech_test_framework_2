# Tech Test: ESP32-S3 Engineering Sandbox
**"Virtuoso Edition"**

Este repositorio es un **Laboratorio de Ingeniería (Sandbox)** diseñado para el desarrollo y validación de componentes de software de alta abstracción para el ESP32-S3. Este documento sirve tanto como manual de arquitectura como de especificación técnica de los estándares implementados.

---

## 1. Arquitectura & Visión
El sistema abandona el tradicional "Spaghetti Code" de Arduino en favor de una arquitectura orientada a objetos, modular y testeable.

### 1.1 Estructura en Capas (Layered Architecture)
1.  **Capa Drivers (HAL)**: (`/src/hal`)
    *   `DisplayHAL`: Singleton que encapsula la librería `TFT_eSPI`. Abstrae la inicialización del bus SPI y la gestión de la retroiluminación.
    *   `LedHAL`: Control abstracto del LED RGB (NeoPixel).
    *   `InputHAL`: Wrapper para **Bluepad32**. Gestiona la pila Bluetooth y los eventos de Gamepads. Implementa stubs críticos para evitar crashes por callbacks nulos.
2.  **Capa Core**: (`/src/core`)
    *   `ScreenManager`: Orquestador del ciclo de vida de la aplicación.
    *   `BootOrchestrator`: Secuencia de arranque con UI de progreso (WiFi → GeoIP → NTP).
    *   `Logger`: Sistema de trazas centralizado.
3.  **Capa Servicios**: (`/src/services`)
    *   `TimeService`: Singleton NTP con sincronización automática (`pool.ntp.org`, `time.nist.gov`). Expone `getTimeParts()`, `getDateParts()`, countdown hasta resync.
    *   `NetService`: Gestión WiFi con reconexión automática.
    *   `GeoService`: Geolocalización por IP para offset de zona horaria.
    *   `WebService`: Servidor HTTP (puerto 80) con PIN aleatorio de 4 dígitos, sesión por cookie. Arquitectura **Shell + AJAX**: sirve un HTML estático mínimo desde PROGMEM (`<3.4KB`) y carga datos dinámicos secuencialmente vía API REST (`/api/...`). Dashboard multi-pestaña (System Info, Game of Life Control, Palette Selector con preview visual de colores ranqueados por frecuencia).
4.  **Capa Presentación**: (`/src/screens`)
    *   Implementaciones concretas de la interfaz `IScreen` (`Strategy Pattern`).
    *   `ScreenStatus`: Monitor de Sistema — Info del sistema, memoria (tabla 2 columnas) y red (SSID+RSSI/IP/MAC/Web URL+PIN) - *[Activo]*.
    *   `ScreenGameOfLife`: Autómata celular (Conway's Game of Life). Control remoto asíncrono desde el Web Dashboard (colores, velocidad, reset). Gestión avanzada de memoria con doble grid de bits (Graceful Degradation) - *[Activo]*.
    *   `ScreenAnalogClock`: Reloj analógico (mitad izq.) + Calendario mensual con caja roja en día actual (mitad der.) - *[Activo]*.
    *   `ScreenFlipClock`: Reloj estilo "flip clock" con dígitos 7-segmentos custom via `fillRect`. HH:MM en paneles grandes, `:SS` y countdown NTP en footer - *[Activo]*.
    *   `BootConsoleScreen`: Pantalla de arranque con barra vertical de progreso (izq.) y log incremental (der.) - *[Boot]*.
    *   `ScreenBLEScan`: Radar BLE con gestión de energía (WiFi Toggle) - *[Latente]*.
    *   `ScreenGamepad`: Monitor HID BLE (Modo Explorador de Servicios) - *[Activo]*.

### 1.2 Estructura del Proyecto
```text
.
├── platformio.ini        # Configuración de Entorno & Hardware (Build Flags)
├── build.ps1             # Script de compilación (salida limpia + log detallado)
├── upload.ps1            # Script de upload + monitor serie (sin recompilación)
├── src/
│   ├── main.cpp          # Entry Point: Setup & Loop (Minimalista)
│   ├── core/             # Lógica de Negocio Pura
│   │   ├── Config.h      # Constantes Hardware/Software (constexpr)
│   │   ├── Logger.h      # Wrapper Serial (Variadic Args)
│   │   ├── BootOrchestrator # Secuencia de arranque con UI
│   │   └── ScreenManager # State Machine & Event Dispatcher
│   ├── hal/              # Abstracción de Hardware
│   │   ├── Hal.h         # Facade para inclusions
│   │   └── ...           # Implementaciones (Display, Led, Input)
│   ├── services/         # Servicios de Red y Tiempo
│   │   ├── TimeService   # NTP sync + getTimeParts/getDateParts
│   │   ├── NetService    # WiFi manager
│   │   ├── GeoService    # Geo IP → timezone offset
│   │   └── WebService    # HTTP server + PIN auth + system dashboard
│   └── screens/          # Vistas (UI)
│       ├── IScreen.h     # Interfaz base
│       ├── BaseSprite.h  # Helper DRY para sprites (Graceful Degradation)
│       ├── BootConsoleScreen # UI de Boot (barra vertical + log)
│       ├── ScreenStatus  # Monitor de Sistema (3 secciones)
│       ├── ScreenAnalogClock # Reloj Analógico + Calendario Mensual
│       ├── ScreenFlipClock   # Flip Clock 7-segmentos (HH:MM)
│       ├── ScreenBLEScan # Escáner Simplificado (Latente)
│       └── ScreenGamepad # Cliente BLE HID (Activo)
```

---

## 2. Definición de Hardware & Configuración (`platformio.ini`)
La "magia" de la portabilidad reside en cómo se inyectan las configuraciones en tiempo de compilación.

### 2.1 Mapa de Hardware (Pin Definition)
Se definen dos fuentes de verdad: `platformio.ini` para el Display (inyectado al driver) y `Config.h` para periféricos generales.

| Periférico | Pin (GPIO) | Definición | Notas |
| :--- | :--- | :--- | :--- |
| **TFT SPI** | MOSI:11, SCLK:12, MISO:13 | `platformio.ini` | Bus SPI dedicado |
| **TFT Control** | CS:48, DC:47, RST:21 | `platformio.ini` | Control ILI9341 |
| **LED RGB** | GPIO 38 | `Config.h` | NeoPixel / WS2812 |
| **Botón BOOT** | GPIO 0 | `Config.h` | Active LOW (Pull-up interno) |

### 2.2 Entorno de Compilación
*   **Board**: `esp32-s3-devkitc-1` (Lolin S3 Pro Map).
*   **Velocidad**: Upload a `921600` baud.
*   **Framework**: `arduino`.

### 2.3 Librerías (`lib_deps`)
1.  **`bodmer/TFT_eSPI`**: Motor gráfico de alto rendimiento (DMA/Sprites).
2.  **`ESP32 BLE Arduino`**: Stack Bluetooth estándar. *Nota de Diseño*: Se prefiere sobre `NimBLE` por mayor estabilidad en escaneos síncronos cortos.

---

## 3. Estándares de Desarrollo (Design Guidelines)
Para mantener la coherencia y calidad del código ("The Virtuoso Way"):

### 3.1 Estilo de Código (C++)
*   **Encapsulamiento**: Prohibido el uso de variables globales. Todo estado debe residir en clases gestoras o singletons.
*   **Gestión de Memoria**: Uso estricto de `new/delete` para recursos pesados (Sprites) en el ciclo de vida de las pantallas (`onEnter`/`onExit`) para mantener la RAM libre.
*   **Logging**: Uso obligatorio de `Logger::log(TAG, format, ...)` para trazas unificadas con timestamp.

### 3.2 UI/UX Standard: "Retro Terminal"
Todas las interfaces deben adherirse estrictamente a la estética de fósforo verde monocromática.

**Especificaciones Visuales:**
*   **Paleta**:
    *   Fondo: `TFT_BLACK`
    *   Texto/Datos: `TFT_GREEN`
    *   Cabeceras: Inverso (Fondo `TFT_GREEN` + Texto `TFT_BLACK`).
    *   Líneas: `TFT_DARKGREEN` (Separadores).
*   **Tipografía**:
    *   **Font 2** (`setTextFont(2)`): Sans-serif pixelada ~16px. Usada para el 90% de la UI.
    *   **Leading**: **18px** fijos.
    *   **Márgenes**: Izquierdo 4px.

**Observabilidad (Diagnóstico):**
El sistema expone información crítica para depuración y emparejamiento:
*   **MAC WiFi**: Identificador de red y diagnóstico de hardware.
*   **MAC Bluetooth**: Dirección necesaria para emparejamiento con mandos (SixaxisPairTool).
    *   Visible en logs de arranque: `[BP32]` y `[STATUS]`.
    *   Visible en UI: Pantalla `STATUS_SYS` (Filas inferiores).

**Grid System (Columnas Estándar en 320px):**
Para listas densas (e.g., BLE), respetar las siguientes posiciones X:
*   **Columna 1 (ID/MAC)**: x = 4
*   **Columna 2 (Métrica/RSSI)**: x = 90
*   **Columna 3 (Tipo/Nature)**: x = 140
*   **Columna 4 (Detalle/Nombre)**: x = 190

### 3.3 Motor de Renderizado (Flicker-Free)
1.  **Sprites Mandatory**: Prohibido dibujar en pantalla directa. Se debe instanciar un `TFT_eSprite` del tamaño del viewport.
2.  **Limpieza Selectiva**: Usar `fillRect(..., TFT_BLACK)` antes de escribir texto variable.
3.  **Atomic Push**: `sprite.pushSprite(0,0)` una sola vez por frame.

#### 3.3.1 Componente BaseSprite (Refactorización)
Para cumplir con el principio **DRY (Don't Repeat Yourself)**, se implementa el helper `BaseSprite` (`src/screens/BaseSprite.h`). Este componente encapsula:
*   Ciclo de vida del sprite (`init/free`) para gestión segura de RAM.
*   **Estrategia de Asignación Dinámica**: Intenta reservar buffer de 16-bits (High Color). Si la RAM es insuficiente, degrada automáticamente a 8-bits (256 colores) o 4-bits (16 colores) para evitar crashes (`Graceful Degradation`).
*   Primitivas de dibujo estandarizadas ("Retro Look").

### 3.4 Estándar de Identificación BLE
El sistema debe clasificar activamente los dispositivos detectados:
*   **HID**: Service UUID `0x1812` o Appearance `960+`.
*   **Audio**: Service UUID `0x180E`/`0x110B`.
*   **Log Format**: `[BLE] Found: <MAC> | RSSI: <RSSI> | Nature: <TYPE> | Name: <NAME>`
*   **Gestión de Recursos BLE**: Se desaconseja el uso de `BLEDevice::deinit(true)` en tiempo de ejecución. Aunque libera RAM, la re-inicialización (`esp_bt_controller_enable`) es inestable en este core, provocando Panics (`esp_bt_controller_mem_release`).
    *   *Solución*: Mantener el stack inicializado y confiar en la **Degradación Gráfica** de `BaseSprite` para encajar en la RAM restante.

### 3.5 Estándar de Feedback LED (Traffic Light)
El LED RGB integrado actúa como un indicador de estado del sistema (System Semaphore).
*   **Arquitectura**: Gestionado por `LedHAL` (Singleton).
*   **Brillo Global**: Limitado por hardware al **5%** (aprox. valor 12/255) para evitar deslumbramiento.
*   **Códigos de Color**:
    1.  **BOOT**: Parpadeo Amarillo (`255, 255, 0`) durante la inicialización.
    2.  **STATUS**: Naranja Fijo (`255, 165, 0`).
    3.  **BLE**: Verde Fijo (`0, 255, 0`).
    4.  **ERROR/PANIC**: Rojo Parpadeante (`255, 0, 0`).

### 3.6 Sistema de Seguridad y Watchdog (Overarching WDT)
Para garantizar la fiabilidad en despliegues desatendidos ("Zero Maintenance"), el sistema implementa una vigilancia activa del bucle principal.

*   **Task Watchdog Timer (TWDT)**: Configurado a **8 segundos**. Si el bucle principal (`loop()`) se bloquea por más de este tiempo (e.g., driver SPI colgado, bucle infinito), el hardware reiniciará la CPU.
    *   *Nota*: El escaneo BLE dura 5s, lo cual entra dentro del margen de seguridad.
*   **Gestión de Reset (Crash Recovery)**:
    *   Al arrancar, el sistema consulta `esp_reset_reason()`.
    *   Si la causa fue un **WDT Reset** o **Panic**, el LED parpadeará en **Rojo** durante 2 segundos antes de iniciar, alertando visualmente del fallo anterior.
    *   Se registrará la causa del reinicio en el puerto serie.

---

## 4. Arquitectura de Ejecución (Runtime Flow)

### 4.1 Secuencia de Arranque (`setup`)
1.  **HAL Init**: Configuración de SPI y limpieza de VRAM.
2.  **Manager Hydration**: Se inyectan las instancias `ScreenStatus` y `ScreenBLE` en el `ScreenManager`.
3.  **Timer Launch**: Se inicia el Timer 0 a 1Hz.
4.  **Loop Handover**: Se cede el control al bucle principal.

### 4.2 Motor de Interrupciones ("The Heartbeat")
El sistema utiliza un **Timer de Hardware** para garantizar la precisión temporal sin bloquear la UI.
*   **ISR (Interrupt Service Routine)**: Se ejecuta a 1Hz. *Solo* levanta una bandera `volatile bool timerFlag`.
*   **Dispatch**: El `loop()` comprueba la bandera y llama a `ScreenManager::dispatchTimer()`.
*   **Seguridad**: Esto evita problemas de concurrencia al acceder al bus SPI desde una interrupción.

### 4.3 Gestión de Navegación (Carrusel)
El `ScreenManager` implementa una máquina de estados simple:
*   **Cambio de Pantalla**: Destruye la pantalla anterior (`onExit`) -> Libera RAM -> Inicializa la nueva (`onEnter`).
*   **Input**: El botón BOOT (GPIO0) dispara la transición con debounce por software.

### 4.4 Lógica de Pantallas Específicas
*   **ScreenStatus**: Modo **Reactivo**. Se actualiza solo cuando el Timer dispara el evento.
*   **ScreenBLE**: Modo **Bloqueante**. Toma el control de la CPU durante 5s (`pBLEScan->start(5, false)`) para garantizar la integridad de la recepción de radio, deteniendo temporalmente el renderizado.

---

## 5. Manual de Operaciones

### 5.1 Instrucciones de Uso
1.  **Inicio**: El sistema arranca en el **Monitor de Sistema** (Status).
2.  **Navegación**: Pulsar botón BOOT para cambiar al **Escáner BLE**.
3.  **Escaneo**: Al entrar en BLE, esperar 5 segundos (barra de progreso) para ver resultados.

### 5.2 Compilación y Despliegue (Scripts de Automatización)
Se han incluido scripts de PowerShell en la raíz del proyecto para facilitar el flujo de trabajo. Los scripts están diseñados para producir una **salida mínima en pantalla** (apta para agentes de IA y terminales con buffer limitado) mientras guardan el **detalle completo en ficheros de log** para depuración.

**Compilar:**
```powershell
./build.ps1
```
*   **Salida en pantalla**: Progreso por pasos (`[1/3]`, `[2/3]`, `[3/3]`), resultado (`SUCCESS`/`FAILED`), tiempo de compilación y uso de RAM/Flash.
*   **Log detallado**: `build_debug_log.txt` — Contiene toda la salida del compilador (warnings, includes, linking, etc.).
*   **En caso de error**: Extrae y muestra automáticamente las líneas con `error:` en pantalla.

**Subir Firmware + Monitor Serie:**
```powershell
./upload.ps1
```
*   **Salida en pantalla**: Progreso por pasos (`[1/2]` upload, `[2/2]` monitor), resultado, puerto COM detectado y luego el monitor serie en directo.
*   **Log detallado**: `upload_log.txt` — Contiene toda la salida del proceso de compilación incremental, flashing y verificación.
*   **Monitor Serie**: Se abre automáticamente tras un upload exitoso para capturar los logs de arranque del ESP32.

**Ficheros de Log generados:**
| Fichero | Contenido | Cuándo consultar |
| :--- | :--- | :--- |
| `build_debug_log.txt` | Salida completa del compilador | Warnings, errores de linking, análisis de dependencias |
| `upload_log.txt` | Salida completa del flash | Errores de conexión al puerto serie, verificación de firmware |

*(Nota: Estos scripts detectan automáticamente la instalación de PlatformIO en el perfil de usuario)*

### 5.3 Compilación Manual (Legacy)
Para usuarios avanzados usando terminal PowerShell:

**Compilar:**
```powershell
& "C:\Users\egavi\.platformio\penv\Scripts\platformio.exe" run --environment esp32-s3-devkitc-1
```

**Subir Firmware:**
```powershell
& "C:\Users\egavi\.platformio\penv\Scripts\platformio.exe" run --environment esp32-s3-devkitc-1 --target upload
```

**Monitor Serial:**
```powershell
& "C:\Users\egavi\.platformio\penv\Scripts\platformio.exe" device monitor --environment esp32-s3-devkitc-1
```

---

## 6. Optimización & Rendimiento (Post-Compilación)
Tras el análisis estático y de compilación del 26/01/2026, se aplicaron mejoras de "Zero-Allocation":

### 6.1 Optimización de Heap en Loops Críticos (ScreenBLE)
Se detectó una alta presión en el Heap debido a la creación de objetos `std::string` temporales dentro del bucle de renderizado de resultados BLE.
**Solución**:
*   Reemplazo de `substr()` (que reserva nueva memoria) por aritmética de punteros `char*`.
*   Reemplazo de truncado de cadenas dinámico por formateo `snprintf(..., "%.6s", ...)`.
*   **Resultado**: Reducción de ~30 allocs/frame durante la visualización de listas densas, eliminando fragmentación de RAM.

### 6.2 Eficiencia Gráfica (BaseSprite)
Se ratifica el uso de `display.clear()` + Redraw completo como estrategia válida bajo el principio "Virtuoso". Aunque consume más ciclos que un "Partial Update", garantiza la eliminación de artefactos visuales (ghosting) sin complejidad de código adicional.

### 6.3 Solución a Heap Fragmentation (El "Black Screen Bug")
**Síntoma**: Al salir de la pantalla BLE (Heavy Load) y volver a Status, la pantalla se quedaba negra. Los logs muestran: `CRITICAL: Sprite Alloc Failed!`.
**Diagnóstico Revalidado (26/01/2026)**:
*   El Stack Bluetooth y la lista de resultados fragmentan severamente el Heap interno (SRAM).
*   Aunque el Sprite de `ScreenStatus` es de solo ~153KB, requiere un bloque *contiguo*.
*   La pila BLE no libera toda la memoria inmediatamente al llamar a `clearResults()`.

**Solución Virtuosa**:
1.  **Activación de PSRAM (Revisada)**: Se añadieron flags `-D BOARD_HAS_PSRAM` y `-mfix-esp32-psram-cache-issue` en `platformio.ini`.
2.  **Limpieza Agresiva**: En `ScreenBLE::onExit` y al final del escaneo, se fuerza `pBLEScan->clearResults()` para liberar memoria interna antes de intentar alojar la siguiente pantalla.
3.  **Código Defensivo**: La clase `BaseSprite` ahora captura el fallo de `malloc` y, si la PSRAM falla, evita el crash del sistema, aunque la pantalla permanezca en negro hasta el siguiente ciclo exitoso (Graceful Degradation).

**NOTA IMPORTANTE**: Si el error persiste, confirmar que el hardware físico es realmente un ESP32-S3 *con* PSRAM (versiones N8R2 o N8R8). En placas "N8" (sin R suffix), la PSRAM no existe y la única solución es reducir la profundidad de color o usar renderizado directo sin Sprites.

**Monitor Serial:**
```powershell
& "C:\Users\egavi\.platformio\penv\Scripts\platformio.exe" device monitor --environment esp32-s3-devkitc-1
```

### 6.4 Estabilidad crítica (Watchdog & Async BLE)
**Problema**: La conexión vía `BLEClient::connect()` es una operación bloqueante que puede superar los 8 segundos (Timeout por defecto). En el ESP32, realizar tareas bloqueantes en el Loop Principal impide refrescar el `Task Watchdog Timer (TWDT)`, provocando reinicios automáticos (Reset loop) a nivel hardware.

**Solución (ScreenGamepad)**:
*   **Offloading**: Implementación de **Async Task** (FreeRTOS) para la lógica de conexión.
*   **Non-Blocking UI**: El Loop UI (`onLoop`) permanece fluido a 60FPS mientras la conexión ocurre en segundo plano (Core 0).
*   **Isolation**: Uso de `xTaskCreatePinnedToCore` para aislar la inestabilidad del Stack BLE del hilo principal de renderizado.

### 6.5 Renderizado sin parpadeo (Double Buffering)
**Implementación (ScreenAnalogClock)**:
*   Se utiliza `BaseSprite` (wrapper de `TFT_eSprite`) para dibujar toda la escena del reloj en memoria.
*   **Limpieza**: `display.clear()` borra el buffer interno, no la pantalla física.
*   **Push**: Solo se envía al LCD (`display.push()`) una vez que el frame está completo.
*   **Resultado**: Animación fluida de agujas sin el parpadeo característico del dibujado secuencial directo.

### 6.6 Renderizado 7-Segmentos Custom (ScreenFlipClock)
**Problema**: La fuente `Font 7` de TFT_eSPI (estilo LCD) no renderiza correctamente sobre sprites de 8bpp (palettized) cuando se usa `setTextSize(2)`. Los colores personalizados de 16 bits se mapean a la misma entrada de paleta, haciendo los dígitos invisibles.

**Solución**:
*   Renderizado manual de **7 segmentos** usando `fillRect` puro: cada segmento es un rectángulo posicionado por aritmética.
*   Tabla de lookup `SEGS[10]` con codificación estándar `gfedcba` (bitmask).
*   Se usan colores estándar de la paleta (`TFT_GREEN`, `TFT_DARKGREEN`) que se mapean correctamente a cualquier profundidad de color.
*   Los segmentos apagados se renderizan en gris muy oscuro (`0x0841`) para dar efecto "ghosted".
*   **Resultado**: Dígitos nítidos y grandes (56×160px) que funcionan perfectamente en 8bpp.

### 6.7 Pipeline de Build Optimizado (Compilación Incremental)
**Problema**: Cada compilación tardaba ~100s porque `build.ps1` borraba todo el directorio temporal (incluyendo la caché `.pio`), y `upload.ps1` recompilaba todo porque apuntaba al directorio fuente en OneDrive.

**Solución**:
*   `build.ps1`: Copia solo los ficheros fuente al directorio temporal (`C:\Users\egavi\pio_temp_build\TechTest_v2`) preservando la carpeta `.pio` entre compilaciones.
*   `upload.ps1`: Usa `--project-dir "$BuildDir"` apuntando al mismo directorio temporal donde ya existe el `firmware.bin` compilado. Verifica su existencia antes de intentar upload.
*   **Resultado**: Build incremental ~18-30s (vs ~100s), Upload ~33s sin recompilación (vs ~150s).

### 6.8 Fix Crítico: Paleta 8bpp en BaseSprite
**Problema**: Múltiples pantallas presentaban defectos gráficos aparentemente no relacionados: dígitos invisibles en FlipClock, agujas del reloj desapareciendo en AnalogClock, y colores incorrectos en general.

**Causa Raíz**: `BaseSprite::init()` llamaba a `createPalette(nullptr, 16)`, generando solo **16 entradas de paleta** para un sprite de 8bpp que tiene un espacio de **256 índices**. Cualquier color mapeado a un índice ≥16 resolvía a transparente o negro, haciendo elementos gráficos invisibles de forma intermitente.

**Solución**:
*   Eliminación completa de `createPalette()`. El sprite 8bpp ahora usa el **mapeo nativo 332** (3 bits R, 3 bits G, 2 bits B), que asigna colores directamente sin tabla de paleta.
*   Colores estándar (`TFT_GREEN`, `TFT_DARKGREEN`, `TFT_RED`, etc.) se mapean correctamente al espacio 332 sin intervención.
*   **Resultado**: Todos los defectos gráficos resueltos de raíz. Las agujas, dígitos y elementos UI son ahora consistentemente visibles.

### 6.9 Renderizado de Agujas con fillTriangle (ScreenAnalogClock)
**Problema**: Las agujas dibujadas con `drawLine` eran de 1px de ancho, lo que en 8bpp (resolución de color limitada) las hacía intermitentemente invisibles o con flickering.

**Solución**:
*   Reemplazo de `drawLine` por `fillTriangle`: cada aguja se renderiza como un triángulo con base de 2-4px y punta afilada.
*   La aguja de segundos usa `baseHW=2` (half-width) para evitar flickering sin perder la estética delgada.
*   **Resultado**: Agujas sólidas y visibles en todas las condiciones de renderizado.

### 6.10 Pantalla de Boot Rediseñada (BootConsoleScreen)
**Diseño**: Interfaz tipo terminal verde monocromática durante la secuencia de arranque.
*   **Barra vertical de progreso** (10px, lateral izquierdo): Se rellena de arriba hacia abajo conforme avanzan los pasos del boot.
*   **Log incremental** (derecha): Cada paso completado aparece con prefijo `. `, el paso actual con `>` en verde brillante.
*   **Footer**: Nombre del paso actual + porcentaje, alineado a la derecha.
*   **Secuencia**: Init Hardware (10%) → WiFi Connect (30%) → GeoIP Lookup (60%) → NTP Sync (80%) → Web Server (90%) → Ready (100%).

### 6.11 Servidor Web Embebido (WebService)
Servidor HTTP ligero en puerto 80 que permite monitorizar y controlar el sistema desde cualquier dispositivo en la misma red WiFi.

**Arquitectura — Shell + AJAX (Split-Payload Pattern):**
*   **Singleton** (`WebService::getInstance()`) inicializado durante el boot, tras la conexión WiFi.
*   Usa `WebServer` nativo del ESP32 (no requiere librerías externas).
*   `handleClient()` invocado en cada iteración del `loop()` principal.
*   **Diseño Split-Payload**: El dashboard se sirve en **dos respuestas PROGMEM separadas** (`DASH_HTML` ~3.3KB + `DASH_JS` ~4.9KB), cada una por debajo de la ventana TCP del ESP32 (~5744 bytes). El HTML referencia `<script src='/d.js'>` para que el navegador haga una segunda petición independiente.
*   **Carga secuencial de datos**: El JS ejecuta las peticiones API en cadena (`lS().then(lG).then(lP)`) en lugar de en paralelo, porque el `WebServer` nativo del ESP32 es **single-threaded** y solo puede atender una conexión TCP simultánea.

**Seguridad:**
*   **PIN de 4 dígitos** generado con `esp_random()` en cada arranque. Visible en la pantalla TFT (`ScreenStatus`) y en los logs de boot.
*   **Token de sesión** (`_sessionToken`): Hash aleatorio de 8 caracteres hex regenerado en cada boot. Las cookies de sesiones anteriores quedan automáticamente invalidadas al reiniciar el ESP32.
*   **Cookie `auth=<token>`**: Se establece tras autenticación exitosa. Todas las rutas protegidas validan el token contra el de la sesión actual.
*   **Ruta `/logout`**: Borra la cookie y redirige al login.

**Rutas:**
| Ruta | Método | Protegida | Descripción | Tamaño |
| :--- | :--- | :--- | :--- | :--- |
| `/` | GET | No | Login (o redirect a dashboard si autenticado) | ~1.5KB |
| `/login` | POST | No | Validación del PIN | — |
| `/dashboard` | GET | Sí | Shell HTML estático (CSS + estructura DOM + preview container) | ~3.3KB PROGMEM |
| `/d.js` | GET | Sí | JavaScript del dashboard (lógica AJAX + color preview) | ~4.9KB PROGMEM |
| `/api/system` | GET | Sí | JSON: uptime, CPU, temp, SDK, heap, network | ~350B dinámico |
| `/api/gol/state` | GET | Sí | JSON: velocidad, colores alive/dead (hex) | ~80B dinámico |
| `/api/gol/config` | POST | Sí | Configura velocidad y colores del GOL | — |
| `/api/gol/action` | POST | Sí | Acciones: toggle, reset, clear | — |
| `/api/palettes` | GET | Sí | JSON con todas las paletas disponibles | ~27KB PROGMEM (WiFiClient stream) |
| `/api/palettes/set` | POST | Sí | Aplica una paleta (folder + nombre) | — |
| `/logout` | GET | No | Cierra sesión (borra cookie) | — |
| `/favicon.ico` | GET | No | Icono 1×1px verde (cache 7 días) | 62B |

**UI Web:**
*   Diseño **minimal y responsive**: fondo claro, tarjetas blancas con sombra sutil, fuente del sistema.
*   **Login**: Tarjeta centrada con campo PIN numérico, botón oscuro, mensaje de error inline.
*   **Dashboard**: 3 pestañas (System, Game of Life, Palettes) con contenido cargado dinámicamente vía AJAX.
    *   **System**: Uptime, CPU, temperatura, SDK, memoria (con barra de heap), red (SSID, RSSI, IP, MAC).
    *   **Game of Life**: Slider de velocidad, color pickers (alive/dead), botones Play/Pause, Random, Clear.
    *   **Palettes**: Selectores jerárquicos Folder → Palette con **preview visual de colores**: al seleccionar una paleta se muestran barras horizontales con cada color, código hex y porcentaje de frecuencia de aparición (ranqueado de mayor a menor). Botón Apply.
*   **Logging descriptivo**: Todas las peticiones 404 se logean con método, URI e IP del cliente.

### 6.12 Lecciones de HTTP en ESP32 (TCP Buffer Constraints)
Durante el desarrollo del WebService se descubrió un **límite crítico del hardware** que condicionó toda la arquitectura web.

**El Problema — `ERR_CONTENT_LENGTH_MISMATCH`:**
El `WebServer` nativo del ESP32 llama internamente a `WiFiClient::write()` con el flag `MSG_DONTWAIT` (non-blocking). Si el payload excede la **ventana TCP del socket** (~5744 bytes en ESP32-S3), `write()` devuelve un **short write** (menos bytes de los enviados). El WebServer **ignora el valor de retorno**, por lo que los bytes restantes se pierden silenciosamente. El navegador recibe un `Content-Length: 8000` en la cabecera pero solo llega ~5700 bytes de cuerpo, generando `net::ERR_CONTENT_LENGTH_MISMATCH`.

**Intentos Fallidos (Registro de Batalla):**
| Intento | Resultado | Por qué falló |
| :--- | :--- | :--- |
| `_server.send(200, "text/html", page)` | `ERR_CONTENT_LENGTH_MISMATCH` | Payload de 8218 bytes > ventana TCP de 5744 bytes. Short write silencioso. |
| Chunked Transfer (`sendContent()` en loop) | `ERR_INVALID_CHUNKED_ENCODING` | `sendContent()` hace 3 writes internos por chunk (size line + data + CRLF). Si cualquiera falla parcialmente, el stream chunked se corrompe. |
| Raw `WiFiClient::write()` con headers manuales | `Connection reset by peer` (errno 104) | Escribir `HTTP/1.1 200 OK\r\n` directamente al `WiFiClient` conflicta con el estado interno del `WebServer`, que ya ha procesado la request. |
| `WiFiClient::write()` con `setContentLength` | `Connection reset by peer` (errno 104) | El `WiFiClient` obtenido vía `_server.client()` comparte el socket con el `WebServer`. Escribir directamente causa conflicto de estado. |
| `send_P()` con PROGMEM grande | `ERR_CONTENT_LENGTH_MISMATCH` | `send_P` internamente usa el mismo `write()` non-blocking sin reintentos. |

**Solución Final — Split-Payload Pattern:**
Dividir todo payload en **archivos PROGMEM independientes**, cada uno menor que la ventana TCP (~5744 bytes):
*   `DASH_HTML` (~3344 bytes) → servido vía `send_P()` ✅
*   `DASH_JS` (~4902 bytes) → servido vía `send_P()` ✅
*   `PALETTES_JSON` (27116 bytes) → servido vía `WiFiClient::write()` con **backpressure retry loop** (256 bytes/chunk, hasta 50 reintentos con `delay(5)` entre cada uno) ✅

**Patrón WiFiClient con Backpressure (para payloads > 5KB):**
```cpp
_server.setContentLength(len);          // Cabecera Content-Length exacta
_server.sendHeader("Connection", "close");
_server.send(200, "application/json", ""); // Solo headers

WiFiClient client = _server.client();
char buf[256];
size_t pos = 0;
while (pos < len && client.connected()) {
    size_t toSend = min(len - pos, (size_t)256);
    memcpy_P(buf, DATA + pos, toSend);
    size_t sent = 0;
    uint8_t retries = 0;
    while (sent < toSend && client.connected() && retries < 50) {
        int w = client.write((const uint8_t*)(buf + sent), toSend - sent);
        if (w > 0) { sent += w; retries = 0; }
        else       { retries++; delay(5); }
    }
    pos += sent;
    yield();
}
```

**Problema Adicional — Conexiones Simultáneas:**
El `WebServer` del ESP32 es **single-threaded** (procesa una request, responde, luego acepta la siguiente). Si el navegador dispara 3 `fetch()` en paralelo (como `lS(); lG(); lP();`), solo la primera se procesa; las otras dos obtienen `ERR_CONNECTION_RESET` porque el servidor las rechaza.

**Solución**: Encadenar las peticiones secuencialmente en JavaScript:
```javascript
// ❌ MAL: Paralelo — 2 de 3 fallarán en ESP32
lS(); lG(); lP();

// ✅ BIEN: Secuencial — cada petición espera a la anterior
lS().then(function(){ return lG() }).then(function(){ return lP() });
```

---

## 7. Changelog

### v0.6.0 — 2026-04-02
**Refactorización Completa del Web Dashboard (Shell + AJAX):**

El dashboard monolítico HTML (~8KB en una sola respuesta) fue reemplazado por una arquitectura **Split-Payload + AJAX** para superar las limitaciones del buffer TCP del ESP32.

**Nuevas Rutas API:**
*   `/d.js` — JavaScript del dashboard servido como recurso PROGMEM independiente.
*   `/api/system` — JSON con información de sistema (uptime, CPU, temp, heap, red).
*   `/api/gol/state` — JSON con estado actual del Game of Life (velocidad, colores).
*   `/api/palettes` — JSON completo (~27KB) servido con WiFiClient backpressure retry.
*   `/api/palettes/set` — POST para aplicar una paleta al TFT.

**Arquitectura:**
*   HTML estático `DASH_HTML` (~3.3KB, PROGMEM) + JS `DASH_JS` (~4.9KB, PROGMEM) → ambos bajo la ventana TCP de 5744 bytes.
*   Carga de datos secuencial (`lS().then(lG).then(lP)`) para respetar la naturaleza single-threaded del WebServer.
*   `PALETTES_JSON` (27KB) transferido con loop de backpressure (`WiFiClient::write` + retry + `delay(5)`).

**Dashboard UI (3 pestañas):**
*   **System**: Uptime, CPU, temperatura, SDK, memoria con barra visual, info de red con chip de estado.
*   **Game of Life**: Slider velocidad, color pickers (alive/dead), botones control (Play/Pause, Random, Clear).
*   **Palettes**: Selectores jerárquicos Folder → Palette con preview de colores, botón Apply.

### v0.6.1 — 2026-04-02
**Preview Visual de Paletas en Web Dashboard:**

*   Al seleccionar una paleta en la pestaña Palettes, se muestra un **preview visual** con los colores que la componen, ranqueados por frecuencia de aparición en el muestreo de la imagen original.
*   Cada color se representa como una **barra horizontal** proporcional a su porcentaje de frecuencia, acompañada del **código hex** y el **porcentaje**.
*   El preview se actualiza en tiempo real al cambiar Folder o Palette — sin peticiones HTTP adicionales (renderizado client-side con los datos ya cargados de `/api/palettes`).
*   Contenedor `#pPrev` con cabecera dinámica que muestra el número de colores de la paleta.
*   CSS: clases `.sw` (fila), `.sb` (barra de color), `.sh` (hex monospace), `.sl` (porcentaje).
*   **Tamaños PROGMEM actualizados**: `DASH_HTML` 2.9→3.3KB, `DASH_JS` 4.1→4.9KB. Ambos bajo el límite TCP de 5744 bytes.

**Reorganización de Scripts Auxiliares:**
*   Movidos 9 scripts de desarrollo (`.py` + `.ps1`) de la raíz del proyecto a la nueva carpeta `aux_/` (sufijo `_` por compatibilidad con OneDrive).
*   Documentados en DevGuidelines.md §11 (propósito, uso y clasificación de cada script).

**Bugs Resueltos:**
*   Fix: `ERR_CONTENT_LENGTH_MISMATCH` — payload HTML de 8218 bytes excedía la ventana TCP (5744 bytes).
*   Fix: `ERR_INVALID_CHUNKED_ENCODING` — `sendContent()` corrupts chunked stream on partial writes.
*   Fix: `ERR_CONNECTION_RESET` — fetch() paralelos rechazados por WebServer single-threaded.
*   Fix: `sT is not defined` — el `<script>` se ubicaba al final del HTML truncado, nunca llegaba al navegador.
*   Fix: `Connection reset by peer (errno 104)` — raw WiFiClient writes conflictan con el estado interno del WebServer.

### v0.5.0 — 2026-03-29
**Nuevo: Servidor Web Embebido (`WebService`):**
*   Servidor HTTP en puerto 80 con `WebServer` nativo del ESP32.
*   PIN aleatorio de 4 dígitos generado con `esp_random()` en cada boot.
*   Token de sesión único por boot — las cookies de sesiones anteriores se invalidan automáticamente al reiniciar.
*   Autenticación por cookie (`auth=<sessionToken>`). Ruta `/logout` para cerrar sesión.
*   Dashboard con información de System, Memory y Network en diseño minimal responsive.
*   Favicon inline (1×1px verde, cache 7 días) para evitar 404.
*   Logging descriptivo de peticiones 404 (método + URI + IP del cliente).

**Integración en Boot:**
*   Nuevo paso "Web Server" al 90% en `BootOrchestrator`, entre NTP Sync y Ready.
*   URL y PIN visibles en pantalla de boot y en `ScreenStatus`.

**ScreenStatus Actualizado:**
*   SSID y RSSI fusionados en una línea: `Cuba (-50dBm)`.
*   Nueva línea "Web" con URL + PIN entre corchetes: `http://192.168.68.50 [7917]`.

**UI Web Minimal:**
*   Rediseño completo abandonando la estética de consola verde.
*   Fondo claro (#f5f5f5), tarjetas blancas redondeadas, fuente del sistema (San Francisco/Segoe UI/Roboto).
*   Login: tarjeta centrada, campo PIN numérico con `inputmode='numeric'`.
*   Dashboard: 3 tarjetas (System, Memory, Network), barra de heap azul, chip de estado semántico.

### v0.4.1 — 2026-03-29
**Rediseño Boot Screen:**
*   Nueva `BootConsoleScreen` con layout vertical: barra de progreso en lateral izquierdo (relleno top-down) + log incremental a la derecha.
*   Estética terminal verde coherente con el resto de la UI.
*   Paso actual destacado con `>` en verde brillante; pasos completados en verde apagado.

**Fixes Críticos (Renderizado 8bpp):**
*   **Fix: Paleta 8bpp rota en BaseSprite** — `createPalette(nullptr, 16)` generaba solo 16 entradas para 256 índices, causando elementos invisibles en múltiples pantallas. Eliminada paleta; se usa mapeo nativo 332.
*   **Fix: Agujas de reloj invisibles** — Reemplazo de `drawLine` por `fillTriangle` en ScreenAnalogClock. Base de 2-4px para visibilidad sólida en 8bpp.
*   **Fix: Flickering aguja de segundos** — `baseHW` aumentado de 1 a 2px para rasterización estable.
*   **Fix: Calendario solapado** — Separación entre título del mes y cabeceras de día de semana (`hdY` offset).

### v0.4.0 — 2026-03-28
**Nuevas Pantallas:**
*   **ScreenFlipClock**: Nueva pantalla tipo "flip clock" con estética retro.
    *   Dígitos HH:MM en paneles de 72×180px con efecto split (bisagra).
    *   Renderizado custom de 7 segmentos via `fillRect` (compatible con 8bpp).
    *   Colon parpadeante cada segundo.
    *   Footer compacto: `:SS` centrado + countdown NTP alineado a la derecha.

**Rediseño de Pantallas Existentes:**
*   **ScreenAnalogClock** → Layout split 50/50:
    *   Mitad izquierda: Reloj analógico (R=78px) con hora digital en font pequeño y verde apagado.
    *   Mitad derecha: Calendario mensual completo (tabla 7 columnas × 6 filas, celdas de 22×30px).
    *   Día actual rodeado por caja roja. Fines de semana en tono verde apagado.
    *   Línea divisoria vertical sutil entre ambas mitades.
*   **ScreenStatus** → 3 secciones:
    *   SYSTEM: Time, Uptime, CPU Freq, Int Temp.
    *   MEMORY: Tabla de 2 columnas (Heap/MaxBlk, MinFree/Flash).
    *   NETWORK: SSID, IP, MAC WiFi, RSSI.

**Mejoras Visuales:**
*   Header en tono verde apagado (`TFT_DARKGREEN`) con texto negro para FlipClock y AnalogClock (reduce deslumbramiento).

**TimeService:**
*   Nuevos métodos: `getTimeParts(h,m,s)`, `getDateParts(y,m,d,wday)`.
*   Tracking de sincronización NTP: `getSecsSinceSync()`, `getSecsUntilNextSync()`.
*   Intervalo de resync configurable (`NTP_SYNC_INTERVAL = 3600s`).

**Build Pipeline:**
*   `build.ps1`: Compilación incremental (preserva caché `.pio`). Salida limpia con log en `build_debug_log.txt`.
*   `upload.ps1`: Upload sin recompilación (reutiliza `firmware.bin` del build). Log en `upload_log.txt`.
*   IntelliSense: Generación de `compile_commands.json` via `pio run -t compiledb` + configuración en `c_cpp_properties.json`.

**Fixes:**
*   Fix: ScreenAnalogClock usaba `millis()` para simular hora → ahora usa NTP real via `TimeService`.
*   Fix: Linker error `undefined reference to ScreenFlipClock::SEGS` (C++14 ODR-use de `static constexpr`) → tabla movida a `static const` local.
*   Fix: Time mostraba "N/A" intermitente en ScreenStatus.

