#include "WebService.h"
#include <esp_mac.h>
#include "../core/GOLConfig.h"

// ─── Lifecycle ───────────────────────────────────────────────

void WebService::begin()
{
    generatePin();
    generateSessionToken();
    Logger::log(TAG, "PIN: %s", _pin.c_str());
    Logger::log(TAG, "Session: %s", _sessionToken.c_str());

    // Register headers we want to read
    const char *headerKeys[] = {"Cookie"};
    _server.collectHeaders(headerKeys, 1);

    _server.on("/", HTTP_GET, [this]()
               { handleRoot(); });
    _server.on("/login", HTTP_POST, [this]()
               { handleLogin(); });
    _server.on("/logout", HTTP_GET, [this]()
               { handleLogout(); });
    _server.on("/dashboard", HTTP_GET, [this]()
               { handleDashboard(); });
    _server.on("/api/gol/config", HTTP_POST, [this]()
               { handleGOLConfig(); });
    _server.on("/api/gol/action", HTTP_POST, [this]()
               { handleGOLAction(); });
    _server.on("/favicon.ico", HTTP_GET, [this]()
               { handleFavicon(); });
    _server.onNotFound([this]()
                       { handleNotFound(); });

    _server.begin();
    Logger::log(TAG, "Server started on %s", getURL().c_str());
}

void WebService::handleClient()
{
    _server.handleClient();
}

// ─── PIN Generation ──────────────────────────────────────────

void WebService::generatePin()
{
    // Use hardware random number generator
    uint32_t r = esp_random();
    _pin = String(r % 10000);
    // Pad to 4 digits
    while (_pin.length() < 4)
        _pin = "0" + _pin;
}

void WebService::generateSessionToken()
{
    // Unique token per boot — old browser cookies become invalid
    uint32_t r = esp_random();
    char tok[9];
    snprintf(tok, sizeof(tok), "%08X", r);
    _sessionToken = String(tok);
}

// ─── Authentication ──────────────────────────────────────────

bool WebService::isAuthenticated()
{
    if (_server.hasHeader("Cookie"))
    {
        String cookie = _server.header("Cookie");
        // Cookie must contain the current session token (changes every boot)
        String expected = "auth=" + _sessionToken;
        if (cookie.indexOf(expected) >= 0)
            return true;
    }
    return false;
}

// ─── Route Handlers ──────────────────────────────────────────

void WebService::handleRoot()
{
    if (isAuthenticated())
    {
        _server.sendHeader("Location", "/dashboard");
        _server.send(302, "text/plain", "Redirecting...");
        return;
    }
    _server.send(200, "text/html", buildLoginPage());
}

void WebService::handleLogin()
{
    if (_server.hasArg("pin"))
    {
        String attempt = _server.arg("pin");
        if (attempt == _pin)
        {
            Logger::log(TAG, "Auth OK from %s", _server.client().remoteIP().toString().c_str());
            _server.sendHeader("Set-Cookie", "auth=" + _sessionToken + "; Path=/");
            _server.sendHeader("Location", "/dashboard");
            _server.send(302, "text/plain", "OK");
            return;
        }
        else
        {
            Logger::log(TAG, "Auth FAIL from %s", _server.client().remoteIP().toString().c_str());
            _server.send(200, "text/html", buildLoginPage("Wrong PIN. Try again."));
            return;
        }
    }
    _server.send(200, "text/html", buildLoginPage());
}

void WebService::handleDashboard()
{
    if (!isAuthenticated())
    {
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", "Auth required");
        return;
    }
    _server.send(200, "text/html", buildDashboardPage());
}

void WebService::handleLogout()
{
    Logger::log(TAG, "Logout from %s", _server.client().remoteIP().toString().c_str());
    // Clear auth cookie
    _server.sendHeader("Set-Cookie", "auth=; Path=/; Max-Age=0");
    _server.sendHeader("Location", "/");
    _server.send(302, "text/plain", "Logged out");
}

