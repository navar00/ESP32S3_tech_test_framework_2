// test/test_native_smoke/test_main.cpp
//
// Smoke test del harness Unity sobre el entorno `native` (host PC, sin Arduino).
// Sirve para:
//   1. Validar que `pio test -e native` funciona.
//   2. Servir de plantilla para futuros tests sobre lógica pura del proyecto
//      (helpers que no dependan de millis()/Serial/TFT).
//
// Ejecutar:
//   pio test -e native
//
// Cómo añadir más tests:
//   - Crear un fichero hermano test/test_<nombre>/test_main.cpp.
//   - Solo incluir lógica pura. Para código con dependencias Arduino, mockear
//     o mover la lógica a un helper sin dependencias.

#include <unity.h>
#include <cstdint>
#include <cstring>

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Test 1 — Aritmética básica (sanity de la cadena de build)
// ---------------------------------------------------------------------------
void test_arithmetic_sanity(void)
{
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
    TEST_ASSERT_EQUAL_INT(0, 0xFF & 0x00);
}

// ---------------------------------------------------------------------------
// Test 2 — Mapeo color RGB565 desde componentes 8-bit (lógica usada en BaseSprite)
// Replica el cálculo `((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)`.
// ---------------------------------------------------------------------------
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void test_rgb565_mapping(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x0000, rgb565(0, 0, 0));          // black
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, rgb565(0xFF, 0xFF, 0xFF)); // white
    TEST_ASSERT_EQUAL_HEX16(0xF800, rgb565(0xFF, 0, 0));       // red
    TEST_ASSERT_EQUAL_HEX16(0x07E0, rgb565(0, 0xFF, 0));       // green
    TEST_ASSERT_EQUAL_HEX16(0x001F, rgb565(0, 0, 0xFF));       // blue
}

// ---------------------------------------------------------------------------
// Test 3 — Conversión RGB332 (8-bit indexed) usada por BaseSprite con 8 bpp
// Mapping: ((r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6)).
// ---------------------------------------------------------------------------
static inline uint8_t rgb332(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint8_t>((r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6));
}

void test_rgb332_mapping(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00, rgb332(0, 0, 0));
    TEST_ASSERT_EQUAL_HEX8(0xFF, rgb332(0xFF, 0xFF, 0xFF));
    TEST_ASSERT_EQUAL_HEX8(0xE0, rgb332(0xFF, 0, 0));
    TEST_ASSERT_EQUAL_HEX8(0x1C, rgb332(0, 0xFF, 0));
    TEST_ASSERT_EQUAL_HEX8(0x03, rgb332(0, 0, 0xFF));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_arithmetic_sanity);
    RUN_TEST(test_rgb565_mapping);
    RUN_TEST(test_rgb332_mapping);
    return UNITY_END();
}
