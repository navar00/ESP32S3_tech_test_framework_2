---
applyTo: "src/screens/**"
description: "Hard rules for IScreen implementations and BaseSprite usage."
---

# Screens — reglas locales

- `IScreen` real: `getName / onEnter / onLoop / onExit / on1SecTimer`. `onEnter()` ya hace `clear()` por defecto — no duplicar.
- Componer con `BaseSprite`. `init()` en `onEnter`, `free()` en `onExit`. Nada estático grande.
- Sprite 8 bpp usa **mapeo nativo 332** — NUNCA llamar `createPalette()`.
- Líneas finas (1 px) en 8 bpp: usar `fillTriangle` con base ≥ 2 px, no `drawLine`.
- Un único `pushSprite(0, 0)` por frame. Nada de dibujado directo al TFT.
- Estética retro: fondo `TFT_BLACK`, datos `TFT_GREEN`, header inverso (`drawHeader`), separadores `TFT_DARKGREEN`. Font 2, leading 18 px, margen X 4 px.
- `onLoop()` no bloquea > 50 ms. Trabajo bloqueante → `xTaskCreatePinnedToCore` Core 0.
- Para detalle: skill `tft-screen`.
