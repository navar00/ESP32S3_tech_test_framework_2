#include "BootOrchestrator.h"
#include "Config.h"

// Helper for UI Refresh inside blocking tasks
BootOrchestrator *_orchestratorInstance = nullptr;

void BootOrchestrator::run()
{
    _orchestratorInstance = this;

    // --- Step 0: Serial Self-Report ---
    // Serial is already started in main setup usually, but we ensure speed
    // Serial.begin(Config::SERIAL_BAUD); // Assumed done in setup
    Logger::log("BOOT", "=== Virtuoso Boot Orchestrator ===");
    Logger::log("BOOT", "FW: %s", Config::SYS_VERSION);

    // --- Step 1: Diagnostics ---
    StorageHAL::getInstance().begin();
    StorageHAL::getInstance().incrementBootCounter();
    String resetReason = WatchdogHAL::getInstance().getResetReason();
    Logger::log("BOOT", "Reset Reason: %s", resetReason.c_str());
    Logger::log("BOOT", "Boot Count: %u", StorageHAL::getInstance().getBootCounter());

    // --- Step 2: Watchdog ---
    WatchdogHAL::getInstance().begin(Config::WDT_TIMEOUT_BOOT);

    // --- Step 3: TFT Console ---
    DisplayHAL::getInstance().init();

    // Create temporary sprite for boot console (released at end of run)
    _sprite = new TFT_eSprite(&DisplayHAL::getInstance().getRaw());
    _sprite->createSprite(DisplayHAL::getInstance().width(), DisplayHAL::getInstance().height());

    // Init Console Screen
    _console.setProgress(10, "Init Hardware");
    refreshUI(); // Draw first frame

    logToUI("Display Ready.");
    logToUI("WDT Armed (8s).");

    // --- Step 4: WiFi ---
    _console.setProgress(30, "Network");
    logToUI("Scanning WiFi...");
    refreshUI();
    WatchdogHAL::getInstance().feed();

    bool online = _net.connectBestRSSI(5000); // 5s timeout
    if (online)
    {
        logToUI("WiFi Connected.");
        logToUI("IP: %s", _net.getIP().c_str());
        logToUI("SSID: %s", _net.getSSID().c_str());
    }
    else
    {
        logToUI("WiFi Failed. Offline Mode.");
    }
    refreshUI();

    // --- Step 5: Geo ---
    if (online)
    {
        _console.setProgress(60, "Geolocation");
        WatchdogHAL::getInstance().feed();
        logToUI("Fetching Geo IP...");
        refreshUI();

        if (_geo.fetchAndApply())
        {
            logToUI("TZ: %s", _geo.getTimezone().c_str());
        }
        else
        {
            logToUI("Geo Failed.");
        }
    }

    // --- Step 6: Time ---
    _console.setProgress(80, "Time Sync");
    WatchdogHAL::getInstance().feed();
    refreshUI();

    // Even if offline, we apply cached offset/rules if available
    TimeService::getInstance().sync(_geo.getOffset());
    logToUI("Time: %s", TimeService::getInstance().getFormattedTime().c_str());

    // --- Step 7: Web Server ---
    if (online)
    {
        _console.setProgress(90, "Web Server");
        WatchdogHAL::getInstance().feed();
        refreshUI();

        WebService::getInstance().begin();
        logToUI("Web: %s", WebService::getInstance().getURL().c_str());
        logToUI("PIN: %s", WebService::getInstance().getPin().c_str());
    }

    // --- Step 8: Finalize ---
    _console.setProgress(100, "Ready");
    WatchdogHAL::getInstance().feed();
    logToUI("Boot Complete.");
    refreshUI();

    delay(1000); // Show 100% for a second

    // Cleanup to free RAM for Runtime
    if (_sprite)
    {
        _sprite->deleteSprite();
        delete _sprite;
        _sprite = nullptr;
    }
}

void BootOrchestrator::refreshUI()
{
    // Quick and dirty render loop for the blocking boot phase
    if (!_sprite)
        return;

    _console.draw(_sprite);
    _sprite->pushSprite(0, 0);
}

void BootOrchestrator::logToUI(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Logger::log("BOOT_UI", "%s", buf); // Also send to serial
    _console.appendLog(buf);
}
