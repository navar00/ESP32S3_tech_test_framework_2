#pragma once
#include "IScreen.h"
#include "BaseSprite.h"
#include "../core/GOLConfig.h"
#include "../core/Logger.h"

// Conway's Game of Life Screen
class ScreenGameOfLife : public IScreen
{
private:
    BaseSprite display;
    uint8_t* gridA = nullptr;
    uint8_t* gridB = nullptr;
    uint8_t* currentGrid = nullptr;
    uint8_t* nextGrid = nullptr;

    const int CELL_SIZE = 4;
    int cols = 0;
    int rows = 0;
    int rowBytes = 0;
    int gridBytes = 0;

    unsigned long lastUpdateMs = 0;

    inline bool getCellBit(uint8_t* grid, int x, int y) {
        if(x < 0) x = cols - 1;
        if(x >= cols) x = 0;
        if(y < 0) y = rows - 1;
        if(y >= rows) y = 0;
        int idx = y * rowBytes + (x / 8);
        return (grid[idx] & (1 << (x % 8))) != 0;
    }

    inline void setCellBit(uint8_t* grid, int x, int y, bool alive) {
        if(x < 0) x = cols - 1;
        if(x >= cols) x = 0;
        if(y < 0) y = rows - 1;
        if(y >= rows) y = 0;
        int idx = y * rowBytes + (x / 8);
        if (alive) {
            grid[idx] |= (1 << (x % 8));
        } else {
            grid[idx] &= ~(1 << (x % 8));
        }
    }

    void randomizeGrid() {
        if(!currentGrid) return;
        for(int i = 0; i < gridBytes; i++) {
            currentGrid[i] = (uint8_t)random(256);
            nextGrid[i] = 0;
        }
    }

    void clearGrid() {
        if(!currentGrid) return;
        memset(currentGrid, 0, gridBytes);
        memset(nextGrid, 0, gridBytes);
    }

    void drawFullGrid(uint16_t cAlive, uint16_t cDead) {
        if(!display.isReady()) return;
        TFT_eSprite* sprite = display.getRaw();
        sprite->fillSprite(cDead);
        for(int y=0; y<rows; y++) {
            for(int x=0; x<cols; x++) {
                if(getCellBit(currentGrid, x, y)) {
                    sprite->fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE-1, CELL_SIZE-1, cAlive);
                }
            }
        }
        display.push();
    }

public:
    const char *getName() override { return "GAME_OF_LIFE"; }

    void onEnter() override {
        IScreen::onEnter();
        auto& tft = DisplayHAL::getInstance().getRaw();
        display.init(tft);

        cols = display.getRaw()->width() / CELL_SIZE;
        rows = display.getRaw()->height() / CELL_SIZE;
        rowBytes = (cols + 7) / 8;
        gridBytes = rowBytes * rows;

        if(!gridA) gridA = (uint8_t*)malloc(gridBytes);
        if(!gridB) gridB = (uint8_t*)malloc(gridBytes);
        currentGrid = gridA;
        nextGrid = gridB;

        bool r, c;
        GOLConfig::getInstance().consumeTriggers(r, c);

        randomizeGrid();
        lastUpdateMs = millis();
        
        auto state = GOLConfig::getInstance().getState();
        drawFullGrid(state.colorAlive, state.colorDead);
    }

    void onExit() override {
        display.free();
        if(gridA) { free(gridA); gridA = nullptr; }
        if(gridB) { free(gridB); gridB = nullptr; }
    }

    void onLoop() override {
        if(!display.isReady() || !currentGrid || !nextGrid) return;

        bool doReset, doClear;
        GOLConfig::getInstance().consumeTriggers(doReset, doClear);
        auto state = GOLConfig::getInstance().getState();

        if(doClear) {
            clearGrid();
            drawFullGrid(state.colorAlive, state.colorDead);
        } else if(doReset) {
            randomizeGrid();
            drawFullGrid(state.colorAlive, state.colorDead);
        }

        if(!state.isRunning) return;

        if((millis() - lastUpdateMs) > state.cycleSpeedMs) {
            lastUpdateMs = millis();
            stepGeneration(state.colorAlive, state.colorDead);
        }
    }

private:
    void stepGeneration(uint16_t cAlive, uint16_t cDead) {
        TFT_eSprite* sprite = display.getRaw();
        sprite->fillSprite(cDead);

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int neighbors = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        if (getCellBit(currentGrid, x + dx, y + dy)) {
                            neighbors++;
                        }
                    }
                }

                bool vivo = getCellBit(currentGrid, x, y);
                bool nextVivo = false;

                if (vivo && (neighbors == 2 || neighbors == 3)) {
                    nextVivo = true;
                } else if (!vivo && neighbors == 3) {
                    nextVivo = true;
                }

                setCellBit(nextGrid, x, y, nextVivo);
                if (nextVivo) {
                    sprite->fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE-1, CELL_SIZE-1, cAlive);
                }
            }
        }

        uint8_t* temp = currentGrid;
        currentGrid = nextGrid;
        nextGrid = temp;
        
        display.push();
    }
};
