#pragma once

#include "IScreen.h"
#include <ArduinoJson.h>
#include "../core/PalettesData.h"

class ScreenPalette : public IScreen
{
private:
    uint16_t *_matrix = nullptr;
    const int cols = 320 / 40; // 8
    const int rows = 240 / 40; // 6
    unsigned long _lastUpdate = 0;

    String _currentFolder;
    String _currentPaletteName;
    uint16_t _colors[16];
    int _numColors = 0;

    // Utilidad para extraer colores del JSON y convertirlos a RGB565 (TFT)
    void extractColorsFromJSON()
    {
        _numColors = 0;
        DynamicJsonDocument doc(32768); // El JSON puede ser muy grande
        DeserializationError error = deserializeJson(doc, PALETTES_JSON);

        if (!error && doc.containsKey(_currentFolder))
        {
            JsonObject folderObj = doc[_currentFolder];
            if (folderObj.containsKey(_currentPaletteName))
            {
                JsonArray pal = folderObj[_currentPaletteName];
                for (JsonVariant c : pal)
                {
                    if (_numColors >= 16)
                        break;

                    JsonArray rgb = c["rgb"];
                    if (rgb.size() == 3)
                    {
                        uint8_t r = rgb[0].as<uint8_t>();
                        uint8_t g = rgb[1].as<uint8_t>();
                        uint8_t b = rgb[2].as<uint8_t>();

                        // Convertir RGB888 a RGB565
                        _colors[_numColors++] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                    }
                }
            }
        }

        // Fallback porsi_acaso
        if (_numColors == 0)
        {
            _colors[0] = 0xF800; // Red
            _colors[1] = 0x07E0; // Green
            _colors[2] = 0x001F; // Blue
            _numColors = 3;
        }
    }

public:
    ScreenPalette()
    {
        _currentFolder = "GertrudArndt";
        _currentPaletteName = "paleta1";
    }

    // Set desde el Web Server
    void setPalette(String folder, String name)
    {
        _currentFolder = folder;
        _currentPaletteName = name;
        extractColorsFromJSON(); // Recargar colores
    }

    String getFolder() { return _currentFolder; }
    String getPaletteName() { return _currentPaletteName; }

    void onEnter() override
    {
        TFT_eSPI *tft = &DisplayHAL::getInstance().getRaw();
        if (!_matrix)
        {
            _matrix = new uint16_t[cols * rows];
        }

        // Parsear el mega JSON (bloqueante)
        extractColorsFromJSON();

        // Rellenar pantalla inicial
        for (int i = 0; i < cols * rows; i++)
        {
            _matrix[i] = _colors[random(_numColors)];
        }

        tft->fillScreen(TFT_BLACK);
        drawGrid(tft);
        drawOverlay(tft);
    }

    void drawGrid(TFT_eSPI *tft)
    {
        int CELL_SIZE = 40;
        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols; x++)
            {
                tft->fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, _matrix[y * cols + x]);
            }
        }
    }

    void drawOverlay(TFT_eSPI *tft)
    {
        // Texto en negro abajo derecha
        tft->setTextDatum(BR_DATUM); // Bottom Right
        tft->setTextColor(TFT_BLACK);
        String txt = _currentFolder + " / " + _currentPaletteName;
        // Shadow/borde para que se lea siempre
        tft->drawString(txt, 320 - 5, 240 - 5 + 1);
        tft->drawString(txt, 320 - 5 + 1, 240 - 5);
        tft->setTextColor(TFT_WHITE);
        tft->drawString(txt, 320 - 5, 240 - 5);
    }

    void onLoop() override
    {
        TFT_eSPI *tft = &DisplayHAL::getInstance().getRaw();
        if (millis() - _lastUpdate > 200)
        {
            _lastUpdate = millis();
            bool changed = false;
            int CELL_SIZE = 40;

            for (int i = 0; i < cols * rows; i++)
            {
                if (random(100) < 10)
                { // 10% prob
                    _matrix[i] = _colors[random(_numColors)];
                    int x = i % cols;
                    int y = i / cols;
                    tft->fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, _matrix[i]);
                    changed = true;
                }
            }

            if (changed)
            {
                drawOverlay(tft);
            }
        }
    }

    void onExit() override
    {
        if (_matrix)
        {
            delete[] _matrix;
            _matrix = nullptr;
        }
    }

    const char *getName() override
    {
        return "Palette";
    }
};
