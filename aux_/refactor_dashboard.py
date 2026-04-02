"""Refactor WebService.cpp: replace monolithic dashboard with small HTML shell + AJAX."""
import re

with open('src/services/WebService.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# ──────────────────────────────────────────────────────────────
# 1. Replace handleDashboard() with PROGMEM static page + send_P
# ──────────────────────────────────────────────────────────────
old_dashboard = re.compile(
    r'void WebService::handleDashboard\(\)\s*\{.*?\n\}',
    re.DOTALL
)

new_dashboard = r'''// ─── Static Dashboard HTML (loaded via send_P, ~3 KB) ────────
static const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
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
.foot a{color:#999;text-decoration:none;border-bottom:1px solid #ddd}
.chip{display:inline-block;background:#e8f5e9;color:#2e7d32;font-size:11px;padding:2px 8px;border-radius:10px;font-weight:500}
.chip.off{background:#fbe9e7;color:#c62828}
.tabs{display:flex;margin-bottom:14px;border-bottom:2px solid #ddd}
.tab{flex:1;text-align:center;padding:10px;cursor:pointer;color:#888;font-weight:500;font-size:14px;border-bottom:2px solid transparent;transition:all .2s}
.tab.active{color:#4a90d9;border-bottom-color:#4a90d9;margin-bottom:-2px}
.tc{display:none}.tc.active{display:block}
.btn{padding:10px;border:none;border-radius:4px;cursor:pointer;font-weight:600}
.il{display:block;margin-bottom:5px;font-size:13px;color:#666}
</style></head><body>
<h1>ESP32 Control</h1>
<div class='tabs'>
<div class='tab active' onclick='sT("t_sys",this)'>System</div>
<div class='tab' onclick='sT("t_gol",this)'>Game of Life</div>
<div class='tab' onclick='sT("t_pal",this)'>Palettes</div>
</div>
<div id='t_sys' class='tc active'><p style='text-align:center;color:#aaa'>Loading&hellip;</p></div>
<div id='t_gol' class='tc'><p style='text-align:center;color:#aaa'>Loading&hellip;</p></div>
<div id='t_pal' class='tc'>
<div class='card'><h2>Palette Config</h2>
<div style='margin-bottom:15px'><label class='il'>Folder</label>
<select id='pF' onchange='uPL()' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'><option>Loading...</option></select></div>
<div style='margin-bottom:15px'><label class='il'>Palette</label>
<select id='pN' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'></select></div>
<button class='btn' style='background:#4a90d9;color:#fff;width:100%' onclick='sP()'>Apply Palette</button>
</div></div>
<p class='foot'>ESP32-S3 &middot; <a href='/logout'>Sign out</a></p>
<script>
function sT(id,el){document.querySelectorAll('.tc').forEach(e=>e.classList.remove('active'));document.querySelectorAll('.tab').forEach(e=>e.classList.remove('active'));var t=document.getElementById(id);if(t)t.classList.add('active');if(el)el.classList.add('active');}
function lS(){fetch('/api/system').then(r=>r.json()).then(d=>{var h='<div class="card"><h2>System</h2><table><tr><td>Uptime</td><td>'+d.up+'</td></tr><tr><td>CPU</td><td>'+d.cpu+' MHz</td></tr><tr><td>Temp</td><td>'+d.temp+' C</td></tr><tr><td>SDK</td><td>'+d.sdk+'</td></tr></table></div><div class="card"><h2>Memory</h2><table><tr><td>Free Heap</td><td>'+d.fh+' / '+d.th+' KB</td></tr><tr><td>Flash</td><td>'+d.fl+' MB</td></tr></table><div class="bar-track"><div class="bar-fill" style="width:'+d.hp+'%"></div></div></div><div class="card"><h2>Network</h2><table>';if(d.wf){h+='<tr><td>Status</td><td><span class="chip">Conn</span></td></tr><tr><td>SSID</td><td>'+d.ss+' ('+d.rs+'dBm)</td></tr><tr><td>IP</td><td>'+d.ip+'</td></tr><tr><td>MAC</td><td>'+d.mc+'</td></tr>';}else{h+='<tr><td>Status</td><td><span class="chip off">Off</span></td></tr>';}h+='</table></div>';document.getElementById('t_sys').innerHTML=h;}).catch(e=>{document.getElementById('t_sys').innerHTML='<p>Error: '+e.message+'</p>';});}
function lG(){fetch('/api/gol/state').then(r=>r.json()).then(d=>{var h='<div class="card"><h2>Screen Config</h2><div style="margin-bottom:15px"><label class="il">Speed (<span id="sv">'+d.sp+'</span>ms)</label><input type="range" id="spd" min="10" max="1000" value="'+d.sp+'" onchange="uG()" oninput="document.getElementById(\'sv\').innerText=this.value" style="width:100%"></div><div style="display:flex;gap:10px;margin-bottom:20px"><div style="flex:1"><label class="il">Alive</label><input type="color" id="ca" value="'+d.ac+'" onchange="uG()" style="width:100%;height:40px;border:none;padding:0"></div><div style="flex:1"><label class="il">Dead</label><input type="color" id="cd" value="'+d.dc+'" onchange="uG()" style="width:100%;height:40px;border:none;padding:0"></div></div><div style="display:flex;gap:10px"><button class="btn" style="background:#f0f0f0;color:#222;flex:1" onclick="aG(\'toggle\')">Play/Pause</button><button class="btn" style="background:#e3f2fd;color:#1565c0;flex:1" onclick="aG(\'reset\')">Random</button><button class="btn" style="background:#ffebee;color:#c62828;flex:1" onclick="aG(\'clear\')">Clear</button></div></div>';document.getElementById('t_gol').innerHTML=h;}).catch(e=>{document.getElementById('t_gol').innerHTML='<p>Error: '+e.message+'</p>';});}
function uG(){fetch('/api/gol/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'speed='+document.getElementById('spd').value+'&alive='+encodeURIComponent(document.getElementById('ca').value)+'&dead='+encodeURIComponent(document.getElementById('cd').value)});}
function aG(c){fetch('/api/gol/action',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+c});}
var PD={};function lP(){fetch('/api/palettes').then(r=>r.json()).then(d=>{PD=d;var f=document.getElementById('pF');f.innerHTML='';var k=Object.keys(d).sort();for(var i=0;i<k.length;i++)f.innerHTML+='<option value="'+k[i]+'">'+k[i]+'</option>';uPL();}).catch(e=>{document.getElementById('pF').innerHTML='<option>Error</option>';});}
function uPL(){var f=document.getElementById('pF');if(!f)return;var v=f.value,p=document.getElementById('pN');p.innerHTML='';if(PD[v]){var k=Object.keys(PD[v]).sort();for(var i=0;i<k.length;i++)p.innerHTML+='<option value="'+k[i]+'">'+k[i]+'</option>';}}
function sP(){var f=document.getElementById('pF').value,p=document.getElementById('pN').value;fetch('/api/palettes/set',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({folder:f,palette:p})});}
window.onload=function(){lS();lG();lP();};
</script></body></html>)rawhtml";

void WebService::handleDashboard()
{
    if (!isAuthenticated())
    {
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", "Auth required");
        return;
    }
    _server.send_P(200, "text/html", DASHBOARD_HTML, strlen_P(DASHBOARD_HTML));
}'''

code = old_dashboard.sub(new_dashboard, code, count=1)

# ──────────────────────────────────────────────────────────────
# 2. Remove the old buildDashboardPage() function entirely
# ──────────────────────────────────────────────────────────────
old_build = re.compile(
    r'String WebService::buildDashboardPage\(\)\s*\{.*',
    re.DOTALL
)
code = old_build.sub('', code)

# ──────────────────────────────────────────────────────────────
# 3. Add handleSystemInfo() and handleGOLStateGet() before the
#    Palettes API Handlers section
# ──────────────────────────────────────────────────────────────
new_handlers = r'''// ─── System API ──────────────────────────────────────────────

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
    doc["up"]   = upBuf;
    doc["cpu"]  = ESP.getCpuFreqMHz();
    doc["temp"] = String(temperatureRead(), 1);
    doc["sdk"]  = ESP.getSdkVersion();
    doc["fh"]   = fh;
    doc["th"]   = th;
    doc["fl"]   = ESP.getFlashChipSize() / (1024 * 1024);
    doc["hp"]   = hp;
    doc["wf"]   = (WiFi.status() == WL_CONNECTED);
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

// ─── Palettes API Handlers ───────────────────────────────────────────────────'''

code = code.replace(
    '// ─── Palettes API Handlers ───────────────────────────────────────────────────',
    new_handlers,
    1
)

# Clean up trailing whitespace at end of file
code = code.rstrip() + '\n'

with open('src/services/WebService.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print('Refactored WebService.cpp successfully')
