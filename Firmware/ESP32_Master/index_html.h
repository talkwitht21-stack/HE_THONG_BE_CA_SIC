#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>He Thong Be Ca SIC</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
    .container { max-width: 540px; margin: 0 auto; padding: 16px; }
    h1 { text-align: center; color: #38bdf8; margin: 10px 0 4px 0; font-size: 1.4em; letter-spacing: 2px; font-weight: 800; }
    .subtitle { text-align: center; color: #475569; font-size: 0.76em; margin-bottom: 16px; letter-spacing: 1px; }
    .tabs { display: flex; gap: 8px; margin-bottom: 14px; }
    .tab { flex: 1; padding: 11px; text-align: center; background: #1e293b; border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 0.9em; color: #94a3b8; border: 1px solid #334155; }
    .tab.active { background: #3b82f6; color: #fff; border-color: #3b82f6; }
    .panel { display: none; }
    .panel.active { display: block; }
    .card { background: #1e293b; border-radius: 12px; padding: 14px; margin-bottom: 14px; border: 1px solid #1e3a5f; }
    .card-title { color: #64748b; font-size: 0.78em; text-transform: uppercase; margin-bottom: 12px; border-bottom: 1px solid #334155; padding-bottom: 6px; font-weight: 700; letter-spacing: 0.5px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    .box { background: #0f172a; padding: 12px 8px; border-radius: 8px; text-align: center; border: 1px solid #334155; }
    .val { font-size: 1.4em; font-weight: bold; color: #f8fafc; }
    .lbl { font-size: 0.72em; color: #64748b; margin-top: 4px; }
    .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; padding: 9px 10px; background: #0f172a; border-radius: 8px; border: 1px solid #1e293b; }
    .row-info { flex: 1; display: flex; flex-direction: column; }
    .row-label { font-size: 0.88em; color: #cbd5e1; font-weight: 600; }
    .countdown-lbl { font-size: 0.72em; color: #38bdf8; margin-top: 2px; }
    .row-ctrl { display: flex; align-items: center; gap: 6px; }
    .btn { padding: 8px 14px; border: none; border-radius: 6px; font-weight: bold; cursor: pointer; color: white; min-width: 62px; text-align: center; font-size: 0.85em; }
    .btn.on  { background: #16a34a; }
    .btn.off { background: #334155; }
    .btn.timer-btn { background: #4f46e5; font-size: 0.78em; padding: 8px 8px; min-width: 66px; }
    .btn.timer-btn.active { background: #9333ea; }
    .sys-btn { width: 100%; margin-bottom: 12px; padding: 12px; font-size: 0.95em; letter-spacing: 1px; }
    .feed-btn { background: #2563eb; width: 100%; padding: 13px; margin-top: 8px; font-size: 0.95em; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; color: white; }
    .timer-wrap { display: flex; align-items: center; gap: 2px; }
    .timer-input { width: 34px; padding: 5px 2px; background: #1e293b; border: 1px solid #334155; color: #38bdf8; border-radius: 4px; text-align: center; font-size: 0.8em; }
    .timer-unit { font-size: 0.68em; color: #64748b; margin-right: 2px; }
    .form-group { margin-bottom: 9px; display: flex; justify-content: space-between; align-items: center; font-size: 0.88em; }
    .form-group input { width: 88px; padding: 6px 8px; background: #0f172a; border: 1px solid #334155; color: white; border-radius: 5px; text-align: center; }
    .form-group input.wide { width: 175px; text-align: left; }
    .form-group select { padding: 6px 8px; background: #0f172a; border: 1px solid #334155; color: white; border-radius: 5px; width: 115px; }
    .save-btn { width: 100%; background: #7c3aed; color: white; padding: 13px; border: none; border-radius: 8px; font-weight: bold; cursor: pointer; margin-top: 14px; font-size: 0.95em; }
    .ir-lbl { font-size: 0.82em; color: #94a3b8; }
    .ir-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 9px; }
    .ir-row input { width: 52px; padding: 5px 6px; background: #0f172a; border: 1px solid #334155; color: #38bdf8; border-radius: 4px; text-align: center; font-size: 0.88em; font-weight: bold; }
    .ir-btn { padding: 5px 10px; background: #1d4ed8; border: none; border-radius: 4px; color: white; font-size: 0.78em; font-weight: bold; cursor: pointer; }
    .hint { font-size: 0.72em; color: #64748b; margin-top: -4px; margin-bottom: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>HE THONG BE CA SIC</h1>
    <p class="subtitle">GIAM SAT &amp; DIEU KHIEN TU DONG</p>
    <div class="tabs">
      <div class="tab active" onclick="switchTab('dash',this)">DASHBOARD</div>
      <div class="tab" onclick="switchTab('settings',this)">CAI DAT</div>
    </div>
    <div id="dash" class="panel active">
      <div class="card">
        <div class="card-title">Thong So Moi Truong</div>
        <div class="grid">
          <div class="box"><div class="val" id="v-wt">--</div><div class="lbl">Nhiet Do Nuoc (C)</div></div>
          <div class="box"><div class="val" id="v-at">--</div><div class="lbl">Nhiet Do KKhi (C)</div></div>
          <div class="box"><div class="val" id="v-ah">--</div><div class="lbl">Do Am KKhi (%)</div></div>
          <div class="box"><div class="val" id="v-wl">--</div><div class="lbl">Muc nuoc cach mat be (cm)</div></div>
          <div class="box"><div class="val" id="v-time" style="font-size:1.1em">--:--</div><div class="lbl">Gio He Thong</div></div>
          <div class="box"><div class="val" id="v-net" style="font-size:0.9em;color:#22c55e">ONLINE</div><div class="lbl">Ket Noi Web</div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-title">Dieu Khien Thiet Bi</div>
        <button class="btn on sys-btn" id="b-sys" onclick="toggle('system')">HE THONG: BAT</button>
        <p class="hint">Nhap Gio:Phut:Giay roi bam HEN GIO de tu tat, hoac bam BAT/TAT de bat tat thu cong.</p>
        <div class="row">
          <div class="row-info">
            <span class="row-label">May Suoi Nhiet</span>
            <span class="countdown-lbl" id="c-heater"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="th-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="th-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="th-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-heater" onclick="startTimer('heater')">HEN GIO</button>
            <button class="btn off" id="b-heater" onclick="toggle('heater')">TAT</button>
          </div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Quat Lam Mat</span>
            <span class="countdown-lbl" id="c-fan"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tf-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tf-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tf-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-fan" onclick="startTimer('fan')">HEN GIO</button>
            <button class="btn off" id="b-fan" onclick="toggle('fan')">TAT</button>
          </div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Bom Bu Nuoc</span>
          </div>
          <div class="row-ctrl"><button class="btn off" id="b-pump" onclick="toggle('pump')">TAT</button></div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Suc Oxy (<span id="oxy-mode-lbl">Chu ky</span>)</span>
          </div>
          <div class="row-ctrl"><button class="btn off" id="b-oxy" onclick="toggle('oxy')">TAT</button></div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Bom Rut Nuoc (Thu Cong)</span>
            <span class="countdown-lbl" id="c-drain"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="td-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="td-m" min="0" max="59" value="3"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="td-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-drain" onclick="startTimer('drain')">HEN GIO</button>
            <button class="btn off" id="b-drain" onclick="toggle('drain')">TAT</button>
          </div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Che Do Loc Nuoc (Song Song)</span>
            <span class="countdown-lbl" id="c-filter"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tfl-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tfl-m" min="0" max="59" value="15"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tfl-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-filter" onclick="startTimer('filter')">HEN GIO</button>
            <button class="btn off" id="b-filter" onclick="toggle('filter')">TAT</button>
          </div>
        </div>
        <div class="row">
          <div class="row-info">
            <span class="row-label">Den LED Chieu Sang</span>
            <span class="countdown-lbl" id="c-led"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tl-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tl-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tl-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-led" onclick="startTimer('led')">HEN GIO</button>
            <button class="btn off" id="b-led" onclick="toggle('led')">TAT</button>
          </div>
        </div>
        <button class="feed-btn" onclick="toggle('feed')">CHO AN NGAY</button>
      </div>
    </div>
    <div id="settings" class="panel">
      <div class="card">
        <div class="card-title">Nguong Nhiet Do (C)</div>
        <div class="form-group"><span>Bat Suoi khi &lt;</span><input type="number" id="s-ho" step="0.5"></div>
        <div class="form-group"><span>Tat Suoi khi &gt;=</span><input type="number" id="s-hf" step="0.5"></div>
        <div class="form-group"><span>Bat Quat khi &gt;</span><input type="number" id="s-fo" step="0.5"></div>
        <div class="form-group"><span>Tat Quat khi &lt;=</span><input type="number" id="s-ff" step="0.5"></div>
      </div>
      <div class="card">
        <div class="card-title">Nguong Muc Nuoc (cm cach mat be)</div>
        <div class="form-group"><span>Chieu cao be:</span><input type="number" id="s-thh" step="1"></div>
        <div class="form-group"><span>Nuoc DAY khi &lt;=</span><input type="number" id="s-twf" step="1"></div>
        <div class="form-group"><span>Nuoc THAP khi &gt;=</span><input type="number" id="s-twl" step="1"></div>
        <div class="form-group"><span>Nuoc CAN khi &gt;= (Cat bom rut, tat suoi)</span><input type="number" id="s-twe" step="1"></div>
        <div class="form-group"><span>Tu dong Bom Bu:</span><select id="s-ap"><option value="1">BAT</option><option value="0">TAT</option></select></div>
      </div>
      <div class="card">
        <div class="card-title">Suc Oxy</div>
        <div class="form-group"><span>Che do Suc Oxy:</span><select id="s-om"><option value="0">Chu ky</option><option value="1">Lien tuc</option></select></div>
        <div class="form-group"><span>Thoi gian BAT (phut):</span><input type="number" id="s-oo" min="1" max="999" step="1"></div>
        <div class="form-group"><span>Thoi gian TAT (phut):</span><input type="number" id="s-of" min="1" max="999" step="1"></div>
      </div>
      <div class="card">
        <div class="card-title">Hen Gio Den LED</div>
        <div class="form-group"><span>Kich hoat Hen gio:</span><select id="s-lm"><option value="0">TAT</option><option value="1">BAT</option></select></div>
        <div class="form-group"><span>Den Bat luc:</span><input type="time" id="s-lon"></div>
        <div class="form-group"><span>Den Tat luc:</span><input type="time" id="s-loff"></div>
      </div>
      <div class="card">
        <div class="card-title">Hen Gio Cho An Tu Dong (HH:MM:SS)</div>
        <div class="form-group">
          <span>Buoi 1:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe1" style="width:75px"><option value="0">TAT</option><option value="1">BAT</option></select>
            <input type="text" id="s-ft1" style="width:85px" placeholder="08:00:00">
          </div>
        </div>
        <div class="form-group">
          <span>Buoi 2:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe2" style="width:75px"><option value="0">TAT</option><option value="1">BAT</option></select>
            <input type="text" id="s-ft2" style="width:85px" placeholder="12:00:00">
          </div>
        </div>
        <div class="form-group">
          <span>Buoi 3:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe3" style="width:75px"><option value="0">TAT</option><option value="1">BAT</option></select>
            <input type="text" id="s-ft3" style="width:85px" placeholder="18:00:00">
          </div>
        </div>
      </div>
      <div class="card">
        <div class="card-title">Ket Noi WiFi &amp; Ten Mien Local</div>
        <div class="form-group"><span>WiFi SSID:</span><input type="text" id="s-ssid" class="wide"></div>
        <div class="form-group"><span>Mat Khau:</span><input type="text" id="s-pass" class="wide"></div>
        <div class="form-group"><span>Ten Mien Local:</span><span style="color:#38bdf8;font-weight:bold;font-size:0.9em">http://beca.local</span></div>
        <div class="form-group"><span>IP Camera:</span><input type="text" id="s-cam" class="wide"></div>
        <div class="form-group"><span>Trang thai WiFi:</span><span id="s-wst" style="font-weight:bold;font-size:0.85em">--</span></div>
      </div>
      <div class="card">
        <div class="card-title">Ket Noi MQTT ThingsBoard</div>
        <div class="form-group"><span>Kich hoat MQTT:</span><select id="s-mqe"><option value="0">TAT</option><option value="1">BAT</option></select></div>
        <div class="form-group"><span>MQTT Server:</span><input type="text" id="s-mqs" class="wide"></div>
        <div class="form-group"><span>Access Token:</span><input type="text" id="s-mqt" class="wide"></div>
        <div class="form-group"><span>Trang thai MQTT:</span><span id="s-mqst" style="font-weight:bold;font-size:0.85em">--</span></div>
      </div>
      <div class="card">
        <div class="card-title">Cai Dat Ma Phim Remote IR (Hex)</div>
        <div style="background:#0f172a;padding:10px;border-radius:6px;margin-bottom:12px;display:flex;justify-content:space-between;align-items:center;">
          <span style="font-size:0.86em">Ma IR vua bam tren Remote:</span>
          <span id="s-last-ir" style="color:#38bdf8;font-weight:bold;font-size:1.2em">--</span>
        </div>
        <div class="ir-row"><span class="ir-lbl">1 - May Suoi:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir1"><button class="ir-btn" onclick="ganIR('s-ir1')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">2 - Quat Lam Mat:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir2"><button class="ir-btn" onclick="ganIR('s-ir2')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">3 - Bom Bu Nuoc:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir3"><button class="ir-btn" onclick="ganIR('s-ir3')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">4 - Suc Oxy:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir4"><button class="ir-btn" onclick="ganIR('s-ir4')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">5 - Bom Rut Nuoc:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir5"><button class="ir-btn" onclick="ganIR('s-ir5')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">6 - Den LED:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir6"><button class="ir-btn" onclick="ganIR('s-ir6')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">7 - Cho An (Servo):</span><div style="display:flex;gap:6px"><input type="text" id="s-ir7"><button class="ir-btn" onclick="ganIR('s-ir7')">GAN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">0 - Tat Tat Ca:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir0"><button class="ir-btn" onclick="ganIR('s-ir0')">GAN</button></div></div>
      </div>
      <button class="save-btn" onclick="saveSettings()">LUU CAI DAT</button>
    </div>
  </div>
  <script>
    let _fetching = false;
    let _currentTab = 'dash';
    let _settingsLoaded = false;
    let _rem = { heater: 0, fan: 0, drain: 0, led: 0 };

    function switchTab(id, el) {
      _currentTab = id;
      document.querySelectorAll('.tab').forEach(e => e.classList.remove('active'));
      document.querySelectorAll('.panel').forEach(e => e.classList.remove('active'));
      el.classList.add('active');
      document.getElementById(id).classList.add('active');
      if (id === 'settings' && !_settingsLoaded) {
        f(true);
      }
    }

    function ganIR(inputId) {
      let code = document.getElementById('s-last-ir').textContent;
      if (code && code !== '--' && code.trim().length > 0) {
        let inp = document.getElementById(inputId);
        inp.value = code.trim();
        inp.focus();
      } else {
        alert('Hay bam mot nut tren Remote truoc de he thong bat ma Hex!');
      }
    }

    function formatSec(s) {
      if (s <= 0) return '';
      let h = Math.floor(s / 3600);
      let m = Math.floor((s % 3600) / 60);
      let sec = s % 60;
      let pad = (n) => n < 10 ? '0' + n : n;
      if (h > 0) return '(con: ' + pad(h) + ':' + pad(m) + ':' + pad(sec) + ')';
      return '(con: ' + pad(m) + ':' + pad(sec) + ')';
    }

    // Tick dem nguoc client-side moi giay
    setInterval(() => {
      ['heater', 'fan', 'drain', 'led'].forEach(k => {
        if (_rem[k] > 0) {
          _rem[k]--;
          let el = document.getElementById('c-' + k);
          if (el) el.textContent = formatSec(_rem[k]);
        } else {
          let el = document.getElementById('c-' + k);
          if (el) el.textContent = '';
        }
      });
    }, 1000);

    function secToHms(totalSec, pre) {
      let h = Math.floor(totalSec / 3600);
      let m = Math.floor((totalSec % 3600) / 60);
      let s = totalSec % 60;
      if (document.getElementById(pre + '-h')) document.getElementById(pre + '-h').value = h;
      if (document.getElementById(pre + '-m')) document.getElementById(pre + '-m').value = m;
      if (document.getElementById(pre + '-s')) document.getElementById(pre + '-s').value = s;
    }

    function hmsToSec(pre) {
      let h = parseInt(document.getElementById(pre + '-h').value) || 0;
      let m = parseInt(document.getElementById(pre + '-m').value) || 0;
      let s = parseInt(document.getElementById(pre + '-s').value) || 0;
      return h * 3600 + m * 60 + s;
    }

    function f(forceSettings) {
      if (_fetching) return;
      _fetching = true;
      const ctrl = new AbortController();
      const tid  = setTimeout(() => ctrl.abort(), 1500);

      fetch('/api/data', {signal: ctrl.signal})
        .then(r => r.json())
        .then(d => {
          clearTimeout(tid);

          // Cam bien
          document.getElementById('v-wt').textContent   = (d.wt > -50) ? d.wt.toFixed(1) : '--';
          document.getElementById('v-at').textContent   = (d.at > -50) ? d.at.toFixed(1) : '--';
          document.getElementById('v-ah').textContent   = (d.ah > -50) ? d.ah.toFixed(1) : '--';
          document.getElementById('v-wl').textContent   = (d.wcm > 0)  ? d.wcm.toFixed(1) : 'Loi';
          document.getElementById('v-time').textContent = d.time;
          document.getElementById('oxy-mode-lbl').textContent = d.om ? 'Lien tuc' : 'Chu ky';

          // Cong tac tong he thong
          let bSys = document.getElementById('b-sys');
          if (bSys) {
            bSys.textContent = d.sys ? 'HE THONG: BAT' : 'HE THONG: TAT';
            bSys.className = 'btn sys-btn ' + (d.sys ? 'on' : 'off');
          }

          // Relay buttons
          ub('b-heater', d.h); ub('b-fan', d.f); ub('b-pump', d.p);
          ub('b-oxy', d.o); ub('b-drain', d.d); ub('b-filter', d.fl); ub('b-led', d.l);

          // Cap nhat trang thai nut HEN GIO (Active khi dang dem lui)
          let bth  = document.getElementById('bt-heater'); if (bth)  bth.className  = 'btn timer-btn' + (d.th_a  ? ' active' : '');
          let btf  = document.getElementById('bt-fan');    if (btf)  btf.className  = 'btn timer-btn' + (d.tf_a  ? ' active' : '');
          let btd  = document.getElementById('bt-drain');  if (btd)  btd.className  = 'btn timer-btn' + (d.td_a  ? ' active' : '');
          let btfl = document.getElementById('bt-filter'); if (btfl) btfl.className = 'btn timer-btn' + (d.tfl_a ? ' active' : '');
          let btl  = document.getElementById('bt-led');    if (btl)  btl.className  = 'btn timer-btn' + (d.tl_a  ? ' active' : '');

          // Cap nhat bo dem nguoc tu Master
          _rem.heater = d.th_r  || 0;
          _rem.fan    = d.tf_r  || 0;
          _rem.drain  = d.td_r  || 0;
          _rem.filter = d.tfl_r || 0;
          _rem.led    = d.tl_r  || 0;
          ['heater', 'fan', 'drain', 'filter', 'led'].forEach(k => {
            let el = document.getElementById('c-' + k);
            if (el) el.textContent = formatSec(_rem[k]);
          });

          // WiFi
          let wstEl = document.getElementById('s-wst');
          if (d.wst == 2)      { wstEl.textContent = 'Da ket noi (' + d.wip + ')'; wstEl.style.color = '#22c55e'; }
          else if (d.wst == 1) { wstEl.textContent = 'Dang ket noi...'; wstEl.style.color = '#f59e0b'; }
          else                 { wstEl.textContent = 'Mat ket noi'; wstEl.style.color = '#ef4444'; }

          // MQTT
          let mqEl = document.getElementById('s-mqst');
          if (!d.mqe)     { mqEl.textContent = 'Da tat';       mqEl.style.color = '#64748b'; }
          else if (d.mqc) { mqEl.textContent = 'Da ket noi';   mqEl.style.color = '#22c55e'; }
          else            { mqEl.textContent = 'Chua ket noi'; mqEl.style.color = '#f59e0b'; }

          // Last IR
          if (d.last_ir && d.last_ir.length > 0) {
            document.getElementById('s-last-ir').textContent = d.last_ir;
          }

          // Cap nhat Timer Dashboard neu khong dang go focus
          if (_currentTab === 'dash' && !document.querySelector('.timer-input:focus')) {
            secToHms(d.ths, 'th');
            secToHms(d.tfs, 'tf');
            secToHms(d.tds, 'td');
            secToHms(d.tfls, 'tfl');
            secToHms(d.tls, 'tl');
          }

          // Cap nhat Settings inputs khi vao tab lan dau hoac sau khi luu
          if (!_settingsLoaded || forceSettings) {
            _settingsLoaded = true;
            document.getElementById('s-ho').value    = d.sh_on;
            document.getElementById('s-hf').value    = d.sh_off;
            document.getElementById('s-fo').value    = d.sf_on;
            document.getElementById('s-ff').value    = d.sf_off;
            document.getElementById('s-thh').value   = d.th_h;
            document.getElementById('s-twe').value   = d.th_we;
            document.getElementById('s-twl').value   = d.th_wl;
            document.getElementById('s-twf').value   = d.th_wf;
            document.getElementById('s-ap').value    = d.sap ? 1 : 0;
            
            document.getElementById('s-om').value    = d.om  ? 1 : 0;
            document.getElementById('s-oo').value    = d.oo  || 5;
            document.getElementById('s-of').value    = d.of  || 15;

            document.getElementById('s-lm').value    = d.slm ? 1 : 0;
            document.getElementById('s-lon').value   = d.sl_on;
            document.getElementById('s-loff').value  = d.sl_off;

            document.getElementById('s-fe1').value   = d.fen1 ? 1 : 0;
            document.getElementById('s-ft1').value   = d.ft1  || '08:00:00';
            document.getElementById('s-fe2').value   = d.fen2 ? 1 : 0;
            document.getElementById('s-ft2').value   = d.ft2  || '12:00:00';
            document.getElementById('s-fe3').value   = d.fen3 ? 1 : 0;
            document.getElementById('s-ft3').value   = d.ft3  || '18:00:00';

            document.getElementById('s-ssid').value  = d.ssid;
            document.getElementById('s-pass').value  = d.pass;
            document.getElementById('s-cam').value   = d.cam;
            document.getElementById('s-mqe').value   = d.mqe ? 1 : 0;
            document.getElementById('s-mqs').value   = d.mqs;
            document.getElementById('s-mqt').value   = d.mqt;

            document.getElementById('s-ir1').value   = d.ir1;
            document.getElementById('s-ir2').value   = d.ir2;
            document.getElementById('s-ir3').value   = d.ir3;
            document.getElementById('s-ir4').value   = d.ir4;
            document.getElementById('s-ir5').value   = d.ir5;
            document.getElementById('s-ir6').value   = d.ir6;
            document.getElementById('s-ir7').value   = d.ir7;
            document.getElementById('s-ir0').value   = d.ir0;
          }
        })
        .catch(() => { clearTimeout(tid); })
        .finally(() => { _fetching = false; });
    }

    function ub(id, s) {
      let b = document.getElementById(id);
      if (!b) return;
      b.textContent = s ? 'BAT' : 'TAT';
      b.className   = 'btn ' + (s ? 'on' : 'off');
    }

    const timerMap = { heater: {pre:'th', key:'ths'}, fan: {pre:'tf', key:'tfs'}, drain: {pre:'td', key:'tds'}, filter: {pre:'tfl', key:'tfls'}, led: {pre:'tl', key:'tls'} };

    function startTimer(dev) {
      if (timerMap[dev]) {
        let sec = hmsToSec(timerMap[dev].pre);
        if (sec <= 0) {
          alert('Hay nhap so Gio / Phut / Giay can hen gio truoc!');
          return;
        }
        fetch('/api/timer', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({d: dev, sec: sec})
        }).then(() => { _fetching = false; f(); });
      }
    }

    function toggle(dev) {
      if (dev === 'system') {
        sendCtrl('system');
        return;
      }

      const btnId = {heater:'b-heater', fan:'b-fan', pump:'b-pump', oxy:'b-oxy', drain:'b-drain', filter:'b-filter', led:'b-led'};
      if (btnId[dev]) {
        let b = document.getElementById(btnId[dev]);
        ub(btnId[dev], !b.classList.contains('on'));
      }
      sendCtrl(dev);
    }

    function sendCtrl(dev) {
      fetch('/api/ctrl', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify({d: dev})
      }).then(() => { _fetching = false; f(); });
    }

    function saveSettings() {
      let data = {
        sh_on:  parseFloat(document.getElementById('s-ho').value),
        sh_off: parseFloat(document.getElementById('s-hf').value),
        sf_on:  parseFloat(document.getElementById('s-fo').value),
        sf_off: parseFloat(document.getElementById('s-ff').value),
        th_h:   parseFloat(document.getElementById('s-thh').value),
        th_we:  parseFloat(document.getElementById('s-twe').value),
        th_wl:  parseFloat(document.getElementById('s-twl').value),
        th_wf:  parseFloat(document.getElementById('s-twf').value),
        sap:    document.getElementById('s-ap').value == 1,
        
        om:     document.getElementById('s-om').value == 1,
        oo:     parseInt(document.getElementById('s-oo').value) || 5,
        of:     parseInt(document.getElementById('s-of').value) || 15,

        slm:    document.getElementById('s-lm').value == 1,
        sl_on:  document.getElementById('s-lon').value,
        sl_off: document.getElementById('s-loff').value,

        fen1:   document.getElementById('s-fe1').value == 1,
        ft1:    document.getElementById('s-ft1').value,
        fen2:   document.getElementById('s-fe2').value == 1,
        ft2:    document.getElementById('s-ft2').value,
        fen3:   document.getElementById('s-fe3').value == 1,
        ft3:    document.getElementById('s-ft3').value,

        ths:    hmsToSec('th'),
        tfs:    hmsToSec('tf'),
        tds:    hmsToSec('td'),
        tfls:   hmsToSec('tfl'),
        tls:    hmsToSec('tl'),

        ssid:   document.getElementById('s-ssid').value,
        pass:   document.getElementById('s-pass').value,
        cam:    document.getElementById('s-cam').value,
        mqe:    document.getElementById('s-mqe').value == 1,
        mqs:    document.getElementById('s-mqs').value,
        mqt:    document.getElementById('s-mqt').value,

        ir1:    document.getElementById('s-ir1').value,
        ir2:    document.getElementById('s-ir2').value,
        ir3:    document.getElementById('s-ir3').value,
        ir4:    document.getElementById('s-ir4').value,
        ir5:    document.getElementById('s-ir5').value,
        ir6:    document.getElementById('s-ir6').value,
        ir7:    document.getElementById('s-ir7').value,
        ir0:    document.getElementById('s-ir0').value
      };
      fetch('/api/set', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify(data)
      }).then(() => {
        alert('Da luu cai dat thanh cong!');
        _fetching = false;
        f(true);
      });
    }

    f(true);
    setInterval(f, 1500);
  </script>
</body>
</html>
)rawliteral";

#endif
