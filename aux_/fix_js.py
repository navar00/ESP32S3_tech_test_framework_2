import re

with open('src/services/WebService.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

# Make sure we don't accidentally match the wrong part.
script = '''<script>
  let palData = {};
  function initPal() {
      console.log('Fetching palettes...');
      fetch('/api/palettes')
          .then(async r => {
              if(!r.ok) { let err = await r.text(); throw new Error('HTTP ' + r.status + ' - ' + err); }
              return r.json();
          })
          .then(d => {
              console.log('Palettes loaded:', Object.keys(d));
              palData = d;
              let f = document.getElementById('pFlder');
              f.innerHTML = '';
              let keys = Object.keys(d).sort();
              if(keys.length === 0) {
                 f.innerHTML = '<option>No folders</option>';
              } else {
                 for(let i=0; i<keys.length; i++) {
                     f.innerHTML += '<option value=\"' + keys[i] + '\">' + keys[i] + '</option>';
                 }
              }
              updPalList();
          })
          .catch(e => {
              console.error('initPal error:', e);
              document.getElementById('pFlder').innerHTML = '<option value=\"\">Error: ' + e.message + '</option>';
          });
  }
  function updPalList() {
      let f = document.getElementById('pFlder');
      if(!f) return;
      let fv = f.value;
      let p = document.getElementById('pName');
      p.innerHTML = '';
      if(palData[fv]){
          let keys = Object.keys(palData[fv]).sort();
          for(let i=0; i<keys.length; i++) {
              p.innerHTML += '<option value=\"' + keys[i] + '\">' + keys[i] + '</option>';
          }
      }
  }
  function setPal() {
      let f = document.getElementById('pFlder').value;
      let p = document.getElementById('pName').value;
      console.log('Setting palette:', f, p);
      fetch('/api/palettes/set',{
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({folder:f, palette:p})
      }).then(r => {
          if(r.ok) console.log('Palette applied successfully');
          else r.text().then(t => console.error('Failed to apply:', r.status, t));
      }).catch(e => console.error('Error applying palette:', e));
  }
  window.onload = initPal;

  function shTab(id, e){
      console.log('Switching to tab:', id);
      try {
          document.querySelectorAll('.tab-ctx').forEach(el=>el.classList.remove('active'));
          document.querySelectorAll('.tab').forEach(el=>el.classList.remove('active'));
          let tabCtx = document.getElementById(id);
          if (tabCtx) {
              tabCtx.classList.add('active');
          } else {
              console.log('Tab context not found for ID:', id);
          }
          if (e && e.target) {
              e.target.classList.add('active');
          }
      } catch(err) {
          console.error('Tab switch error:', err);
      }
  }
  function upGol(){
      fetch('/api/gol/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'speed='+document.getElementById('spd').value+'&alive='+encodeURIComponent(document.getElementById('ca').value)+'&dead='+encodeURIComponent(document.getElementById('cd').value)});
  }
  function axGol(c){
      fetch('/api/gol/action',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+c});
  }
  function updVal(val){ document.getElementById('s_val').innerText = val; }
  
  document.addEventListener('DOMContentLoaded', () => {
       document.getElementById('pFlder').addEventListener('change', updPalList);
  });
  </script>'''

new_text = re.sub(r'<script>.*?</script>', script, text, flags=re.DOTALL)

with open('src/services/WebService.cpp', 'w', encoding='utf-8') as f:
    f.write(new_text)

print('JavaScript replacement complete.')