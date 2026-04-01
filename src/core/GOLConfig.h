#pragma once
#include <Arduino.h>

/**
 * @class GOLConfig
 * @brief Singleton for sharing Game of Life state between WebService (HTTP HTTP API) and ScreenGameOfLife (TFT Loop).
 *
 * DESIGN DECISIONS:
 * - Singleton pattern ensures both modules access the exact same memory struct.
 * - Thread-safety: WebServer and Main Loop might run on different FreeRTOS cores. A simple
 *   spin-lock/mutex ensures no tearing when updating RGB colors or state flags.
 */
class GOLConfig
{
public:
    static GOLConfig &getInstance()
    {
        static GOLConfig instance;
        return instance;
    }

    struct State
    {
        // Defaults: Green on Black
        uint16_t colorAlive = 0x07E0;
        uint16_t colorDead = 0x0000;

        // Speed in milliseconds
        uint16_t cycleSpeedMs = 200;

        // State Machine
        bool isRunning = true;

        // Asynchronous Triggers
        bool triggerReset = true; // start with a reset to show something
        bool triggerClear = false;
    };

    State getState()
    {
        portENTER_CRITICAL(&_mux);
        State s = _state;
        portEXIT_CRITICAL(&_mux);
        return s;
    }

    void updateConfig(uint16_t alive, uint16_t dead, uint16_t speed)
    {
        portENTER_CRITICAL(&_mux);
        _state.colorAlive = alive;
        _state.colorDead = dead;
        _state.cycleSpeedMs = speed;
        portEXIT_CRITICAL(&_mux);
    }

    void setRunning(bool running)
    {
        portENTER_CRITICAL(&_mux);
        _state.isRunning = running;
        portEXIT_CRITICAL(&_mux);
    }

    void triggerAction(bool reset, bool clear)
    {
        portENTER_CRITICAL(&_mux);
        if (reset)
            _state.triggerReset = true;
        if (clear)
            _state.triggerClear = true;
        portEXIT_CRITICAL(&_mux);
    }

    void consumeTriggers(bool &outReset, bool &outClear)
    {
        portENTER_CRITICAL(&_mux);
        outReset = _state.triggerReset;
        outClear = _state.triggerClear;
        _state.triggerReset = false;
        _state.triggerClear = false;
        portEXIT_CRITICAL(&_mux);
    }

private:
    GOLConfig()
    {
        _mux = portMUX_INITIALIZER_UNLOCKED;
    }

    portMUX_TYPE _mux;
    State _state;
};
