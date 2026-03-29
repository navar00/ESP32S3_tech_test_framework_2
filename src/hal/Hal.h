#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include "../core/Config.h"
#include "../core/Logger.h"

// Virtuoso Standard: Facade Pattern
// This file aggregates all HAL modules for easy inclusion.

#include "DisplayHAL.h"
#include "LedHAL.h"
#include "WatchdogHAL.h"
#include "StorageHAL.h"
#include "InputHAL.h"
