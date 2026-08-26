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
    .container { max-width: 520px; margin: 0 auto; padding: 16px; }
    h1 { text-align: center; color: #38bdf8; margin: 10px 0; font-size: 1.5em; letter-spacing: 1px; }
    .tabs { display: flex; gap: 10px; margin-bottom: 15px; }
    .tab { flex: 1; padding: 12px; text-align: center; background: #334155; border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 0.95em; }
    .tab.active { background: #3b82f6; color: #ffffff; }
    .panel { display: none; }
    .panel.active { display: block; }
    .card { background: #1e293b; border-radius: 12px; padding: 15px; margin-bottom: 15px; }
    .card-title { color: #94a3b8; font-size: 0.85em; text-transform: uppercase; margin-bottom: 12px; border-bottom: 1px solid #334155; padding-bottom: 6px; font-weight: bold; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .box { background: #334155; padding: 12px 10px; border-radius: 8px; text-align: center; }
    .val { font-size: 1.35em; font-weight: bold; color: #f8fafc; }
    .lbl { font-size: 0.75em; color: #94a3b8; margin-top: 4px; }
    .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .btn { padding: 9px 18px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; color: white; min-width: 75px; text-align: center; }
    .btn.on { background: #22c55e; }
    .btn.off { background: #475569; }
    .btn.feed { background: #3b82f6; width: 100%; padding: 14px; margin-top: 10px; font-size: 1em; }
    .form-group { margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; font-size: 0.9em; }
    .form-group input { width: 90px; padding: 6px 8px; background: #0f172a; border: 1px solid #475569; color: white; border-radius: 4px; text-align: center; }
    .form-group input.text-wide { width: 170px; text-align: left; }
    .form-group select { padding: 6px 8px; background: #0f172a; border: 1px solid #475569; color: white; border-radius: 4px; width: 110px; }
    .save-btn { width: 100%; background: #8b5cf6; color: white; padding: 14px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; margin-top: 12px; font-size: 1em; }
    .ir-grid { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 8px; }
    .ir-item { display: flex; align-items: center; justify-content: space-between; background: #0f172a; padding: 6px 8px; border-radius: 4px; border: 1px solid #334155; font-size: 0.85em; }
    .ir-item input { width: 35px; background: transparent; border: none; color: #38bdf8; font-weight: bold; text-align: center; }
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
        <div class="card-title">Thong So Moi Truong</div>
        <div class="grid">
          <div class="box"><div class="val" id="v-wt">--</div><div class="lbl">Nuoc (C)</div></div>
          <div class="box"><div class="val" id="v-at">--</div><div class="lbl">K.Khi (C)</div></div>
          <div class="box"><div class="val" id="v-ah">--</div><div class="lbl">Do Am (%)</div></div>
          <div class="box"><div class="val" id="v-wl">--</div><div class="lbl">Muc Nuoc (cm)</div></div>
          <div class="box"><div class="val" id="v-time">--:--</div><div class="lbl">Gio He Thong</div></div>
          <div class="box"><div class="val" id="v-net" style="font-size:1em; color:#22c55e;">ONLINE</div><div class="lbl">Trang Thai Web</div></div>
        </div>
      </div>

      <div class="card">
        <div class="card-title">Dieu Khien Thiet Bi</div>
        <div class="row"><span>May Suoi</span><button class="btn off" id="b-heater" onclick="t('heater')">TAT</button></div>
        <div class="row"><span>Quat Lam Mat</span><button class="btn off" id="b-fan" onclick="t('fan')">TAT</button></div>
        <div class="row"><span>Bom Bu Nuoc</span><button class="btn off" id="b-pump" onclick="t('pump')">TAT</button></div>
        <div class="row"><span>Suc Oxy (<span id="oxy-mode-lbl">Chu ky</span>)</span><button class="btn off" id="b-oxy" onclick="t('oxy')">TAT</button></div>
        <div class="row"><span>Bom Thay Nuoc</span><button class="btn off" id="b-drain" onclick="t('drain')">TAT</button></div>
        <div class="row"><span>Den LED Chieu Sang</span><button class="btn off" id="b-led" onclick="t('led')">TAT</button></div>
        <button class="btn feed" onclick="t('feed')">CHO AN NGAY</button>
      </div>
    </div>

    <!-- SETTINGS PANEL -->
    <div id="settings" class="panel">
      
      <!-- Cài đặt Nhiệt độ -->
      <div class="card">
        <div class="card-title">Nguong Nhiet Do (C)</div>
        <div class="form-group"><span>Bat Suoi khi <</span><input type="number" id="s-ho" step="0.5"></div>
        <div class="form-group"><span>Tat Suoi khi >=</span><input type="number" id="s-hf" step="0.5"></div>
        <div class="form-group"><span>Bat Quat khi ></span><input type="number" id="s-fo" step="0.5"></div>
        <div class="form-group"><span>Tat Quat khi <=</span><input type="number" id="s-ff" step="0.5"></div>
      </div>
      
      <!-- Cài đặt Mực nước siêu âm -->
      <div class="card">
        <div class="card-title">Nguong Muc Nuoc Sieu Am (cm)</div>
        <div class="form-group"><span>Chieu cao be:</span><input type="number" id="s-thh" step="1"></div>
        <div class="form-group"><span>Muc nuoc can (<):</span><input type="number" id="s-twe" step="1"></div>
        <div class="form-group"><span>Muc nuoc thap (<):</span><input type="number" id="s-twl" step="1"></div>
        <div class="form-group"><span>Muc nuoc day (>=):</span><input type="number" id="s-twf" step="1"></div>
        <div class="form-group"><span>Tu dong bom bu:</span><select id="s-ap"><option value="1">BAT</option><option value="0">TAT</option></select></div>
        <div class="form-group"><span>Tu dong bom thay:</span><select id="s-ad"><option value="1">BAT</option><option value="0">TAT</option></select></div>
      </div>

      <!-- Cài đặt Oxy & Đèn -->
      <div class="card">
        <div class="card-title">Suc Oxy & Hen Gio Den</div>
        <div class="form-group"><span>Che do Suc Oxy:</span><select id="s-om"><option value="0">Chu ky (5p/15p)</option><option value="1">Lien tuc</option></select></div>
        <div class="form-group"><span>Hen gio Den LED:</span><select id="s-lm"><option value="1">BAT</option><option value="0">TAT</option></select></div>
        <div class="form-group"><span>Den Bat luc:</span><input type="time" id="s-lon"></div>
        <div class="form-group"><span>Den Tat luc:</span><input type="time" id="s-loff"></div>
      </div>

      <!-- Cài đặt Timer tự tắt -->
      <div class="card">
        <div class="card-title">Timer Tu Tat (Phut) - 0 = Khong dung</div>
        <div class="form-group"><span>Suoi:</span><input type="number" id="t-heater"></div>
        <div class="form-group"><span>Quat:</span><input type="number" id="t-fan"></div>
        <div class="form-group"><span>Bom thay:</span><input type="number" id="t-drain"></div>
      </div>

      <!-- Cài đặt Mạng WiFi & Camera -->
      <div class="card">
        <div class="card-title">Ket Noi WiFi & Camera</div>
        <div class="form-group"><span>WiFi Nha (SSID):</span><input type="text" id="s-ssid" class="text-wide"></div>
        <div class="form-group"><span>Pass WiFi:</span><input type="text" id="s-pass" class="text-wide"></div>
        <div class="form-group"><span>IP Camera:</span><input type="text" id="s-cam" class="text-wide"></div>
        <div class="form-group"><span>Trang thai WiFi:</span><span id="s-wst" style="font-weight:bold; font-size:0.85em;">--</span></div>
      </div>

      <!-- Cài đặt MQTT ThingsBoard -->
      <div class="card">
        <div class="card-title">Ket Noi MQTT ThingsBoard</div>
        <div class="form-group"><span>Kich hoat MQTT:</span><select id="s-mqe"><option value="0">TAT</option><option value="1">BAT</option></select></div>
        <div class="form-group"><span>MQTT Server:</span><input type="text" id="s-mqs" class="text-wide"></div>
        <div class="form-group"><span>MQTT Token:</span><input type="text" id="s-mqt" class="text-wide"></div>
        <div class="form-group"><span>Trang thai MQTT:</span><span id="s-mqst" style="font-weight:bold; font-size:0.85em;">--</span></div>
      </div>
      
      <!-- Cài đặt Mã Remote IR -->
      <div class="card">
        <div class="card-title">Cai Dat Ma Remote IR (Hex)</div>
        <div style="background:#0f172a; padding:10px; border-radius:6px; margin-bottom:12px; font-size:0.9em; display:flex; justify-content:space-between; align-items:center;">
          <span>Ma IR vua bam tren Remote:</span>
          <span id="s-last-ir" style="color:#38bdf8; font-weight:bold; font-size:1.15em;">--</span>
        </div>
        <div class="form-group">
          <span>1 - May Suoi:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir1" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir1')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>2 - Quat Lam Mat:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir2" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir2')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>3 - Bom Bu Nuoc:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir3" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir3')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>4 - Suc Oxy:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir4" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir4')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>5 - Bom Thay Nuoc:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir5" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir5')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>6 - Den LED:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir6" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir6')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>7 - Cho An (Servo):</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir7" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir7')">GAN</button>
          </div>
        </div>
        <div class="form-group">
          <span>0 - Tat Tat Ca:</span>
          <div style="display:flex; gap:6px;">
            <input type="text" id="s-ir0" style="width:50px;">
            <button type="button" class="btn" style="padding:4px 8px; font-size:0.8em; background:#3b82f6;" onclick="assignIR('s-ir0')">GAN</button>
          </div>
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

    function assignIR(id) {
      let code = document.getElementById('s-last-ir').textContent;
      if (code && code !== '--' && code.length > 0) {
        document.getElementById(id).value = code;
      } else {
        alert('Hay bam mot nut bat ky tren Remote truoc de he thong bat ma Hex!');
      }
    }

    function f() {
      fetch('/api/data').then(r=>r.json()).then(d=>{
        // Update Dashboard
        document.getElementById('v-wt').textContent = (d.wt > -50) ? d.wt.toFixed(1) : '--';
        document.getElementById('v-at').textContent = (d.at > -50) ? d.at.toFixed(1) : '--';
        document.getElementById('v-ah').textContent = (d.ah > -50) ? d.ah.toFixed(1) : '--';
        document.getElementById('v-wl').textContent = (d.wcm > 0) ? d.wcm.toFixed(1) : 'Loi';
        document.getElementById('v-time').textContent = d.time;
        document.getElementById('oxy-mode-lbl').textContent = d.om ? 'Lien tuc' : 'Chu ky';

        ub('b-heater', d.h); ub('b-fan', d.f); ub('b-pump', d.p);
        ub('b-oxy', d.o); ub('b-drain', d.d); ub('b-led', d.l);

        // Update WiFi status
        let wstEl = document.getElementById('s-wst');
        if (d.wst == 2) {
          wstEl.textContent = 'Da ket noi (' + d.wip + ')';
          wstEl.style.color = '#22c55e';
        } else if (d.wst == 1) {
          wstEl.textContent = 'Dang ket noi...';
          wstEl.style.color = '#f59e0b';
        } else {
          wstEl.textContent = 'Mat ket noi';
          wstEl.style.color = '#ef4444';
        }

        // Update MQTT status
        let mqstEl = document.getElementById('s-mqst');
        if (!d.mqe) {
          mqstEl.textContent = 'Da tat';
          mqstEl.style.color = '#94a3b8';
        } else if (d.mqc) {
          mqstEl.textContent = 'Da ket noi';
          mqstEl.style.color = '#22c55e';
        } else {
          mqstEl.textContent = 'Chua ket noi';
          mqstEl.style.color = '#f59e0b';
        }

        // Update Last IR Code
        if (d.last_ir && d.last_ir.length > 0) {
          document.getElementById('s-last-ir').textContent = d.last_ir;
        }

        // Only populate inputs if user is not actively typing
        if (!document.querySelector('input:focus')) {
          document.getElementById('s-ho').value = d.sh_on;
          document.getElementById('s-hf').value = d.sh_off;
          document.getElementById('s-fo').value = d.sf_on;
          document.getElementById('s-ff').value = d.sf_off;
          
          document.getElementById('s-thh').value = d.th_h;
          document.getElementById('s-twe').value = d.th_we;
          document.getElementById('s-twl').value = d.th_wl;
          document.getElementById('s-twf').value = d.th_wf;
          document.getElementById('s-ap').value = d.sap ? 1 : 0;
          document.getElementById('s-ad').value = d.sad ? 1 : 0;

          document.getElementById('s-om').value = d.om ? 1 : 0;
          document.getElementById('s-lm').value = d.slm ? 1 : 0;
          document.getElementById('s-lon').value = d.sl_on;
          document.getElementById('s-loff').value = d.sl_off;

          document.getElementById('t-heater').value = d.th;
          document.getElementById('t-fan').value = d.tf;
          document.getElementById('t-drain').value = d.td;

          document.getElementById('s-ssid').value = d.ssid;
          document.getElementById('s-pass').value = d.pass;
          document.getElementById('s-cam').value = d.cam;

          document.getElementById('s-mqe').value = d.mqe ? 1 : 0;
          document.getElementById('s-mqs').value = d.mqs;
          document.getElementById('s-mqt').value = d.mqt;

          document.getElementById('s-ir1').value = d.ir1;
          document.getElementById('s-ir2').value = d.ir2;
          document.getElementById('s-ir3').value = d.ir3;
          document.getElementById('s-ir4').value = d.ir4;
          document.getElementById('s-ir5').value = d.ir5;
          document.getElementById('s-ir6').value = d.ir6;
          document.getElementById('s-ir7').value = d.ir7;
          document.getElementById('s-ir0').value = d.ir0;
        }
      });
    }

    function ub(id, s) {
      let b = document.getElementById(id);
      b.textContent = s ? 'BAT' : 'TAT';
      b.className = 'btn ' + (s ? 'on' : 'off');
    }

    function t(dev) {
      fetch('/api/ctrl', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({d: dev})
      }).then(() => setTimeout(f, 300));
    }

    function saveSettings() {
      let data = {
        sh_on: parseFloat(document.getElementById('s-ho').value),
        sh_off: parseFloat(document.getElementById('s-hf').value),
        sf_on: parseFloat(document.getElementById('s-fo').value),
        sf_off: parseFloat(document.getElementById('s-ff').value),
        
        th_h: parseFloat(document.getElementById('s-thh').value),
        th_we: parseFloat(document.getElementById('s-twe').value),
        th_wl: parseFloat(document.getElementById('s-twl').value),
        th_wf: parseFloat(document.getElementById('s-twf').value),
        sap: document.getElementById('s-ap').value == 1,
        sad: document.getElementById('s-ad').value == 1,

        om: document.getElementById('s-om').value == 1,
        slm: document.getElementById('s-lm').value == 1,
        sl_on: document.getElementById('s-lon').value,
        sl_off: document.getElementById('s-loff').value,

        th: parseInt(document.getElementById('t-heater').value),
        tf: parseInt(document.getElementById('t-fan').value),
        td: parseInt(document.getElementById('t-drain').value),

        ssid: document.getElementById('s-ssid').value,
        pass: document.getElementById('s-pass').value,
        cam: document.getElementById('s-cam').value,

        mqe: document.getElementById('s-mqe').value == 1,
        mqs: document.getElementById('s-mqs').value,
        mqt: document.getElementById('s-mqt').value,

        ir1: document.getElementById('s-ir1').value,
        ir2: document.getElementById('s-ir2').value,
        ir3: document.getElementById('s-ir3').value,
        ir4: document.getElementById('s-ir4').value,
        ir5: document.getElementById('s-ir5').value,
        ir6: document.getElementById('s-ir6').value,
        ir7: document.getElementById('s-ir7').value,
        ir0: document.getElementById('s-ir0').value
      };

      fetch('/api/set', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
      }).then(() => alert('Da luu cai dat thanh cong!'));
    }

    f();
    setInterval(f, 2000);
  </script>
</body>
</html>
)rawliteral";

#endif
