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
String sta_ssid = "NONNET";
String sta_password = "123456789";
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

String ir1="45", ir2="46", ir3="47", ir4="44", ir5="40", ir6="43", ir7="07", ir0="16";

// Timer countdown (phút). 0 = Tắt timer.
int timer_heater = 0, timer_fan = 0, timer_pump = 0;
int timer_drain = 30, timer_led = 0;

// ===================== BIẾN HỖ TRỢ TIMER =====================
unsigned long start_heater = 0, start_fan = 0, start_pump = 0;
unsigned long start_drain = 0, start_led = 0;
bool pendingCommand = false;
unsigned long lastMqttPublish = 0;

// ===================== BIẾN HỖ TRỢ WIFI =====================
bool wifiConnecting = false;
unsigned long wifiConnectStart = 0;

#include "index_html.h"

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial2.setTimeout(50);

  loadSettings();

  // Tắt cache tự kết nối ngầm của ESP32 để tránh xung đột kênh AP
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(100);

  WiFi.mode(WIFI_AP_STA);
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  // Tự động bật cờ kết nối ngầm nếu đã có WiFi lưu trong Flash (chạy ngầm 30s, không chặn Web/AP)
  if (sta_ssid.length() > 0) {
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    wifiConnecting = true;
    wifiConnectStart = millis();
    Serial.println("[WIFI] Tu dong ket noi ngam toi: " + sta_ssid + " (Toi da 30s)");
  }

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  setupWeb();
  Serial.println("[MASTER] Khoi dong xong!");
  Serial.print("[MASTER] AP SSID: "); Serial.println(AP_SSID);
  Serial.print("[MASTER] AP IP: "); Serial.println(WiFi.softAPIP());
}

