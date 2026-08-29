#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hệ Thống Bể Cá Thông Minh SIC</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; background: #f0f9ff; color: #1e293b; min-height: 100vh; }
    .container { max-width: 560px; margin: 0 auto; padding: 16px; }
    
    /* Header */
    .header-box { text-align: center; margin-bottom: 16px; padding: 14px; background: #ffffff; border-radius: 16px; border: 1px solid #bae6fd; box-shadow: 0 4px 15px rgba(14, 165, 233, 0.08); }
    h1 { color: #0284c7; font-size: 1.35em; letter-spacing: 1px; font-weight: 800; display: flex; align-items: center; justify-content: center; gap: 8px; }
    .subtitle { color: #64748b; font-size: 0.78em; margin-top: 4px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; }
    
    /* Navigation Tabs */
    .tabs { display: flex; gap: 8px; margin-bottom: 14px; }
    .tab { flex: 1; padding: 12px; text-align: center; background: #ffffff; border-radius: 10px; cursor: pointer; font-weight: bold; font-size: 0.88em; color: #64748b; border: 1px solid #e2e8f0; transition: all 0.2s ease; }
    .tab:hover { background: #e0f2fe; color: #0284c7; }
    .tab.active { background: linear-gradient(135deg, #38bdf8 0%, #0ea5e9 100%); color: #ffffff; border-color: transparent; box-shadow: 0 4px 12px rgba(14, 165, 233, 0.25); }
    
    .panel { display: none; }
    .panel.active { display: block; }
    
    /* Cards */
    .card { background: #ffffff; border-radius: 14px; padding: 16px; margin-bottom: 14px; border: 1px solid #e0f2fe; box-shadow: 0 2px 10px rgba(14, 165, 233, 0.04); }
    .card-title { color: #0369a1; font-size: 0.82em; text-transform: uppercase; margin-bottom: 12px; border-bottom: 1.5px solid #f0f9ff; padding-bottom: 6px; font-weight: 800; letter-spacing: 0.5px; display: flex; justify-content: space-between; align-items: center; }
    
    /* Metrics Grid */
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .box { background: #f8fafc; padding: 12px 8px; border-radius: 10px; text-align: center; border: 1px solid #e0f2fe; transition: all 0.15s ease; }
    .box:hover { border-color: #7dd3fc; background: #f0f9ff; transform: translateY(-1px); }
    .val { font-size: 1.45em; font-weight: 800; color: #0284c7; }
    .lbl { font-size: 0.72em; color: #64748b; margin-top: 4px; font-weight: 600; }
    
    /* Control Rows */
    .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; padding: 10px 12px; background: #f8fafc; border-radius: 10px; border: 1px solid #e2e8f0; }
    .row-info { flex: 1; display: flex; flex-direction: column; }
    .row-label { font-size: 0.88em; color: #1e293b; font-weight: 700; }
    .countdown-lbl { font-size: 0.72em; color: #0ea5e9; font-weight: 700; margin-top: 2px; }
    .row-ctrl { display: flex; align-items: center; gap: 6px; }
    
    /* Buttons */
    .btn { padding: 8px 14px; border: none; border-radius: 8px; font-weight: bold; cursor: pointer; color: white; min-width: 64px; text-align: center; font-size: 0.85em; transition: all 0.2s ease; }
    .btn:active { transform: scale(0.96); }
    .btn.on  { background: #10b981; box-shadow: 0 2px 8px rgba(16, 185, 129, 0.3); }
    .btn.off { background: #94a3b8; }
    .btn.timer-btn { background: #0ea5e9; font-size: 0.76em; padding: 8px 10px; min-width: 68px; }
    .btn.timer-btn.active { background: #0284c7; }
    
    .sys-btn { width: 100%; margin-bottom: 12px; padding: 12px; font-size: 0.95em; letter-spacing: 1px; font-weight: 800; border-radius: 10px; }
    .feed-btn { background: linear-gradient(135deg, #38bdf8 0%, #0ea5e9 100%); width: 100%; padding: 13px; margin-top: 8px; font-size: 0.95em; font-weight: 800; border: none; border-radius: 10px; cursor: pointer; color: white; box-shadow: 0 4px 12px rgba(14, 165, 233, 0.25); transition: all 0.2s ease; }
    .feed-btn:active { transform: scale(0.98); }
    
    .timer-wrap { display: flex; align-items: center; gap: 2px; }
    .timer-input { width: 34px; padding: 5px 2px; background: #ffffff; border: 1px solid #cbd5e1; color: #0284c7; border-radius: 6px; text-align: center; font-size: 0.8em; font-weight: bold; }
    .timer-unit { font-size: 0.68em; color: #64748b; margin-right: 2px; font-weight: 600; }
    
    /* Settings Form */
    .form-group { margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; font-size: 0.88em; color: #334155; font-weight: 600; }
    .form-group input { width: 90px; padding: 7px 8px; background: #ffffff; border: 1.5px solid #cbd5e1; color: #0f172a; border-radius: 6px; text-align: center; font-weight: bold; font-size: 0.9em; }
    .form-group input:focus { border-color: #38bdf8; outline: none; }
    .form-group input.wide { width: 180px; text-align: left; }
    .form-group select { padding: 7px 8px; background: #ffffff; border: 1.5px solid #cbd5e1; color: #0f172a; border-radius: 6px; width: 115px; font-weight: bold; }
    .save-btn { width: 100%; background: linear-gradient(135deg, #38bdf8 0%, #0ea5e9 100%); color: white; padding: 13px; border: none; border-radius: 10px; font-weight: 800; cursor: pointer; margin-top: 14px; font-size: 0.95em; box-shadow: 0 4px 12px rgba(14, 165, 233, 0.25); }
    
    /* IR Mapping */
    .ir-lbl { font-size: 0.84em; color: #475569; font-weight: 600; }
    .ir-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 9px; }
    .ir-row input { width: 54px; padding: 5px 6px; background: #f8fafc; border: 1.5px solid #cbd5e1; color: #0284c7; border-radius: 6px; text-align: center; font-size: 0.88em; font-weight: 800; }
    .ir-btn { padding: 6px 12px; background: #0ea5e9; border: none; border-radius: 6px; color: white; font-size: 0.78em; font-weight: bold; cursor: pointer; }
    
    .hint { font-size: 0.72em; color: #64748b; margin-top: -4px; margin-bottom: 10px; }
    .badge { display: inline-block; padding: 3px 8px; border-radius: 20px; font-size: 0.75em; font-weight: 800; }
    .badge-on { background: #dcfce7; color: #166534; }
    .badge-off { background: #fee2e2; color: #991b1b; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header-box">
      <h1>HỆ THỐNG BỂ CÁ SIC</h1>
      <p class="subtitle">Giám Sát &amp; Điều Khiển Tự Động IoT</p>
    </div>

    <div class="tabs">
      <div class="tab active" onclick="switchTab('dash',this)">BẢNG ĐIỀU KHIỂN</div>
      <div class="tab" onclick="switchTab('settings',this)">CÀI ĐẶT HỆ THỐNG</div>
    </div>

    <!-- TAB DASHBOARD -->
    <div id="dash" class="panel active">
      <div class="card">
        <div class="card-title">
          <span>Thông Số Môi Trường</span>
          <span id="v-net" class="badge badge-on">ONLINE</span>
        </div>
        <div class="grid">
          <div class="box"><div class="val" id="v-wt">--</div><div class="lbl">Nhiệt Độ Nước (°C)</div></div>
          <div class="box"><div class="val" id="v-at">--</div><div class="lbl">Nhiệt Độ K.Khí (°C)</div></div>
          <div class="box"><div class="val" id="v-ah">--</div><div class="lbl">Độ Ẩm K.Khí (%)</div></div>
          <div class="box"><div class="val" id="v-wl">--</div><div class="lbl">Mực Nước (cm cách nắp)</div></div>
          <div class="box"><div class="val" id="v-time" style="font-size:1.2em">--:--</div><div class="lbl">Giờ Hệ Thống (NTP)</div></div>
          <div class="box"><div class="val" id="v-rssi" style="font-size:1.1em;color:#0284c7">-- dBm</div><div class="lbl">Sóng WiFi (RSSI)</div></div>
        </div>
      </div>

      <div class="card">
        <div class="card-title">Điều Khiển Thiết Bị</div>
        <button class="btn on sys-btn" id="b-sys" onclick="toggle('system')">HỆ THỐNG: ĐANG BẬT</button>
        <p class="hint">Nhập Giờ:Phút:Giây rồi bấm HẸN GIỜ để tự tắt, hoặc bấm BẬT/TẮT để điều khiển thủ công.</p>
        
        <div class="row">
          <div class="row-info">
            <span class="row-label">Máy Sưởi Nhiệt</span>
            <span class="countdown-lbl" id="c-heater"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="th-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="th-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="th-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-heater" onclick="startTimer('heater')">HẸN GIỜ</button>
            <button class="btn off" id="b-heater" onclick="toggle('heater')">TẮT</button>
          </div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Quạt Làm Mát</span>
            <span class="countdown-lbl" id="c-fan"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tf-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tf-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tf-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-fan" onclick="startTimer('fan')">HẸN GIỜ</button>
            <button class="btn off" id="b-fan" onclick="toggle('fan')">TẮT</button>
          </div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Bơm Bù Nước</span>
          </div>
          <div class="row-ctrl"><button class="btn off" id="b-pump" onclick="toggle('pump')">TẮT</button></div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Sục Khí Oxy (<span id="oxy-mode-lbl" style="color:#0ea5e9">Chu kỳ</span>)</span>
          </div>
          <div class="row-ctrl"><button class="btn off" id="b-oxy" onclick="toggle('oxy')">TẮT</button></div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Bơm Rút / Xả Nước</span>
            <span class="countdown-lbl" id="c-drain"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="td-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="td-m" min="0" max="59" value="3"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="td-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-drain" onclick="startTimer('drain')">HẸN GIỜ</button>
            <button class="btn off" id="b-drain" onclick="toggle('drain')">TẮT</button>
          </div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Lọc Nước Tuần Hoàn (Song Song)</span>
            <span class="countdown-lbl" id="c-filter"></span>
            <span id="lbl-filter-cycle" style="font-size:0.75em;color:#0ea5e9;display:block;font-weight:bold"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tfl-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tfl-m" min="0" max="59" value="15"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tfl-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-filter" onclick="startTimer('filter')">HẸN GIỜ</button>
            <button class="btn off" id="b-filter-cycle" onclick="toggle('filter_cycle')">CHU KỲ: TẮT</button>
            <button class="btn off" id="b-filter" onclick="toggle('filter')">TẮT</button>
          </div>
        </div>

        <div class="row">
          <div class="row-info">
            <span class="row-label">Đèn LED Chiếu Sáng</span>
            <span class="countdown-lbl" id="c-led"></span>
          </div>
          <div class="row-ctrl">
            <div class="timer-wrap">
              <input type="number" class="timer-input" id="tl-h" min="0" max="99" value="0"><span class="timer-unit">g</span>
              <input type="number" class="timer-input" id="tl-m" min="0" max="59" value="0"><span class="timer-unit">p</span>
              <input type="number" class="timer-input" id="tl-s" min="0" max="59" value="0"><span class="timer-unit">s</span>
            </div>
            <button class="btn timer-btn" id="bt-led" onclick="startTimer('led')">HẸN GIỜ</button>
            <button class="btn off" id="b-led" onclick="toggle('led')">TẮT</button>
          </div>
        </div>

        <button class="feed-btn" onclick="toggle('feed')">CHO ĂN NGAY TỨC THÌ</button>
      </div>
    </div>

    <!-- TAB SETTINGS -->
    <div id="settings" class="panel">
      <div class="card">
        <div class="card-title">Ngưỡng Nhiệt Độ (°C)</div>
        <div class="form-group"><span>Bật Sưởi khi &lt;</span><input type="number" id="s-ho" step="0.5"></div>
        <div class="form-group"><span>Tắt Sưởi khi &gt;=</span><input type="number" id="s-hf" step="0.5"></div>
        <div class="form-group"><span>Bật Quạt khi &gt;</span><input type="number" id="s-fo" step="0.5"></div>
        <div class="form-group"><span>Tắt Quạt khi &lt;=</span><input type="number" id="s-ff" step="0.5"></div>
      </div>

      <div class="card">
        <div class="card-title">Ngưỡng Mực Nước Siêu Âm (cm)</div>
        <div class="form-group"><span>Chiều cao bể cá:</span><input type="number" id="s-thh" step="1"></div>
        <div class="form-group"><span>Nước ĐẦY khi &lt;= (Dừng bơm)</span><input type="number" id="s-twf" step="1"></div>
        <div class="form-group"><span>Nước THẤP khi &gt;= (Bơm bù)</span><input type="number" id="s-twl" step="1"></div>
        <div class="form-group"><span>Nước CẠN NGUY HIỂM khi &gt;=</span><input type="number" id="s-twe" step="1"></div>
        <div class="form-group"><span>Tự động Bơm Bù Nước:</span><select id="s-ap"><option value="1">BẬT</option><option value="0">TẮT</option></select></div>
      </div>

      <div class="card">
        <div class="card-title">Sục Khí Oxy</div>
        <div class="form-group"><span>Chế độ Sục Oxy:</span><select id="s-om"><option value="0">Chu kỳ</option><option value="1">Liên tục 24/7</option></select></div>
        <div class="form-group"><span>Thời gian BẬT (phút):</span><input type="number" id="s-oo" min="1" max="999" step="1"></div>
        <div class="form-group"><span>Thời gian TẮT (phút):</span><input type="number" id="s-of" min="1" max="999" step="1"></div>
      </div>

      <div class="card">
        <div class="card-title">Chu Kỳ Lọc Nước Tuần Hoàn</div>
        <div class="form-group"><span>Thời gian CHẠY (phút):</span><input type="number" id="s-fon" min="1" max="999" step="1" value="15"></div>
        <div class="form-group"><span>Thời gian NGHỈ (phút):</span><input type="number" id="s-fof" min="1" max="999" step="1" value="45"></div>
      </div>

      <div class="card">
        <div class="card-title">Hẹn Giờ Đèn LED Chiếu Sáng</div>
        <div class="form-group"><span>Kích hoạt Hẹn giờ Đèn:</span><select id="s-lm"><option value="0">TẮT</option><option value="1">BẬT</option></select></div>
        <div class="form-group"><span>Đèn Tự Bật lúc:</span><input type="time" id="s-lon"></div>
        <div class="form-group"><span>Đèn Tự Tắt lúc:</span><input type="time" id="s-loff"></div>
      </div>

      <div class="card">
        <div class="card-title">Lịch Cho Ăn Tự Động (3 mốc/ngày)</div>
        <div class="form-group">
          <span>Buổi 1:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe1" style="width:75px"><option value="0">TẮT</option><option value="1">BẬT</option></select>
            <input type="text" id="s-ft1" style="width:85px" placeholder="08:00:00">
          </div>
        </div>
        <div class="form-group">
          <span>Buổi 2:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe2" style="width:75px"><option value="0">TẮT</option><option value="1">BẬT</option></select>
            <input type="text" id="s-ft2" style="width:85px" placeholder="12:00:00">
          </div>
        </div>
        <div class="form-group">
          <span>Buổi 3:</span>
          <div style="display:flex;gap:6px">
            <select id="s-fe3" style="width:75px"><option value="0">TẮT</option><option value="1">BẬT</option></select>
            <input type="text" id="s-ft3" style="width:85px" placeholder="18:00:00">
          </div>
        </div>
        <div class="form-group">
          <span>Góc Quay Servo (10° - 180°):</span>
          <input type="number" id="s-fa" min="10" max="180" value="180" style="width:75px">
        </div>
      </div>

      <div class="card">
        <div class="card-title">Kết Nối Mạng WiFi &amp; mDNS</div>
        <div class="form-group"><span>WiFi SSID:</span><input type="text" id="s-ssid" class="wide"></div>
        <div class="form-group"><span>Mật Khẩu WiFi:</span><input type="text" id="s-pass" class="wide"></div>
        <div class="form-group"><span>Tên Miền Local:</span><span style="color:#0284c7;font-weight:bold;font-size:0.9em">http://beca.local</span></div>
        <div class="form-group"><span>Trạng thái WiFi:</span><span id="s-wst" style="font-weight:bold;font-size:0.85em">--</span></div>
      </div>

      <div class="card">
        <div class="card-title">Kết Nối ThingsBoard Cloud (MQTT)</div>
        <div class="form-group"><span>Kích hoạt MQTT:</span><select id="s-mqe"><option value="0">TẮT</option><option value="1">BẬT</option></select></div>
        <div class="form-group"><span>MQTT Server:</span><input type="text" id="s-mqs" class="wide"></div>
        <div class="form-group"><span>Access Token:</span><input type="text" id="s-mqt" class="wide"></div>
        <div class="form-group"><span>Trạng thái MQTT:</span><span id="s-mqst" style="font-weight:bold;font-size:0.85em">--</span></div>
      </div>

      <div class="card">
        <div class="card-title">Cài Đặt Mã Phím Remote IR (Hex)</div>
        <div style="background:#f8fafc;padding:10px;border-radius:8px;margin-bottom:12px;display:flex;justify-content:space-between;align-items:center;border:1px solid #e2e8f0;">
          <span style="font-size:0.86em;color:#475569;font-weight:bold">Mã IR vừa bấm:</span>
          <span id="s-last-ir" style="color:#0284c7;font-weight:800;font-size:1.25em">--</span>
        </div>
        <div class="ir-row"><span class="ir-lbl">1 - Máy Sưởi:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir1"><button class="ir-btn" onclick="ganIR('s-ir1')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">2 - Quạt Làm Mát:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir2"><button class="ir-btn" onclick="ganIR('s-ir2')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">3 - Bơm Bù Nước:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir3"><button class="ir-btn" onclick="ganIR('s-ir3')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">4 - Sục Oxy:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir4"><button class="ir-btn" onclick="ganIR('s-ir4')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">5 - Bơm Rút Nước:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir5"><button class="ir-btn" onclick="ganIR('s-ir5')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">6 - Đèn LED:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir6"><button class="ir-btn" onclick="ganIR('s-ir6')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">7 - Cho Ăn (Servo):</span><div style="display:flex;gap:6px"><input type="text" id="s-ir7"><button class="ir-btn" onclick="ganIR('s-ir7')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">8 - Lọc Nước:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir8"><button class="ir-btn" onclick="ganIR('s-ir8')">GÁN</button></div></div>
        <div class="ir-row"><span class="ir-lbl">0 - Tắt Tất Cả:</span><div style="display:flex;gap:6px"><input type="text" id="s-ir0"><button class="ir-btn" onclick="ganIR('s-ir0')">GÁN</button></div></div>
      </div>

      <button class="save-btn" onclick="saveSettings()">LƯU TOÀN BỘ CÀI ĐẶT</button>
    </div>
  </div>

  <script>
    let _fetching = false;
    let _currentTab = 'dash';
    let _settingsLoaded = false;
    let _rem = { heater: 0, fan: 0, drain: 0, filter: 0, led: 0 };

    function switchTab(id, el) {
      _currentTab = id;
      document.querySelectorAll('.tab').forEach(e => e.classList.remove('active'));
      document.querySelectorAll('.panel').forEach(e => e.classList.remove('active'));
      el.classList.add('active');
      document.getElementById(id).classList.add('active');
      if (id === 'settings' && !_settingsLoaded) f(true);
    }

    function ganIR(inputId) {
      let code = document.getElementById('s-last-ir').textContent;
      if (code && code !== '--' && code.trim().length > 0) {
        let inp = document.getElementById(inputId);
        inp.value = code.replace('0x','').trim();
        inp.focus();
      } else {
        alert('Hãy bấm một nút trên Remote IR trước để hệ thống bắt mã Hex!');
      }
    }

    function formatSec(s) {
      if (s <= 0) return '';
      let h = Math.floor(s / 3600);
      let m = Math.floor((s % 3600) / 60);
      let sec = s % 60;
      let pad = (n) => n < 10 ? '0' + n : n;
      if (h > 0) return '(còn: ' + pad(h) + ':' + pad(m) + ':' + pad(sec) + ')';
      return '(còn: ' + pad(m) + ':' + pad(sec) + ')';
    }

    // Tick đếm ngược client-side mỗi giây
    setInterval(() => {
      ['heater', 'fan', 'drain', 'filter', 'led'].forEach(k => {
        let el = document.getElementById('c-' + k);
        if (_rem[k] > 0) {
          _rem[k]--;
          if (el) el.textContent = formatSec(_rem[k]);
        } else {
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
      const tid = setTimeout(() => ctrl.abort(), 1500);

      fetch('/api/data', {signal: ctrl.signal})
        .then(r => r.json())
        .then(d => {
          clearTimeout(tid);

          // Cảm biến
          document.getElementById('v-wt').textContent   = (d.wt > -50) ? d.wt.toFixed(1) : '--';
          document.getElementById('v-at').textContent   = (d.at > -50) ? d.at.toFixed(1) : '--';
          document.getElementById('v-ah').textContent   = (d.ah > -50) ? d.ah.toFixed(1) : '--';
          document.getElementById('v-wl').textContent   = (d.wcm > 0)  ? d.wcm.toFixed(1) : 'Lỗi';
          document.getElementById('v-time').textContent = d.time;
          document.getElementById('v-rssi').textContent = (d.rssi ? d.rssi : '--') + ' dBm';

          // Trạng thái nút
          setBtn('b-heater', d.h);
          setBtn('b-fan',    d.f);
          setBtn('b-pump',   d.p);
          setBtn('b-oxy',    d.o);
          setBtn('b-drain',  d.d);
          setBtn('b-led',    d.l);
          setBtn('b-filter', d.fl);

          let bfc = document.getElementById('b-filter-cycle');
          if (bfc) {
            bfc.className = d.fcm ? 'btn on' : 'btn off';
            bfc.textContent = d.fcm ? 'CHU KỲ: BẬT' : 'CHU KỲ: TẮT';
          }
          let lblFc = document.getElementById('lbl-filter-cycle');
          if (lblFc) {
            lblFc.textContent = d.fcm ? ('(Chu kỳ: Chạy ' + d.fon + 'p / Nghỉ ' + d.fof + 'p)') : '';
          }

          document.getElementById('oxy-mode-lbl').textContent = d.om ? 'Liên tục' : 'Chu kỳ';

          // Nút hệ thống
          let bSys = document.getElementById('b-sys');
          if (bSys) {
            bSys.className = d.sys ? 'btn on sys-btn' : 'btn off sys-btn';
            bSys.textContent = d.sys ? 'HỆ THỐNG: ĐANG BẬT' : 'HỆ THỐNG: ĐÃ KHÓA (TẮT)';
            bSys.style.background = d.sys ? '#10b981' : '#ef4444';
          }

          // Cập nhật remaining timers
          _rem.heater = d.th_r || 0;
          _rem.fan    = d.tf_r || 0;
          _rem.drain  = d.td_r || 0;
          _rem.filter = d.tfl_r|| 0;
          _rem.led    = d.tl_r || 0;

          setTimerBtn('bt-heater', d.th_a);
          setTimerBtn('bt-fan',    d.tf_a);
          setTimerBtn('bt-drain',  d.td_a);
          setTimerBtn('bt-filter', d.tfl_a);
          setTimerBtn('bt-led',    d.tl_a);

          if (d.last_ir) {
            document.getElementById('s-last-ir').textContent = '0x' + d.last_ir;
          }

          // Form settings load 1 lần
          if (!_settingsLoaded || forceSettings) {
            _settingsLoaded = true;

            document.getElementById('s-ho').value = d.sh_on;
            document.getElementById('s-hf').value = d.sh_off;
            document.getElementById('s-fo').value = d.sf_on;
            document.getElementById('s-ff').value = d.sf_off;

            document.getElementById('s-thh').value = d.th_h;
            document.getElementById('s-twf').value = d.th_wf;
            document.getElementById('s-twl').value = d.th_wl;
            document.getElementById('s-twe').value = d.th_we;
            document.getElementById('s-ap').value  = d.sap ? "1" : "0";

            document.getElementById('s-om').value  = d.om ? "1" : "0";
            document.getElementById('s-oo').value  = d.oo;
            document.getElementById('s-of').value  = d.of;

            document.getElementById('s-fon').value = d.fon;
            document.getElementById('s-fof').value = d.fof;

            document.getElementById('s-lm').value   = d.slm ? "1" : "0";
            document.getElementById('s-lon').value  = d.sl_on;
            document.getElementById('s-loff').value = d.sl_off;

            document.getElementById('s-fe1').value = d.fen1 ? "1" : "0";
            document.getElementById('s-ft1').value = d.ft1;
            document.getElementById('s-fe2').value = d.fen2 ? "1" : "0";
            document.getElementById('s-ft2').value = d.ft2;
            document.getElementById('s-fe3').value = d.fen3 ? "1" : "0";
            document.getElementById('s-ft3').value = d.ft3;
            document.getElementById('s-fa').value  = d.fa;

            document.getElementById('s-ssid').value = d.ssid;
            document.getElementById('s-pass').value = d.pass;

            document.getElementById('s-mqe').value  = d.mqe ? "1" : "0";
            document.getElementById('s-mqs').value  = d.mqs;
            document.getElementById('s-mqt').value  = d.mqt;

            document.getElementById('s-ir1').value = d.ir1;
            document.getElementById('s-ir2').value = d.ir2;
            document.getElementById('s-ir3').value = d.ir3;
            document.getElementById('s-ir4').value = d.ir4;
            document.getElementById('s-ir5').value = d.ir5;
            document.getElementById('s-ir6').value = d.ir6;
            document.getElementById('s-ir7').value = d.ir7;
            document.getElementById('s-ir8').value = d.ir8;
            document.getElementById('s-ir0').value = d.ir0;

            secToHms(d.ths, 'th');
            secToHms(d.tfs, 'tf');
            secToHms(d.tds, 'td');
            secToHms(d.tfls,'tfl');
            secToHms(d.tls, 'tl');
          }

          document.getElementById('s-wst').textContent  = (d.wst === 2) ? ('Đã kết nối (' + d.wip + ')') : 'Mất mạng';
          document.getElementById('s-wst').style.color  = (d.wst === 2) ? '#10b981' : '#ef4444';
          document.getElementById('s-mqst').textContent = d.mqc ? 'ONLINE' : (d.mqe ? 'Đang kết nối...' : 'Đang TẮT');
          document.getElementById('s-mqst').style.color = d.mqc ? '#10b981' : '#f59e0b';
        })
        .catch(() => {})
        .finally(() => { _fetching = false; });
    }

    function setBtn(id, st) {
      let b = document.getElementById(id);
      if (!b) return;
      b.className = st ? 'btn on' : 'btn off';
      b.textContent = st ? 'BẬT' : 'TẮT';
    }

    function setTimerBtn(id, active) {
      let b = document.getElementById(id);
      if (!b) return;
      if (active) b.classList.add('active');
      else b.classList.remove('active');
    }

    function toggle(dev) {
      fetch('/api/ctrl', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({d: dev})
      }).then(() => setTimeout(() => f(false), 200));
    }

    function startTimer(dev) {
      let preMap = { heater: 'th', fan: 'tf', drain: 'td', filter: 'tfl', led: 'tl' };
      let sec = hmsToSec(preMap[dev]);
      fetch('/api/timer', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({d: dev, sec: sec})
      }).then(() => setTimeout(() => f(false), 200));
    }

    function saveSettings() {
      let payload = {
        sh_on:  parseFloat(document.getElementById('s-ho').value),
        sh_off: parseFloat(document.getElementById('s-hf').value),
        sf_on:  parseFloat(document.getElementById('s-fo').value),
        sf_off: parseFloat(document.getElementById('s-ff').value),

        th_h:   parseFloat(document.getElementById('s-thh').value),
        th_wf:  parseFloat(document.getElementById('s-twf').value),
        th_wl:  parseFloat(document.getElementById('s-twl').value),
        th_we:  parseFloat(document.getElementById('s-twe').value),
        sap:    document.getElementById('s-ap').value === "1",

        om:     document.getElementById('s-om').value === "1",
        oo:     parseInt(document.getElementById('s-oo').value),
        of:     parseInt(document.getElementById('s-of').value),

        fon:    parseInt(document.getElementById('s-fon').value),
        fof:    parseInt(document.getElementById('s-fof').value),

        slm:    document.getElementById('s-lm').value === "1",
        sl_on:  document.getElementById('s-lon').value,
        sl_off: document.getElementById('s-loff').value,

        fen1:   document.getElementById('s-fe1').value === "1",
        ft1:    document.getElementById('s-ft1').value,
        fen2:   document.getElementById('s-fe2').value === "1",
        ft2:    document.getElementById('s-ft2').value,
        fen3:   document.getElementById('s-fe3').value === "1",
        ft3:    document.getElementById('s-ft3').value,
        fa:     parseInt(document.getElementById('s-fa').value),

        ths:    hmsToSec('th'),
        tfs:    hmsToSec('tf'),
        tds:    hmsToSec('td'),
        tfls:   hmsToSec('tfl'),
        tls:    hmsToSec('tl'),

        ssid:   document.getElementById('s-ssid').value,
        pass:   document.getElementById('s-pass').value,

        mqe:    document.getElementById('s-mqe').value === "1",
        mqs:    document.getElementById('s-mqs').value,
        mqt:    document.getElementById('s-mqt').value,

        ir1:    document.getElementById('s-ir1').value,
        ir2:    document.getElementById('s-ir2').value,
        ir3:    document.getElementById('s-ir3').value,
        ir4:    document.getElementById('s-ir4').value,
        ir5:    document.getElementById('s-ir5').value,
        ir6:    document.getElementById('s-ir6').value,
        ir7:    document.getElementById('s-ir7').value,
        ir8:    document.getElementById('s-ir8').value,
        ir0:    document.getElementById('s-ir0').value
      };

      fetch('/api/set', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(payload)
      }).then(() => {
        alert('Đã lưu toàn bộ cài đặt thành công!');
        f(true);
      });
    }

    // Polling tự động mỗi 1.5 giây
    f(true);
    setInterval(() => f(false), 1500);
  </script>
</body>
</html>
)rawliteral";

#endif
