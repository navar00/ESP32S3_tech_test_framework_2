---
applyTo: "src/hal/StorageHAL.*"
description: "Reglas locales para StorageHAL (NVS Flash) — códigos de error, namespaces y límites."
---

# StorageHAL — reglas locales (NVS)

`StorageHAL` envuelve la NVS de ESP-IDF (vía `Preferences` de Arduino). La NVS es flash-backed, persistente entre resets y la única forma soportada de guardar credenciales, contadores y configuración cross-boot.

Referencia oficial: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/storage/nvs_flash.html>.

## Reglas duras
- **Namespace ≤ 15 bytes** (límite NVS). Si se necesita más jerarquía, usar prefijos de clave.
- **Clave ≤ 15 bytes**. Validar al añadir nuevas claves.
- **Una operación NVS = una transacción**: cada `putXxx()` hace flush implícito. No agrupar dentro de un loop tight; bufferizar en RAM y volcar al final.
- **No escribir desde un ISR** ni desde callbacks de `esp_timer` con flash deshabilitado. La escritura NVS bloquea cache → panic `Cache disabled`.
- **`StorageHAL::begin()` es idempotente**: llamarlo 2 veces no debe crashear. Validar antes de cualquier `putXxx`.

## Manejo de errores (no silenciar)
NVS devuelve códigos `esp_err_t` documentados. Los más relevantes:
| Código | Significado | Acción |
|---|---|---|
| `ESP_OK` | Operación correcta | continuar |
| `ESP_ERR_NVS_NOT_FOUND` | Clave o namespace no existe | crear con valor por defecto y loggear |
| `ESP_ERR_NVS_NO_FREE_PAGES` | Partición NVS llena/corrupta | `nvs_flash_erase()` + reinicializar (último recurso, **borra todo**) |
| `ESP_ERR_NVS_INVALID_HANDLE` | Falta `begin()` o `end()` previo cerró el handle | comprobar ciclo de vida |
| `ESP_ERR_NVS_INVALID_LENGTH` | Buffer demasiado corto en `getString` | aumentar buffer |
| `ESP_ERR_NVS_VALUE_TOO_LONG` | String > 4000 bytes | trocear o usar `LittleFS` |

Patrón de logging obligatorio:
```cpp
esp_err_t err = nvs_set_u32(handle, "boot_cnt", value);
if (err != ESP_OK) {
    Logger::log("STG", "nvs_set_u32(boot_cnt) failed: %s", esp_err_to_name(err));
}
```
Nunca `Logger::log("STG", "NVS error")` sin código.

## Namespaces canónicos en uso
Documentar aquí cualquier namespace nuevo para evitar colisiones (límite 15 bytes):
- `boot` — contador de arranques, última `reset_reason`.
- `wifi` — credenciales preferidas (si se externalizan de `Config.h`).
- `geo` — última posición geo cacheada (offset zona horaria).

## Antipatrones
- Llamar `nvs_flash_erase()` "por las dudas" en runtime → borra credenciales del usuario.
- Usar `Preferences` y `nvs_*` directamente en el mismo namespace → conflictos de handle.
- Escribir contadores en cada iteración del loop → wear-out de flash. Bufferizar y flush cada N segundos o en `onExit`.
- Almacenar binarios > 4 KB en NVS (sprites, paletas) → usar `LittleFS` o PROGMEM.