void loadSettings() {
  prefs.begin("beca", false);
  sta_ssid = prefs.getString("ssid", "NONNET");
  sta_password = prefs.getString("pass", "123456789");
  cameraIP = prefs.getString("cam", "");
  ir1 = prefs.getString("ir1", "45");
  ir2 = prefs.getString("ir2", "46");
  ir3 = prefs.getString("ir3", "47");
  ir4 = prefs.getString("ir4", "44");
  ir5 = prefs.getString("ir5", "40");
  ir6 = prefs.getString("ir6", "43");
  ir7 = prefs.getString("ir7", "07");
  ir0 = prefs.getString("ir0", "16");
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
    d["ssid"] = sta_ssid; d["pass"] = sta_password;
    d["wst"] = (WiFi.status() == WL_CONNECTED) ? 2 : (wifiConnecting ? 1 : 0);
    d["wip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    d["cam"] = cameraIP;
    d["ir1"] = ir1; d["ir2"] = ir2; d["ir3"] = ir3; d["ir4"] = ir4;
    d["ir5"] = ir5; d["ir6"] = ir6; d["ir7"] = ir7; d["ir0"] = ir0;
    String resp; serializeJson(d, resp);
    server.send(200, "application/json", resp);
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
    
    th_heater_on = d["sh_on"];
    th_heater_off = d["sh_off"];
    th_fan_on = d["sf_on"];
    th_fan_off = d["sf_off"];
    auto_pump = d["sap"];
    auto_drain = d["sad"];
    th_tank_height = d["th_h"];
    th_water_empty = d["th_we"];
    th_water_low = d["th_wl"];
    th_water_full = d["th_wf"];
    oxyModeContinuous = d["om"];
    led_timer_mode = d["slm"];
    led_on_time = d["sl_on"].as<String>();
    led_off_time = d["sl_off"].as<String>();
    timer_drain = d["td"];
    timer_heater = d["th"];
    timer_fan = d["tf"];
    cameraIP = d["cam"].as<String>();
    
    bool wifiTrigger = false;
    if(d.containsKey("ssid") && d.containsKey("pass")) {
      String ns = d["ssid"].as<String>();
      String np = d["pass"].as<String>();
      if(ns.length() > 0) {
        sta_ssid = ns; sta_password = np;
        wifiTrigger = true;
      }
    }

    if(d.containsKey("ir1")) ir1 = d["ir1"].as<String>();
    if(d.containsKey("ir2")) ir2 = d["ir2"].as<String>();
    if(d.containsKey("ir3")) ir3 = d["ir3"].as<String>();
    if(d.containsKey("ir4")) ir4 = d["ir4"].as<String>();
    if(d.containsKey("ir5")) ir5 = d["ir5"].as<String>();
    if(d.containsKey("ir6")) ir6 = d["ir6"].as<String>();
    if(d.containsKey("ir7")) ir7 = d["ir7"].as<String>();
    if(d.containsKey("ir0")) ir0 = d["ir0"].as<String>();

    prefs.begin("beca", false);
    prefs.putFloat("h_on", th_heater_on); prefs.putFloat("h_off", th_heater_off);
    prefs.putFloat("f_on", th_fan_on); prefs.putFloat("f_off", th_fan_off);
    prefs.putBool("ap", auto_pump); prefs.putBool("ad", auto_drain);
    prefs.putFloat("th_h", th_tank_height); prefs.putFloat("th_we", th_water_empty);
    prefs.putFloat("th_wl", th_water_low); prefs.putFloat("th_wf", th_water_full);
    prefs.putBool("om", oxyModeContinuous);
    prefs.putBool("lm", led_timer_mode);
    prefs.putString("lon", led_on_time); prefs.putString("loff", led_off_time);
    prefs.putInt("td", timer_drain); prefs.putInt("th", timer_heater); prefs.putInt("tf", timer_fan);
    prefs.putString("cam", cameraIP);
    prefs.putString("ssid", sta_ssid); prefs.putString("pass", sta_password);
    prefs.putString("ir1", ir1); prefs.putString("ir2", ir2);
    prefs.putString("ir3", ir3); prefs.putString("ir4", ir4);
    prefs.putString("ir5", ir5); prefs.putString("ir6", ir6);
    prefs.putString("ir7", ir7); prefs.putString("ir0", ir0);
    prefs.end();

    // Gửi map IR mới xuống Slave
    StaticJsonDocument<300> cmd;
    cmd["cmd"] = "ir_map";
    cmd["ir1"] = ir1; cmd["ir2"] = ir2; cmd["ir3"] = ir3; cmd["ir4"] = ir4;
    cmd["ir5"] = ir5; cmd["ir6"] = ir6; cmd["ir7"] = ir7; cmd["ir0"] = ir0;
    String js; serializeJson(cmd, js);
    Serial2.println(js);
    
    sendToSlave(false); // Update oxy mode
    server.send(200, "application/json", "{}");

    if (wifiTrigger) {
      Serial.println("[WIFI] Nhan lenh ket noi tu Web! Bat co ket noi toi: " + sta_ssid);
      WiFi.disconnect();
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
      wifiConnecting = true;
      wifiConnectStart = millis();
    }
  });

  server.onNotFound([]() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("[WEB] Web Server started on port 80");
}

