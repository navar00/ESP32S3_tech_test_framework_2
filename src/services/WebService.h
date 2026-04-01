#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "../core/Logger.h"

/**
 * WebService — Singleton
 * Lightweight HTTP server on port 80.
 * Protected by a random 4-digit PIN generated at boot.
 * Serves system information once authenticated.
 */
class WebService
{
public:
    static WebService &getInstance()
    {
        static WebService instance;
        return instance;
    }

    /** Start the server (call once after WiFi is connected) */
    void begin();

    /** Must be called periodically (e.g. in loop()) to handle clients */
    void handleClient();

    /** Get the 4-digit access PIN */
    const String &getPin() const { return _pin; }

    /** Get the base URL */
    String getURL() const
    {
        return "http://" + WiFi.localIP().toString();
    }

private:
    WebService() {}
    WebServer _server{80};
    String _pin;
    String _sessionToken; // Changes every boot → invalidates old cookies
    const char *TAG = "WebSrv";

    void generatePin();
    void generateSessionToken();

    // --- Route Handlers ---
    void handleRoot();
    void handleLogin();
    void handleLogout();
    void handleDashboard();
    void handleFavicon();
    void handleNotFound();

    // --- GOL API Handlers ---
    void handleGOLConfig();
    void handleGOLAction();

    // --- Helpers ---
    bool isAuthenticated();
    String buildLoginPage(const char *errorMsg = nullptr);
    String buildDashboardPage();
};