void WebService::handleFavicon()
{
    // Minimal 1x1 pixel green ICO (62 bytes)
    static const uint8_t ico[] = {
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00,
        0x18, 0x00, 0x30, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    _server.sendHeader("Cache-Control", "public, max-age=604800");
    _server.send_P(200, "image/x-icon", (const char *)ico, sizeof(ico));
}

void WebService::handleNotFound()
{
    String uri = _server.uri();
    String method = (_server.method() == HTTP_GET) ? "GET" : "POST";
    Logger::log(TAG, "404 %s %s from %s",
                method.c_str(), uri.c_str(),
                _server.client().remoteIP().toString().c_str());
    _server.send(404, "text/plain", "Not Found: " + uri);
}

// ─── GOL API Handlers ────────────────────────────────────────

void WebService::handleGOLConfig()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }
    if (_server.hasArg("speed") && _server.hasArg("alive") && _server.hasArg("dead"))
    {
        uint16_t speed = _server.arg("speed").toInt();
        String aliveHex = _server.arg("alive");
        String deadHex = _server.arg("dead");
        auto hexTo565 = [](String hex) -> uint16_t
        {
            if (hex.startsWith("#"))
                hex.remove(0, 1);
            long rgb = strtol(hex.c_str(), NULL, 16);
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        };
        GOLConfig::getInstance().updateConfig(hexTo565(aliveHex), hexTo565(deadHex), speed);
        Logger::log((char *)"WebSrv", "GOL Config: Spd %d", speed);
        _server.send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        _server.send(400, "application/json", "{\"error\":\"Bad request\"}");
    }
}

void WebService::handleGOLAction()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }
    if (_server.hasArg("cmd"))
    {
        String cmd = _server.arg("cmd");
        if (cmd == "toggle")
            GOLConfig::getInstance().setRunning(!GOLConfig::getInstance().getState().isRunning);
        else if (cmd == "reset")
            GOLConfig::getInstance().triggerAction(true, false);
        else if (cmd == "clear")
            GOLConfig::getInstance().triggerAction(false, true);
        Logger::log((char *)"WebSrv", "GOL Action %s", cmd.c_str());
        _server.send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        _server.send(400, "application/json", "{\"error\":\"Bad request\"}");
    }
}

// ─── HTML Builders ───────────────────────────────────────────

String WebService::buildLoginPage(const char *errorMsg)
{
    String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ESP32</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#f5f5f5;color:#222;font-family:-apple-system,system-ui,'Segoe UI',Roboto,sans-serif;
     display:flex;justify-content:center;align-items:center;height:100vh}
.card{background:#fff;border-radius:12px;padding:40px 36px;text-align:center;
      max-width:320px;width:100%;box-shadow:0 2px 12px rgba(0,0,0,.08)}
