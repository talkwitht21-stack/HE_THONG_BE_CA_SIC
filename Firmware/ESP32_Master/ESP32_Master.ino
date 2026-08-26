/*
 * ============================================================
 *  ESP32-MASTER FIRMWARE (v2)
 *  Dự án: Hệ thống Giám sát & Điều khiển Tự động Bể Cá
 * ============================================================
 *  Vai trò: Bộ não logic + Gateway Internet
 *    + WiFi STA+AP đồng thời
 *    + NTP Client đồng bộ giờ thực
 *    + Nhận dữ liệu cảm biến & trạng thái 6 relay từ Slave
 *    + Thuật toán Cross-check (ngưỡng tuỳ chỉnh qua Web)
 *    + Xử lý Auto-off timer (đếm ngược tự tắt) cho từng thiết bị
 *    + Web Server: Dashboard + Cài đặt (lưu Flash)
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// ===================== CẤU HÌNH MẠNG =====================
const char* STA_SSID     = "YOUR_WIFI_SSID";
const char* STA_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* AP_SSID     = "BeCa_Control";
const char* AP_PASSWORD = "12345678";

const char* MQTT_SERVER = "demo.thingsboard.io";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOKEN  = "YOUR_DEVICE_ACCESS_TOKEN";

const char* NTP_SERVER  = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 7 * 3600; // GMT+7

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
WebServer server(80);
Preferences prefs;

// ===================== BIẾN TRẠNG THÁI (Từ Slave) =====================
float waterTemp = 0.0, airTemp = 0.0, airHum = 0.0;
float waterLevelCm = 0.0; // Từ HC-SR04

bool heaterState = false, fanState = false, pumpState = false;
bool oxyState = false, drainState = false, ledState = false;
bool oxyModeContinuous = false;

// ===================== BIẾN CÀI ĐẶT (Lưu Flash) =====================
String cameraIP = "";
float th_heater_on = 24.0, th_heater_off = 28.0;
float th_fan_on = 30.0, th_fan_off = 28.0;
bool auto_pump = true, auto_drain = true;
float th_tank_height = 40.0;
float th_water_empty = 10.0; // Khoảng cách tới cạn
float th_water_low = 15.0; // Khoảng cách tới thấp
float th_water_full = 35.0; // Khoảng cách tới đủ

bool led_timer_mode = false;
String led_on_time = "07:00", led_off_time = "21:00";

// Timer countdown (phút). 0 = Tắt timer.
int timer_heater = 0, timer_fan = 0, timer_pump = 0;
int timer_drain = 30, timer_led = 0;

// ===================== BIẾN HỖ TRỢ TIMER =====================
unsigned long start_heater = 0, start_fan = 0, start_pump = 0;
unsigned long start_drain = 0, start_led = 0;
bool pendingCommand = false;
unsigned long lastMqttPublish = 0;

// ===================== TRANG WEB HTML =====================
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
    <h1>🐟 BE CA THONG MINH</h1>
    
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
        <div class="row"><span>🔥 Suoi</span><button class="btn off" id="b-heater" onclick="t('heater')">TAT</button></div>
        <div class="row"><span>🌀 Quat</span><button class="btn off" id="b-fan" onclick="t('fan')">TAT</button></div>
        <div class="row"><span>💧 Bom bu</span><button class="btn off" id="b-pump" onclick="t('pump')">TAT</button></div>
        <div class="row"><span>🫧 Suc Oxy (<span id="oxy-mode-lbl"></span>)</span><button class="btn off" id="b-oxy" onclick="t('oxy')">TAT</button></div>
        <div class="row"><span>🚰 Bom thay</span><button class="btn off" id="b-drain" onclick="t('drain')">TAT</button></div>
        <div class="row"><span>💡 Den LED</span><button class="btn off" id="b-led" onclick="t('led')">TAT</button></div>
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
        <div class="card-title">Camera IP</div>
        <div class="form-group"><span>IP:</span><input type="text" id="s-cam" style="width:150px;"></div>
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
          document.getElementById('s-cam').value = d.cam;
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
        cam: document.getElementById('s-cam').value
      };
      fetch('/api/set', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(data)})
      .then(()=>alert('Da luu!'));
    }

    f(); setInterval(f, 2000);
  </script>
</body>
</html>
)rawliteral";

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  loadSettings();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  WiFi.begin(STA_SSID, STA_PASSWORD);
  
  int r = 0;
  while(WiFi.status() != WL_CONNECTED && r < 20) { delay(500); r++; }
  
  configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  setupWeb();
  Serial.println("[MASTER] Started v2.");
}

void loadSettings() {
  prefs.begin("beca", false);
  cam_ip = prefs.getString("cam", "");
  th_heater_on = prefs.getFloat("h_on", 24.0);
  th_heater_off = prefs.getFloat("h_off", 28.0);
  th_fan_on = prefs.getFloat("f_on", 30.0);
  th_fan_off = prefs.getFloat("f_off", 28.0);
  auto_pump = prefs.getBool("ap", true);
  auto_drain = prefs.getBool("ad", true);
  led_timer_mode = prefs.getBool("lm", false);
  led_on_time = prefs.getString("lon", "07:00");
  led_off_time = prefs.getString("loff", "21:00");
  timer_drain = prefs.getInt("td", 30);
  timer_heater = prefs.getInt("th", 0);
  timer_fan = prefs.getInt("tf", 0);
  oxyModeContinuous = prefs.getBool("om", false);
  th_tank_height = prefs.getFloat("th_h", 40.0);
  th_water_empty = prefs.getFloat("th_we", 10.0);
  th_water_low = prefs.getFloat("th_wl", 15.0);
  th_water_full = prefs.getFloat("th_wf", 35.0);
  prefs.end();
}

String getTimeStr() {
  struct tm ti;
  if(!getLocalTime(&ti)) return "--:--";
  char b[6]; strftime(b, 6, "%H:%M", &ti);
  return String(b);
}

// ===================== WEB SERVER =====================
void setupWeb() {
  server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
  
  server.on("/api/data", []() {
    StaticJsonDocument<512> d;
    d["wt"] = waterTemp; d["at"] = airTemp; d["wcm"] = waterLevelCm;
    d["time"] = getTimeStr();
    d["h"] = heaterState; d["f"] = fanState; d["p"] = pumpState;
    d["o"] = oxyState; d["d"] = drainState; d["l"] = ledState;
    
    d["sh_on"] = th_heater_on; d["sh_off"] = th_heater_off;
    d["sf_on"] = th_fan_on; d["sf_off"] = th_fan_off;
    d["sap"] = auto_pump; d["sad"] = auto_drain;
    d["om"] = oxyModeContinuous; d["slm"] = led_timer_mode;
    d["sl_on"] = led_on_time; d["sl_off"] = led_off_time;
    d["td"] = timer_drain; d["th"] = timer_heater; d["tf"] = timer_fan;
    d["cam"] = cameraIP;

    String s; serializeJson(d, s); server.send(200, "application/json", s);
  });

  server.on("/api/ctrl", HTTP_POST, []() {
    StaticJsonDocument<100> d; deserializeJson(d, server.arg("plain"));
    String dev = d["d"].as<String>();
    bool feed = false;
    
    if(dev=="heater") { heaterState=!heaterState; if(heaterState) start_heater = millis(); }
    if(dev=="fan") { fanState=!fanState; if(fanState) start_fan = millis(); }
    if(dev=="pump") pumpState=!pumpState;
    if(dev=="drain") { drainState=!drainState; if(drainState) start_drain = millis(); }
    if(dev=="led") ledState=!ledState;
    if(dev=="oxy") {
      oxyModeContinuous = false; // Web toggle auto turns off continuous
      oxyState = !oxyState;
    }
    if(dev=="feed") feed = true;
    
    sendToSlave(feed);
    server.send(200, "application/json", "{}");
  });

  server.on("/api/set", HTTP_POST, []() {
    StaticJsonDocument<512> d; deserializeJson(d, server.arg("plain"));
    prefs.begin("beca", false);
    
    th_heater_on = d["sh_on"]; prefs.putFloat("h_on", th_heater_on);
    th_heater_off = d["sh_off"]; prefs.putFloat("h_off", th_heater_off);
    th_fan_on = d["sf_on"]; prefs.putFloat("f_on", th_fan_on);
    th_fan_off = d["sf_off"]; prefs.putFloat("f_off", th_fan_off);
    auto_pump = d["sap"]; prefs.putBool("ap", auto_pump);
    auto_drain = d["sad"]; prefs.putBool("ad", auto_drain);
    oxyModeContinuous = d["om"]; prefs.putBool("om", oxyModeContinuous);
    led_timer_mode = d["slm"]; prefs.putBool("lm", led_timer_mode);
    led_on_time = d["sl_on"].as<String>(); prefs.putString("lon", led_on_time);
    led_off_time = d["sl_off"].as<String>(); prefs.putString("loff", led_off_time);
    timer_drain = d["td"]; prefs.putInt("td", timer_drain);
    timer_heater = d["th"]; prefs.putInt("th", timer_heater);
    timer_fan = d["tf"]; prefs.putInt("tf", timer_fan);
    cameraIP = d["cam"].as<String>(); prefs.putString("cam", cameraIP);
    
    prefs.end();
    sendToSlave(false); // Update oxy mode
    server.send(200, "application/json", "{}");
  });

  server.begin();
}

// ===================== LOGIC =====================
void checkLogic() {
  bool c = false;
  unsigned long ms = millis();

  // Cross check Nhiệt
  if(waterTemp > 0) {
    if(waterTemp < th_heater_on && airTemp < th_heater_on && !heaterState) { heaterState=1; start_heater=ms; c=1; }
    if(waterTemp >= th_heater_off && heaterState) { heaterState=0; c=1; }
    
    if(waterTemp > th_fan_on && airTemp > th_fan_on && !fanState) { fanState=1; start_fan=ms; c=1; }
    if(waterTemp <= th_fan_off && fanState) { fanState=0; c=1; }
  }

  // Nước cạn/thấp (HC-SR04)
  bool is_empty = (waterLevelCm > 0 && waterLevelCm < th_water_empty);
  bool is_low   = is_empty || (waterLevelCm > 0 && waterLevelCm < th_water_low);
  bool is_full  = (waterLevelCm > 0 && waterLevelCm >= th_water_full);

  if(is_empty) {
    if(auto_pump && !pumpState) { pumpState=1; c=1; }
  } else if (is_full) {
    if(auto_pump && pumpState) { pumpState=0; c=1; }
  }

  if(is_low) {
    if(auto_drain && !drainState) { drainState=1; start_drain=ms; c=1; }
  } else if (is_full) {
    if(auto_drain && drainState) { drainState=0; c=1; }
  }

  // Hẹn giờ LED
  if(led_timer_mode) {
    String t = getTimeStr();
    if(t == led_on_time && !ledState) { ledState=1; c=1; }
    if(t == led_off_time && ledState) { ledState=0; c=1; }
  }

  // Timer tắt
  if(timer_heater > 0 && heaterState && (ms - start_heater > timer_heater*60000UL)) { heaterState=0; c=1; }
  if(timer_fan > 0 && fanState && (ms - start_fan > timer_fan*60000UL)) { fanState=0; c=1; }
  if(timer_drain > 0 && drainState && (ms - start_drain > timer_drain*60000UL)) { drainState=0; c=1; }

  if(c) sendToSlave(false);
}

// ===================== UART =====================
void handleSlave() {
  if(Serial2.available()) {
    String s = Serial2.readStringUntil('\n'); s.trim();
    if(s.length()==0) return;
    StaticJsonDocument<300> d; if(deserializeJson(d, s)) return;

    waterTemp = d["water_temp"]; airTemp = d["air_temp"];
    airHum = d["air_hum"];
    if(d.containsKey("water_cm")) waterLevelCm = d["water_cm"];
    
    bool sh = d["heater"], sf = d["fan"], sp = d["pump"];
    bool so = d["oxy"], sd = d["drain"], sl = d["led"];
    bool som = d["oxy_mode"];

    // Update from Slave (IR remote changes)
    if(sh!=heaterState){ heaterState=sh; if(sh) start_heater=millis(); }
    if(sf!=fanState){ fanState=sf; if(sf) start_fan=millis(); }
    if(sd!=drainState){ drainState=sd; if(sd) start_drain=millis(); }
    pumpState=sp; oxyState=so; ledState=sl; oxyModeContinuous=som;
  }
}

void sendToSlave(bool feed) {
  StaticJsonDocument<200> d;
  d["cmd"] = "relay"; d["heater"] = heaterState; d["fan"] = fanState;
  d["pump"] = pumpState; d["oxy"] = oxyState; d["drain"] = drainState;
  d["led"] = ledState; d["oxy_mode"] = oxyModeContinuous;
  if(feed) d["feed"] = 1;
  serializeJson(d, Serial2); Serial2.println();
}

// ===================== MQTT =====================
void mqttReconnect() {
  if(!mqtt.connected() && WiFi.status() == WL_CONNECTED) {
    if(mqtt.connect("ESP_M", MQTT_TOKEN, NULL)) mqtt.subscribe("v1/devices/me/rpc/request/+");
  }
}

void mqttCallback(char* topic, byte* p, unsigned int len) {
  String msg; for(int i=0;i<len;i++) msg+=(char)p[i];
  StaticJsonDocument<200> d; if(deserializeJson(d, msg)) return;
  String m = d["method"].as<String>(); bool v = d["params"].as<bool>();
  
  if(m=="setHeater") { heaterState=v; if(v) start_heater=millis(); }
  else if(m=="setFan") { fanState=v; if(v) start_fan=millis(); }
  else if(m=="setDrain") { drainState=v; if(v) start_drain=millis(); }
  else if(m=="setLed") ledState=v;
  else if(m=="setOxy") { oxyModeContinuous=false; oxyState=v; }
  else if(m=="setFeed" && v) { sendToSlave(true); return; }
  
  sendToSlave(false);
}

// ===================== LOOP =====================
void loop() {
  server.handleClient();
  if(!mqtt.connected()) mqttReconnect();
  mqtt.loop();
  
  handleSlave();
  checkLogic();

  if(millis() - lastMqttPublish >= 5000) {
    lastMqttPublish = millis();
    if(mqtt.connected()) {
      StaticJsonDocument<300> d;
      d["water_temp"]=waterTemp; d["air_temp"]=airTemp;
      d["water_cm"]=waterLevelCm; d["heater"]=heaterState;
      d["fan"]=fanState; d["pump"]=pumpState;
      d["oxy"]=oxyState; d["drain"]=drainState; d["led"]=ledState;
      char b[300]; serializeJson(d, b);
      mqtt.publish("v1/devices/me/telemetry", b);
    }
  }
}
