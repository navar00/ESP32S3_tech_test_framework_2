#include "WebService.h"
#include <esp_mac.h>
#include <ArduinoJson.h>
#include "../core/GOLConfig.h"
#include "../core/ScreenManager.h"
#include "../screens/ScreenPalette.h"
#include "../core/PalettesData.h"

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
    _server.on("/d.js", HTTP_GET, [this]()
               { handleDashboardJS(); });
    _server.on("/api/system", HTTP_GET, [this]()
               { handleSystemInfo(); });
    _server.on("/api/gol/state", HTTP_GET, [this]()
               { handleGOLStateGet(); });
    _server.on("/api/gol/config", HTTP_POST, [this]()
               { handleGOLConfig(); });
    _server.on("/api/gol/action", HTTP_POST, [this]()
               { handleGOLAction(); });
    _server.on("/api/palettes", HTTP_GET, [this]()
               { handlePalettesGet(); });
    _server.on("/api/palettes/set", HTTP_POST, [this]()
               { handlePaletteSet(); });
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

// ─── Dashboard HTML (~3.2 KB, well under TCP window) ─────────
static const char DASH_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ESP32 Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#f5f5f5;color:#222;font-family:-apple-system,system-ui,'Segoe UI',Roboto,sans-serif;padding:24px;max-width:480px;margin:0 auto}
h1{font-size:20px;font-weight:600;text-align:center;margin-bottom:20px;color:#111}
.c{background:#fff;border-radius:10px;padding:16px 20px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.06)}
.c h2{font-size:11px;text-transform:uppercase;letter-spacing:1px;color:#999;margin-bottom:10px;font-weight:600}
table{width:100%;border-collapse:collapse}
td{padding:6px 0;font-size:14px;border-bottom:1px solid #f0f0f0}
tr:last-child td{border-bottom:none}
td:first-child{color:#888;width:40%}
td:last-child{color:#222;text-align:right;font-weight:500}
.bt{height:4px;background:#eee;border-radius:2px;margin-top:10px;overflow:hidden}
.bf{height:100%;background:#4a90d9;border-radius:2px}
.ft{text-align:center;margin-top:20px;font-size:12px;color:#bbb}
.ft a{color:#999;text-decoration:none}
.ch{display:inline-block;background:#e8f5e9;color:#2e7d32;font-size:11px;padding:2px 8px;border-radius:10px;font-weight:500}
.ch.off{background:#fbe9e7;color:#c62828}
.tabs{display:flex;margin-bottom:14px;border-bottom:2px solid #ddd}
.tab{flex:1;text-align:center;padding:10px;cursor:pointer;color:#888;font-weight:500;font-size:14px;border-bottom:2px solid transparent}
.tab.active{color:#4a90d9;border-bottom-color:#4a90d9;margin-bottom:-2px}
.tc{display:none}.tc.active{display:block}
.btn{padding:10px;border:none;border-radius:4px;cursor:pointer;font-weight:600}
.il{display:block;margin-bottom:5px;font-size:13px;color:#666}
.sw{display:flex;align-items:center;margin-bottom:6px}.sb{height:28px;border-radius:4px;min-width:20px}.sl{font-size:11px;color:#888;margin-left:8px;white-space:nowrap}.sh{font-size:11px;color:#555;font-family:monospace;margin-left:8px}
</style></head><body>
<h1>ESP32 Control</h1>
<div class='tabs'>
<div class='tab active' onclick='sT("t_sys",this)'>System</div>
<div class='tab' onclick='sT("t_gol",this)'>Game of Life</div>
<div class='tab' onclick='sT("t_pal",this)'>Palettes</div>
</div>
<div id='t_sys' class='tc active'><p style='text-align:center;color:#aaa'>Loading&#8230;</p></div>
<div id='t_gol' class='tc'><p style='text-align:center;color:#aaa'>Loading&#8230;</p></div>
<div id='t_pal' class='tc'>
<div class='c'><h2>Palette Config</h2>
<div style='margin-bottom:15px'><label class='il'>Folder</label>
<select id='pF' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'><option>Loading...</option></select></div>
<div style='margin-bottom:15px'><label class='il'>Palette</label>
<select id='pN' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'></select></div>
<button class='btn' style='background:#4a90d9;color:#fff;width:100%' onclick='sP()'>Apply Palette</button>
</div>
<div class='c' id='pPrev'><h2>Preview</h2><p style='color:#aaa;font-size:13px'>Select a palette to preview colors</p></div></div>
<p class='ft'>ESP32-S3 &middot; <a href='/logout'>Sign out</a></p>
<script src='/d.js'></script>
</body></html>)rawhtml";

// ─── Dashboard JS (~4.8 KB, sequential fetch + palette preview) ─────
static const char DASH_JS[] PROGMEM = R"rawjs(function sT(id,el){document.querySelectorAll('.tc').forEach(function(e){e.classList.remove('active')});document.querySelectorAll('.tab').forEach(function(e){e.classList.remove('active')});var t=document.getElementById(id);if(t)t.classList.add('active');if(el)el.classList.add('active')}
function lS(){return fetch('/api/system').then(function(r){return r.json()}).then(function(d){var h='<div class="c"><h2>System</h2><table>';h+='<tr><td>Uptime</td><td>'+d.up+'</td></tr>';h+='<tr><td>CPU</td><td>'+d.cpu+' MHz</td></tr>';h+='<tr><td>Temp</td><td>'+d.temp+' C</td></tr>';h+='<tr><td>SDK</td><td>'+d.sdk+'</td></tr>';h+='</table></div>';h+='<div class="c"><h2>Memory</h2><table>';h+='<tr><td>Heap</td><td>'+d.fh+'/'+d.th+' KB</td></tr>';h+='<tr><td>Flash</td><td>'+d.fl+' MB</td></tr>';h+='</table><div class="bt"><div class="bf" style="width:'+d.hp+'%"></div></div></div>';h+='<div class="c"><h2>Network</h2><table>';if(d.wf){h+='<tr><td>Status</td><td><span class="ch">Conn</span></td></tr>';h+='<tr><td>SSID</td><td>'+d.ss+' ('+d.rs+'dBm)</td></tr>';h+='<tr><td>IP</td><td>'+d.ip+'</td></tr>';h+='<tr><td>MAC</td><td>'+d.mc+'</td></tr>'}else{h+='<tr><td>Status</td><td><span class="ch off">Off</span></td></tr>'}h+='</table></div>';document.getElementById('t_sys').innerHTML=h}).catch(function(e){document.getElementById('t_sys').innerHTML='<p>Error: '+e.message+'</p>'})}
function lG(){return fetch('/api/gol/state').then(function(r){return r.json()}).then(function(d){var h='<div class="c"><h2>Screen Config</h2>';h+='<div style="margin-bottom:15px"><label class="il">Speed (<span id="sv">'+d.sp+'</span>ms)</label>';h+='<input type="range" id="spd" min="10" max="1000" value="'+d.sp+'" onchange="uG()" oninput="document.getElementById(\'sv\').innerText=this.value" style="width:100%"></div>';h+='<div style="display:flex;gap:10px;margin-bottom:20px">';h+='<div style="flex:1"><label class="il">Alive</label><input type="color" id="ca" value="'+d.ac+'" onchange="uG()" style="width:100%;height:40px;border:none;padding:0"></div>';h+='<div style="flex:1"><label class="il">Dead</label><input type="color" id="cd" value="'+d.dc+'" onchange="uG()" style="width:100%;height:40px;border:none;padding:0"></div></div>';h+='<div style="display:flex;gap:10px">';h+='<button class="btn" style="background:#f0f0f0;color:#222;flex:1" onclick="aG(\'toggle\')">Play/Pause</button>';h+='<button class="btn" style="background:#e3f2fd;color:#1565c0;flex:1" onclick="aG(\'reset\')">Random</button>';h+='<button class="btn" style="background:#ffebee;color:#c62828;flex:1" onclick="aG(\'clear\')">Clear</button>';h+='</div></div>';document.getElementById('t_gol').innerHTML=h}).catch(function(e){document.getElementById('t_gol').innerHTML='<p>Error: '+e.message+'</p>'})}
function uG(){fetch('/api/gol/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'speed='+document.getElementById('spd').value+'&alive='+encodeURIComponent(document.getElementById('ca').value)+'&dead='+encodeURIComponent(document.getElementById('cd').value)})}
function aG(c){fetch('/api/gol/action',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+c})}
var PD={};function lP(){return fetch('/api/palettes').then(function(r){return r.json()}).then(function(d){PD=d;var f=document.getElementById('pF');f.innerHTML='';var k=Object.keys(d).sort();for(var i=0;i<k.length;i++){f.innerHTML+='<option value="'+k[i]+'">'+k[i]+'</option>'}uPL()}).catch(function(e){document.getElementById('pF').innerHTML='<option>Error</option>'})}
function uPL(){var f=document.getElementById('pF');if(!f)return;var v=f.value,p=document.getElementById('pN');p.innerHTML='';if(PD[v]){var k=Object.keys(PD[v]).sort();for(var i=0;i<k.length;i++){p.innerHTML+='<option value="'+k[i]+'">'+k[i]+'</option>'}}shPv()}
function shPv(){var fv=document.getElementById('pF').value,pv=document.getElementById('pN').value,box=document.getElementById('pPrev');if(!PD[fv]||!PD[fv][pv]){box.innerHTML='<h2>Preview</h2><p style="color:#aaa;font-size:13px">No data</p>';return}var cl=PD[fv][pv],tot=0;for(var i=0;i<cl.length;i++)tot+=cl[i].frequency;var h='<h2>Preview &middot; '+cl.length+' colors</h2>';for(var i=0;i<cl.length;i++){var c=cl[i],pc=tot>0?(c.frequency*100/tot).toFixed(1):'?';h+='<div class="sw"><div class="sb" style="background:'+c.hex+';width:'+Math.max(pc*2.5,8)+'%"></div><span class="sh">'+c.hex+'</span><span class="sl">'+pc+'%</span></div>'}box.innerHTML=h}
function sP(){fetch('/api/palettes/set',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({folder:document.getElementById('pF').value,palette:document.getElementById('pN').value})})}
document.getElementById('pF').onchange=uPL;document.getElementById('pN').onchange=shPv;
lS().then(function(){return lG()}).then(function(){return lP()});)rawjs";

void WebService::handleDashboard()
{
    if (!isAuthenticated())
    {
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", "Auth required");
        return;
    }
    _server.send_P(200, "text/html", DASH_HTML, strlen_P(DASH_HTML));
}

void WebService::handleDashboardJS()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send_P(200, "application/javascript", DASH_JS, strlen_P(DASH_JS));
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
// ─── System API ──────────────────────────────────────────────

void WebService::handleSystemInfo()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }

    unsigned long up = millis() / 1000;
    char upBuf[32];
    snprintf(upBuf, sizeof(upBuf), "%luh %lum %lus", up / 3600, (up % 3600) / 60, up % 60);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    uint32_t fh = ESP.getFreeHeap() / 1024;
    uint32_t th = ESP.getHeapSize() / 1024;
    uint32_t hp = (ESP.getFreeHeap() * 100) / ESP.getHeapSize();

    DynamicJsonDocument doc(384);
    doc["up"] = upBuf;
    doc["cpu"] = ESP.getCpuFreqMHz();
    doc["temp"] = String(temperatureRead(), 1);
    doc["sdk"] = ESP.getSdkVersion();
    doc["fh"] = fh;
    doc["th"] = th;
    doc["fl"] = ESP.getFlashChipSize() / (1024 * 1024);
    doc["hp"] = hp;
    doc["wf"] = (WiFi.status() == WL_CONNECTED);
    if (WiFi.status() == WL_CONNECTED)
    {
        doc["ss"] = WiFi.SSID();
        doc["rs"] = WiFi.RSSI();
        doc["ip"] = WiFi.localIP().toString();
        doc["mc"] = macStr;
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebService::handleGOLStateGet()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }

    auto state = GOLConfig::getInstance().getState();

    uint8_t aR = (state.colorAlive >> 11) << 3;
    uint8_t aG = ((state.colorAlive >> 5) & 0x3F) << 2;
    uint8_t aB = (state.colorAlive & 0x1F) << 3;
    uint8_t dR = (state.colorDead >> 11) << 3;
    uint8_t dG = ((state.colorDead >> 5) & 0x3F) << 2;
    uint8_t dB = (state.colorDead & 0x1F) << 3;

    char hexA[8], hexD[8];
    snprintf(hexA, sizeof(hexA), "#%02x%02x%02x", aR, aG, aB);
    snprintf(hexD, sizeof(hexD), "#%02x%02x%02x", dR, dG, dB);

    DynamicJsonDocument doc(128);
    doc["sp"] = state.cycleSpeedMs;
    doc["ac"] = hexA;
    doc["dc"] = hexD;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

// ─── Palettes API Handlers ───────────────────────────────────────────────────

void WebService::handlePalettesGet()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }

    size_t len = strlen_P(PALETTES_JSON);

    // Send headers with exact Content-Length, then stream body
    _server.setContentLength(len);
    _server.sendHeader("Connection", "close");
    _server.send(200, "application/json", "");

    WiFiClient client = _server.client();
    const size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    size_t pos = 0;

    while (pos < len && client.connected())
    {
        size_t toSend = ((len - pos) < BUF_SZ) ? (len - pos) : BUF_SZ;
        memcpy_P(buf, PALETTES_JSON + pos, toSend);

        size_t sent = 0;
        uint8_t retries = 0;
        while (sent < toSend && client.connected() && retries < 50)
        {
            int w = client.write((const uint8_t *)(buf + sent), toSend - sent);
            if (w > 0)
            {
                sent += w;
                retries = 0;
            }
            else
            {
                retries++;
                delay(5);
            }
        }
        pos += sent;
        yield();
    }
}

void WebService::handlePaletteSet()
{
    if (!isAuthenticated())
    {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }
    if (_server.hasArg("plain"))
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, _server.arg("plain"));
        String folder = doc["folder"];
        String palette = doc["palette"];

        ScreenPalette *sp = (ScreenPalette *)ScreenManager::getInstance().getScreenByName("Palette");
        if (sp)
        {
            sp->setPalette(folder, palette);
            _server.send(200, "application/json", "{\"success\":true}");
            return;
        }
    }
    _server.send(400, "application/json", "{\"error\":\"Bad request\"}");
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