// ===================== LOGIC =====================
void checkLogic() {
  bool c = false;
  unsigned long ms = millis();

  // Cross check Nhiệt
  if(waterTemp > 0) {
    if(waterTemp < th_heater_on && airTemp < th_heater_on && !heaterState) { heaterState=1; start_heater=ms; c=1; Serial.println("[LOGIC] Nhiet do thap -> BAT Suoi"); }
    if(waterTemp >= th_heater_off && heaterState) { heaterState=0; c=1; Serial.println("[LOGIC] Nhiet do dat -> TAT Suoi"); }
    
    if(waterTemp > th_fan_on && airTemp > th_fan_on && !fanState) { fanState=1; start_fan=ms; c=1; Serial.println("[LOGIC] Nhiet do cao -> BAT Quat"); }
    if(waterTemp <= th_fan_off && fanState) { fanState=0; c=1; Serial.println("[LOGIC] Nhiet do dat -> TAT Quat"); }
  }

  // Nước cạn/thấp (HC-SR04)
  bool is_empty = (waterLevelCm > 0 && waterLevelCm < th_water_empty);
  bool is_low   = is_empty || (waterLevelCm > 0 && waterLevelCm < th_water_low);
  bool is_full  = (waterLevelCm > 0 && waterLevelCm >= th_water_full);

  if(is_empty) {
    if(auto_pump && !pumpState) { pumpState=1; c=1; Serial.println("[LOGIC] Nuoc can -> BAT Bom Bu"); }
  } else if (is_full) {
    if(auto_pump && pumpState) { pumpState=0; c=1; Serial.println("[LOGIC] Nuoc day -> TAT Bom Bu"); }
  }

  if(is_low) {
    if(auto_drain && !drainState) { drainState=1; start_drain=ms; c=1; Serial.println("[LOGIC] Nuoc thap -> BAT Bom Thay"); }
  } else if (is_full) {
    if(auto_drain && drainState) { drainState=0; c=1; Serial.println("[LOGIC] Nuoc day -> TAT Bom Thay"); }
  }

  // Hẹn giờ LED
  if(led_timer_mode) {
    String t = getTimeStr();
    if(t == led_on_time && !ledState) { ledState=1; c=1; Serial.println("[LOGIC] Den gio hen -> BAT Den LED"); }
    if(t == led_off_time && ledState) { ledState=0; c=1; Serial.println("[LOGIC] Den gio hen -> TAT Den LED"); }
  }

  // Timer tắt
  if(timer_heater > 0 && heaterState && (ms - start_heater > timer_heater*60000UL)) { heaterState=0; c=1; }
  if(timer_fan > 0 && fanState && (ms - start_fan > timer_fan*60000UL)) { fanState=0; c=1; Serial.println("[LOGIC] Quat het timer -> TAT"); }
  if(timer_drain > 0 && drainState && (ms - start_drain > timer_drain*60000UL)) { drainState=0; c=1; Serial.println("[LOGIC] Bom thay het timer -> TAT"); }

  if(c) {
    Serial.println("[LOGIC] Phat hien thay doi trang thai -> Gui lenh cho Slave");
    sendToSlave(false);
  }
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
    
    Serial.printf("[MASTER] Nhan tu Slave: Nuoc=%.1f KK=%.1f Am=%.1f MucNuoc=%.1fcm\n", 
                  waterTemp, airTemp, airHum, waterLevelCm);
    
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
  String js; serializeJson(d, js);
  Serial.println("[MASTER] Send to Slave: " + js);
  Serial2.println(js);
}

// ===================== MQTT =====================
void mqttReconnect() {
  if(!mqtt.connected() && WiFi.status() == WL_CONNECTED) {
    if(mqtt.connect("ESP_M", MQTT_TOKEN, NULL)) mqtt.subscribe("v1/devices/me/rpc/request/+");
  }
}

void mqttCallback(char* topic, byte* p, unsigned int len) {
  String msg; for(int i=0;i<len;i++) msg+=(char)p[i];
  Serial.println("[MASTER] Nhan tu MQTT: " + msg);
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
  unsigned long ms = millis();

  // Quản lý biến cờ kết nối WiFi (Thời gian chờ tối đa 30s)
  if (wifiConnecting) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnecting = false;
      Serial.print("[WIFI] Ket noi thanh cong! IP: ");
      Serial.println(WiFi.localIP());
      configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
    } else if (ms - wifiConnectStart > 30000) {
      wifiConnecting = false;
      Serial.println("[WIFI] Ket noi that bai (qua 30s)! Da tat co. Dung thu ket noi de on dinh mang AP.");
      WiFi.disconnect();
      WiFi.mode(WIFI_AP_STA);
    }
  }

  server.handleClient();
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
  }
  
  handleSlave();
  checkLogic();

  if (millis() - lastMqttPublish >= 5000) {
    lastMqttPublish = millis();
    
    // Heartbeat để check xem có treo không
    Serial.println("[MASTER] Dang hoat dong... (Web: 192.168.4.1)");
    
    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
      StaticJsonDocument<300> d;
      d["water_temp"]=waterTemp; d["air_temp"]=airTemp;
      d["water_cm"]=waterLevelCm; d["heater"]=heaterState;
      d["fan"]=fanState; d["pump"]=pumpState;
      d["oxy"]=oxyState; d["drain"]=drainState; d["led"]=ledState;
      char b[300]; serializeJson(d, b);
      Serial.println("[MASTER] Send MQTT: " + String(b));
      mqtt.publish("v1/devices/me/telemetry", b);
    }
  }
}