h2{font-size:18px;font-weight:600;margin-bottom:6px;color:#111}
.sub{font-size:13px;color:#888;margin-bottom:24px}
input[type=text]{border:1.5px solid #ddd;border-radius:8px;font-size:28px;
    text-align:center;letter-spacing:10px;padding:12px;width:160px;
    margin:0 auto 20px;display:block;color:#222;outline:none;transition:border .2s}
input[type=text]:focus{border-color:#4a90d9}
button{background:#222;color:#fff;border:none;border-radius:8px;
    padding:11px 36px;font-size:14px;font-weight:500;cursor:pointer;transition:background .2s}
button:hover{background:#444}
.err{color:#d44;font-size:13px;margin-bottom:14px}
.foot{color:#bbb;font-size:11px;margin-top:28px}
</style>
</head>
<body>
<div class='card'>
<h2>Device Access</h2>
<p class='sub'>Enter the 4-digit PIN shown on the display</p>
)rawhtml";

    if (errorMsg)
    {
        html += "<p class='err'>";
        html += errorMsg;
        html += "</p>";
    }

    html += R"rawhtml(
<form method='POST' action='/login'>
<input type='text' name='pin' maxlength='4' pattern='[0-9]{4}' inputmode='numeric' autofocus>
<button type='submit'>Unlock</button>
</form>
<p class='foot'>ESP32-S3</p>
</div>
</body>
</html>
)rawhtml";

    return html;
}

String WebService::buildDashboardPage()
{
    char buf[64];
    uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[18]; snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    auto state = GOLConfig::getInstance().getState();
    char hexA[8], hexD[8];
    uint8_t aR = (state.colorAlive >> 11) << 3; uint8_t aG = ((state.colorAlive >> 5) & 0x3F) << 2; uint8_t aB = (state.colorAlive & 0x1F) << 3;
    uint8_t dR = (state.colorDead >> 11) << 3; uint8_t dG = ((state.colorDead >> 5) & 0x3F) << 2; uint8_t dB = (state.colorDead & 0x1F) << 3;
    snprintf(hexA, sizeof(hexA), "#%02x%02x%02x", aR, aG, aB);
    snprintf(hexD, sizeof(hexD), "#%02x%02x%02x", dR, dG, dB);

    String html = R"(<!DOCTYPE html>
<html>
<head>
<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ESP32 Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#f5f5f5;color:#222;font-family:-apple-system,system-ui,'Segoe UI',Roboto,sans-serif;padding:24px;max-width:480px;margin:0 auto}
h1{font-size:20px;font-weight:600;text-align:center;margin-bottom:20px;color:#111}
.card{background:#fff;border-radius:10px;padding:16px 20px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.06)}
.card h2{font-size:11px;text-transform:uppercase;letter-spacing:1px;color:#999;margin-bottom:10px;font-weight:600}
table{width:100%;border-collapse:collapse}
td{padding:6px 0;font-size:14px;border-bottom:1px solid #f0f0f0}
tr:last-child td{border-bottom:none}
td:first-child{color:#888;width:40%}
td:last-child{color:#222;text-align:right;font-weight:500;font-variant-numeric:tabular-nums}
.bar-track{height:4px;background:#eee;border-radius:2px;margin-top:10px;overflow:hidden}
.bar-fill{height:100%;background:#4a90d9;border-radius:2px}
.foot{text-align:center;margin-top:20px;font-size:12px;color:#bbb}
.foot a{color:#999;text-decoration:none;border-bottom:1px solid #ddd;padding-bottom:1px}
.foot a:hover{color:#222;border-color:#222}
.chip{display:inline-block;background:#e8f5e9;color:#2e7d32;font-size:11px;padding:2px 8px;border-radius:10px;font-weight:500}
.chip.off{background:#fbe9e7;color:#c62828}
.tabs{display:flex;margin-bottom:14px;border-bottom:2px solid #ddd}
.tab{flex:1;text-align:center;padding:10px;cursor:pointer;color:#888;font-weight:500;font-size:14px;transition:all .2s;border-bottom:2px solid transparent}
.tab.active{color:#4a90d9;border-bottom-color:#4a90d9;margin-bottom:-2px}
.tab-ctx{display:none}
.tab-ctx.active{display:block}
.btn{flex:1;padding:10px;border:none;border-radius:4px;cursor:pointer;font-weight:600}
.inp-lbl{display:block;margin-bottom:5px;font-size:13px;color:#666}
</style>
<script>
function shTab(id, e){
    document.querySelectorAll('.tab-ctx').forEach(el=>el.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(el=>el.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    e.target.classList.add('active');
}
function upGol(){
    fetch('/api/gol/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'speed='+document.getElementById('spd').value+'&alive='+encodeURIComponent(document.getElementById('ca').value)+'&dead='+encodeURIComponent(document.getElementById('cd').value)});
}
function axGol(c){
    fetch('/api/gol/action',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+c});
}
function updVal(val){ document.getElementById("s_val").innerText = val; }
</script>
</head>
<body>
<h1>ESP32 Control</h1>
<div class='tabs'>
    <div class='tab active' onclick='shTab("t_sys", event)'>System</div>
    <div class='tab' onclick='shTab("t_gol", event)'>Game of Life</div>
</div>
<div id='t_sys' class='tab-ctx active'>
)";

    html += "<div class='card'><h2>System</h2><table>";
    unsigned long up = millis() / 1000;
    snprintf(buf, sizeof(buf), "%luh %lum %lus", up / 3600, (up % 3600) / 60, up % 60);
    html += "<tr><td>Uptime</td><td>" + String(buf) + "</td></tr>";
    html += "<tr><td>CPU</td><td>" + String(ESP.getCpuFreqMHz()) + " MHz</td></tr>";
    html += "<tr><td>Temperature</td><td>" + String(temperatureRead(), 1) + " &deg;C</td></tr>";
    html += "<tr><td>SDK</td><td>" + String(ESP.getSdkVersion()) + "</td></tr></table></div>";

    html += "<div class='card'><h2>Memory</h2><table>";
    uint32_t fh = ESP.getFreeHeap(); uint32_t th = ESP.getHeapSize();
    html += "<tr><td>Free Heap</td><td>" + String(fh/1024) + " / " + String(th/1024) + " KB</td></tr>";
    html += "<tr><td>Flash</td><td>" + String(ESP.getFlashChipSize() / (1024*1024)) + " MB</td></tr>";
    html += "</table><div class='bar-track'><div class='bar-fill' style='width:" + String((fh*100)/th) + "%'></div></div></div>";

    html += "<div class='card'><h2>Network</h2><table>";
    if(WiFi.status() == WL_CONNECTED){
        html += "<tr><td>Status</td><td><span class='chip'>Conn</span></td></tr>";
        html += "<tr><td>SSID</td><td>" + WiFi.SSID() + " (" + String(WiFi.RSSI()) + "dBm)</td></tr>";
        html += "<tr><td>IP</td><td>" + WiFi.localIP().toString() + "</td></tr>";
        html += "<tr><td>MAC</td><td>" + String(macStr) + "</td></tr>";
    }else{
         html += "<tr><td>Status</td><td><span class='chip off'>Offline</span></td></tr>";
    }
    html += "</table></div></div>";

    html += "<div id='t_gol' class='tab-ctx'><div class='card'><h2>Screen Config</h2>";
    html += "<div style='margin-bottom:15px'><label class='inp-lbl'>Speed (<span id='s_val'>" + String(state.cycleSpeedMs) + "</span>ms)</label>";
    html += "<input type='range' id='spd' min='10' max='1000' value='" + String(state.cycleSpeedMs) + "' onchange='upGol()' oninput='updVal(this.value)' style='width:100%'></div>";
    
    html += "<div style='display:flex;gap:10px;margin-bottom:20px'>";
    html += "<div style='flex:1'><label class='inp-lbl'>Alive Color</label><input type='color' id='ca' value='" + String(hexA) + "' onchange='upGol()' style='width:100%;height:40px;border:none;padding:0'></div>";
    html += "<div style='flex:1'><label class='inp-lbl'>Dead Color</label><input type='color' id='cd' value='" + String(hexD) + "' onchange='upGol()' style='width:100%;height:40px;border:none;padding:0'></div></div>";
    
    html += "<div style='display:flex;gap:10px'>";
    html += "<button class='btn' style='background:#f0f0f0;color:#222' onclick='axGol(\"toggle\")'>Play/Pause</button>";
    html += "<button class='btn' style='background:#e3f2fd;color:#1565c0' onclick='axGol(\"reset\")'>Random</button>";
    html += "<button class='btn' style='background:#ffebee;color:#c62828' onclick='axGol(\"clear\")'>Clear</button>";
    html += "</div></div></div>";

    html += R"(<p class='foot'>ESP32-S3 &middot; <a href='/logout'>Sign out</a></p></body></html>)";

    return html;
}