import sys, os

file_path = "c:\\Users\\egavi\\OneDrive\\Documents\\PlatformIO\\Projects\\ESP32S3_tech_test_framework_2\\src\\services\\WebService.cpp"

if not os.path.exists(file_path):
    print("Could not find", file_path)
    sys.exit(1)

with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# Replace Tab
old_tab = """<div class="tab" onclick="shTab('t_gol', event)">Game of Life</div>"""
new_tab = """<div class="tab" onclick="shTab('t_gol', event)">Game of Life</div>
        <div class="tab" onclick="shTab('t_pal', event)">Palettes</div>"""

if old_tab in content:
    content = content.replace(old_tab, new_tab)
    print("Injected tab.")
else:
    print("Could not find tab injection point.")

# Replace JS
old_js = """function shTab(id, e){"""
new_js = """
      let palData = {};
      function initPal(){
        fetch('/api/palettes').then(r=>r.json()).then(d=>{
          palData=d;
          let f = document.getElementById('pFlder');
          f.innerHTML = '';
          for(let k in d) f.innerHTML += `<option value="${k}">${k}</option>`;
          updPalList();
        });
      }
      function updPalList(){
        let f = document.getElementById('pFlder').value;
        let p = document.getElementById('pName');
        p.innerHTML = '';
        if(palData[f]){
          for(let k in palData[f]) p.innerHTML += `<option value="${k}">${k}</option>`;
        }
      }
      function setPal(){
        let f = document.getElementById('pFlder').value;
        let p = document.getElementById('pName').value;
        fetch('/api/palettes/set',{
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({folder:f, palette:p})
        });
      }
      window.onload = initPal;
      function shTab(id, e){
"""

if old_js in content:
    content = content.replace(old_js, new_js)
    print("Injected JS.")
else:
    print("Could not find JS injection point.")

# Replace HTML Bottom
old_html = '''      html += "</div></div></div>";\n\n      html += R"(<p class='foot'>ESP32-S3 &middot; <a href='/logout'>Sign out</a></p></body></html>)";'''
new_html = '''      html += "</div></div></div>";\n
      html += "<div id='t_pal' class='tab-ctx'><div class='card'><h2>Palette Config</h2>";
      html += "<div style='margin-bottom:15px'><label class='inp-lbl'>Folder</label>";
      html += "<select id='pFlder' onchange='updPalList()' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'></select></div>";
      html += "<div style='margin-bottom:15px'><label class='inp-lbl'>Palette</label>";
      html += "<select id='pName' style='width:100%;padding:8px;border-radius:4px;border:1px solid #ccc'></select></div>";
      html += "<button class='btn' style='background:#4a90d9;color:#fff;width:100%' onclick='setPal()'>Apply Palette</button>";
      html += "</div></div>";\n
      html += R"(<p class='foot'>ESP32-S3 &middot; <a href='/logout'>Sign out</a></p></body></html>)";'''

if old_html in content:
    content = content.replace(old_html, new_html)
    print("Injected HTML.")
else:
    print("Could not find HTML injection point. Saving anyway.")

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)
print("Done patching.")
