#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BeCa Control v2</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
    .container { max-width: 500px; margin: 0 auto; padding: 16px; }
    h1 { text-align: center; color: #38bdf8; margin: 10px 0; }
    .tabs { display: flex; gap: 10px; margin-bottom: 15px; }
    .tab { flex: 1; padding: 10px; text-align: center; background: #334155; border-radius: 8px; cursor: pointer; font-weight: bold; }
    .tab.active { background: #3b82f6; }
    .panel { display: none; }
    .panel.active { display: block; }
    .card { background: #1e293b; border-radius: 12px; padding: 15px; margin-bottom: 15px; }
    .card-title { color: #94a3b8; font-size: 0.9em; text-transform: uppercase; margin-bottom: 10px; border-bottom: 1px solid #334155; padding-bottom: 5px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .box { background: #334155; padding: 10px; border-radius: 8px; text-align: center; }
    .val { font-size: 1.4em; font-weight: bold; }
    .lbl { font-size: 0.75em; color: #94a3b8; }
    .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
    .btn { padding: 8px 15px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; color: white; }
    .btn.on { background: #22c55e; }
    .btn.off { background: #475569; }
    .btn.feed { background: #3b82f6; width: 100%; padding: 12px; margin-top: 10px; }
    .form-group { margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; }
    .form-group input { width: 80px; padding: 5px; background: #0f172a; border: 1px solid #475569; color: white; border-radius: 4px; text-align: center; }
    .form-group select { padding: 5px; background: #0f172a; border: 1px solid #475569; color: white; border-radius: 4px; }
    .save-btn { width: 100%; background: #8b5cf6; color: white; padding: 12px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; margin-top: 10px; }
    .warn { color: #f59e0b; }
    .err { color: #ef4444; }
  </style>
</head>
<body>
  <div class="container">
    <h1>BE CA THONG MINH</h1>
    
    <div class="tabs">
      <div class="tab active" onclick="switchTab('dash')">DASHBOARD</div>
      <div class="tab" onclick="switchTab('settings')">CAI DAT</div>
    </div>

    <!-- DASHBOARD PANEL -->
    <div id="dash" class="panel active">
      <div class="card">
        <div class="card-title">Moi Truong</div>
        <div class="grid">
          <div class="box"><div class="val" id="v-wt">--</div><div class="lbl">Nuoc (°C)</div></div>
          <div class="box"><div class="val" id="v-at">--</div><div class="lbl">K.Khi (°C)</div></div>
          <div class="box"><div class="val" id="v-wl">--</div><div class="lbl">Muc Nuoc</div></div>
          <div class="box"><div class="val" id="v-time">--:--</div><div class="lbl">Gio He Thong</div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-title">Dieu Khien</div>
        <div class="row"><span>Suoi</span><button class="btn off" id="b-heater" onclick="t('heater')">TAT</button></div>
        <div class="row"><span>Quat</span><button class="btn off" id="b-fan" onclick="t('fan')">TAT</button></div>
        <div class="row"><span>Bom bu</span><button class="btn off" id="b-pump" onclick="t('pump')">TAT</button></div>
        <div class="row"><span>Suc Oxy (<span id="oxy-mode-lbl"></span>)</span><button class="btn off" id="b-oxy" onclick="t('oxy')">TAT</button></div>
        <div class="row"><span>Bom thay</span><button class="btn off" id="b-drain" onclick="t('drain')">TAT</button></div>
        <div class="row"><span>Den LED</span><button class="btn off" id="b-led" onclick="t('led')">TAT</button></div>
        <button class="btn feed" onclick="t('feed')">CHO AN NGAY</button>
      </div>
    </div>

    <!-- SETTINGS PANEL -->
    <div id="settings" class="panel">
      <div class="card">
        <div class="card-title">Nguong Nhiet do</div>
        <div class="form-group"><span>Bat Suoi (<)</span><input type="number" id="s-ho" step="0.5"></div>
        <div class="form-group"><span>Tat Suoi (>=)</span><input type="number" id="s-hf" step="0.5"></div>
        <div class="form-group"><span>Bat Quat (>)</span><input type="number" id="s-fo" step="0.5"></div>
        <div class="form-group"><span>Tat Quat (<=)</span><input type="number" id="s-ff" step="0.5"></div>
      </div>
      
      <div class="card">
        <div class="card-title">Tu Dong Nuoc</div>
        <div class="form-group"><span>Bom bu khi can</span><select id="s-ap"><option value="1">BAT</option><option value="0">TAT</option></select></div>
        <div class="form-group"><span>Bom thay khi can</span><select id="s-ad"><option value="1">BAT</option><option value="0">TAT</option></select></div>
      </div>

      <div class="card">
        <div class="card-title">Suc Oxy & Den LED</div>
        <div class="form-group"><span>Che do Oxy</span><select id="s-om"><option value="0">Chu ky (5/15)</option><option value="1">Lien tuc</option></select></div>
        <div class="form-group"><span>Den Hen gio</span><select id="s-lm"><option value="1">BAT</option><option value="0">TAT</option></select></div>
        <div class="form-group"><span>Den Bat luc</span><input type="time" id="s-lon"></div>
        <div class="form-group"><span>Den Tat luc</span><input type="time" id="s-loff"></div>
      </div>

      <div class="card">
        <div class="card-title">Timer Tu Tat (Phut) - 0 = Khong dung</div>
        <div class="form-group"><span>Bom thay</span><input type="number" id="t-drain"></div>
        <div class="form-group"><span>Suoi</span><input type="number" id="t-heater"></div>
        <div class="form-group"><span>Quat</span><input type="number" id="t-fan"></div>
      </div>

      <div class="card">
        <div class="card-title">Kết Nối WiFi & Camera</div>
        <div class="form-group"><span>WiFi:</span><input type="text" id="s-ssid" style="width:150px;"></div>
        <div class="form-group"><span>Pass:</span><input type="text" id="s-pass" style="width:150px;"></div>
        <div class="form-group"><span>Cam:</span><input type="text" id="s-cam" style="width:150px;"></div>
        <div class="form-group"><span>Trạng thái:</span><span id="s-wst" style="font-weight:bold; font-size:0.85em;">--</span></div>
      </div>
      
      <div class="card">
        <div class="card-title">Mã IR Remote (Hex)</div>
        <div class="row">
          <span>S: <input type="text" id="s-ir1" style="width:30px;"></span>
          <span>Q: <input type="text" id="s-ir2" style="width:30px;"></span>
          <span>B: <input type="text" id="s-ir3" style="width:30px;"></span>
          <span>O: <input type="text" id="s-ir4" style="width:30px;"></span>
        </div>
        <div class="row">
          <span>T: <input type="text" id="s-ir5" style="width:30px;"></span>
          <span>L: <input type="text" id="s-ir6" style="width:30px;"></span>
          <span>A: <input type="text" id="s-ir7" style="width:30px;"></span>
          <span>X: <input type="text" id="s-ir0" style="width:30px;"></span>
        </div>
      </div>

      <button class="save-btn" onclick="saveSettings()">LUU CAI DAT</button>
    </div>
  </div>

  <script>
    function switchTab(t) {
      document.querySelectorAll('.tab').forEach(e=>e.classList.remove('active'));
      document.querySelectorAll('.panel').forEach(e=>e.classList.remove('active'));
      event.target.classList.add('active');
      document.getElementById(t).classList.add('active');
    }

    function f() {
      fetch('/api/data').then(r=>r.json()).then(d=>{
        // Dash
        document.getElementById('v-wt').textContent = d.wt.toFixed(1);
        document.getElementById('v-at').textContent = d.at.toFixed(1);
        document.getElementById('v-wl').textContent = (d.wcm > 0) ? d.wcm.toFixed(1) + ' cm' : 'Loi';
        document.getElementById('v-time').textContent = d.time;
        document.getElementById('oxy-mode-lbl').textContent = d.om ? 'Lien tuc' : 'Chu ky';

        ub('b-heater', d.h); ub('b-fan', d.f); ub('b-pump', d.p);
        ub('b-oxy', d.o); ub('b-drain', d.d); ub('b-led', d.l);

        // Settings (only update if not focused)
        let wstEl = document.getElementById('s-wst');
        if (d.wst == 2) {
          wstEl.textContent = 'Da ket noi (' + d.wip + ')';
          wstEl.style.color = '#22c55e';
        } else if (d.wst == 1) {
          wstEl.textContent = 'Dang ket noi... (toi da 30s)';
          wstEl.style.color = '#f59e0b';
        } else {
          wstEl.textContent = 'Chua ket noi (AP Mode)';
          wstEl.style.color = '#94a3b8';
        }

        if(!document.querySelector('input:focus')) {
          document.getElementById('s-ho').value = d.sh_on;
          document.getElementById('s-hf').value = d.sh_off;
          document.getElementById('s-fo').value = d.sf_on;
          document.getElementById('s-ff').value = d.sf_off;
          document.getElementById('s-ap').value = d.sap ? 1:0;
          document.getElementById('s-ad').value = d.sad ? 1:0;
          document.getElementById('s-om').value = d.om ? 1:0;
          document.getElementById('s-lm').value = d.slm ? 1:0;
          document.getElementById('s-lon').value = d.sl_on;
          document.getElementById('s-loff').value = d.sl_off;
          document.getElementById('t-drain').value = d.td;
          document.getElementById('t-heater').value = d.th;
          document.getElementById('t-fan').value = d.tf;
          document.getElementById('s-ssid').value = d.ssid;
          document.getElementById('s-pass').value = d.pass;
          document.getElementById('s-cam').value = d.cam;
          document.getElementById('s-ir1').value = d.ir1; document.getElementById('s-ir2').value = d.ir2;
          document.getElementById('s-ir3').value = d.ir3; document.getElementById('s-ir4').value = d.ir4;
          document.getElementById('s-ir5').value = d.ir5; document.getElementById('s-ir6').value = d.ir6;
          document.getElementById('s-ir7').value = d.ir7; document.getElementById('s-ir0').value = d.ir0;
        }
      });
    }

    function ub(id, s) {
      let b = document.getElementById(id);
      b.textContent = s ? 'BAT' : 'TAT';
      b.className = 'btn ' + (s ? 'on':'off');
    }

    function t(dev) {
      fetch('/api/ctrl', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({d:dev})})
      .then(()=>setTimeout(f, 300));
    }

    function saveSettings() {
      let data = {
        sh_on: parseFloat(document.getElementById('s-ho').value),
        sh_off: parseFloat(document.getElementById('s-hf').value),
        sf_on: parseFloat(document.getElementById('s-fo').value),
        sf_off: parseFloat(document.getElementById('s-ff').value),
        sap: document.getElementById('s-ap').value == 1,
        sad: document.getElementById('s-ad').value == 1,
        om: document.getElementById('s-om').value == 1,
        slm: document.getElementById('s-lm').value == 1,
        sl_on: document.getElementById('s-lon').value,
        sl_off: document.getElementById('s-loff').value,
        td: parseInt(document.getElementById('t-drain').value),
        th: parseInt(document.getElementById('t-heater').value),
        tf: parseInt(document.getElementById('t-fan').value),
        ssid: document.getElementById('s-ssid').value,
        pass: document.getElementById('s-pass').value,
        cam: document.getElementById('s-cam').value,
        ir1: document.getElementById('s-ir1').value, ir2: document.getElementById('s-ir2').value,
        ir3: document.getElementById('s-ir3').value, ir4: document.getElementById('s-ir4').value,
        ir5: document.getElementById('s-ir5').value, ir6: document.getElementById('s-ir6').value,
        ir7: document.getElementById('s-ir7').value, ir0: document.getElementById('s-ir0').value
      };
      fetch('/api/set', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(data)})
      .then(()=>alert('Da luu!'));
    }

    f(); setInterval(f, 2000);
  </script>
</body>
</html>
)rawliteral";

#endif
