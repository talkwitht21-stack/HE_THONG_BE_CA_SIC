/*
 * ============================================================================
 *  ESP32-MASTER FIRMWARE (v3 - REBUILD)
 *  Du an: He thong Giam sat & Dieu khien Tu dong Be Ca Thong Minh
 * ============================================================================
 *  Kien truc:
 *    - WiFi Station Mode (WIFI_STA) ket noi truc tiep vao mang nha.
 *    - Web Server tren cong 80 (ho tro mDNS: http://beca.local va IP cuc bo).
 *    - Giao tiep UART2 voi ESP32-Slave (RX2: GPIO 16, TX2: GPIO 17, 9600 baud).
 *    - Logic tu dong: Cross-check nhiet do, muc nuoc sieu am, hen gio den, countdown timer.
 *    - Ho tro MQTT ThingsBoard qua co bat/tat tren Web.
 *    - Nut BOOT (GPIO 0): Nhan giu 3 giay de Factory Reset Flash.
 *    - LED GPIO 2: Nhay chot 300ms khi mat mang, sang dung khi da ket noi.
 * ============================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <time.h>

#include "index_html.h"

// ===================== DINH NGHIA PHAN CUNG =====================
#define PIN_LED_STATUS    2     // LED xanh tren mach ESP32
#define PIN_BOOT_BTN      0     // Nut BOOT (Active LOW)
#define UART_SLAVE_BAUD   9600
#define UART_RX_PIN       16
#define UART_TX_PIN       17

// ===================== CAU HINH MANG & DICH VU =====================
String sta_ssid       = "NONNET";
String sta_password   = "12345678";
String cameraIP       = "";

// MQTT ThingsBoard
bool   mqtt_enabled   = false;
String mqtt_server    = "demo.thingsboard.io";
int    mqtt_port      = 1883;
String mqtt_token     = "";

// NTP Time
const char* NTP_SERVER      = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = 7 * 3600; // GMT+7

// Doi tuong he thong
WiFiClient    wifiClient;
PubSubClient  mqtt(wifiClient);
WebServer     server(80);
Preferences   prefs;

// ===================== BIEN TRANG THAI (Dong bo voi Slave) =====================
float waterTemp       = -999.0;
float airTemp         = -999.0;
float airHum          = -999.0;
float waterLevelCm    = -1.0;

bool heaterState      = false;
bool fanState         = false;
bool pumpState        = false;
bool oxyState         = true;
bool drainState       = false;
bool ledState         = false;
bool oxyModeContinuous= false;

// ===================== BIEN CAI DAT (Luu Flash) =====================
// Nguong nhiet do
float th_heater_on    = 24.0;
float th_heater_off   = 28.0;
float th_fan_on       = 30.0;
float th_fan_off      = 28.0;

// Nguong muc nuoc sieu am
float th_tank_height  = 40.0; // Chieu cao be (cm)
float th_water_empty  = 10.0; // Khoang cach toi mat nuoc khi can (cm)
float th_water_low    = 15.0; // Khoang cach toi mat nuoc khi thap (cm)
float th_water_full   = 35.0; // Khoang cach toi mat nuoc khi day (cm)
bool  auto_pump       = true;  // Tu dong bom bu
bool  auto_drain      = true;  // Tu dong bom thay

// Hen gio Den LED
bool   led_timer_mode = false;
String led_on_time    = "07:00";
String led_off_time   = "21:00";

// Timer countdown tu tat (phut). 0 = khong dung
int timer_heater      = 0;
int timer_fan         = 0;
int timer_drain       = 30;

// Ma phím IR Remote (Hex)
String ir1 = "45", ir2 = "46", ir3 = "47", ir4 = "44";
String ir5 = "40", ir6 = "43", ir7 = "07", ir0 = "16";

// ===================== BIEN THOI GIAN & HE THONG =====================
unsigned long start_heater    = 0;
unsigned long start_fan       = 0;
unsigned long start_drain     = 0;

unsigned long lastWifiAttempt = 0;
unsigned long lastLedBlink    = 0;
bool          wifiLedState    = false;
bool          mdnsStarted     = false;

unsigned long lastMqttAttempt = 0;
unsigned long lastMqttPublish = 0;
unsigned long bootPressStart  = 0;
unsigned long lastLogicCheck  = 0;

// ===================== NGUYEN MAU HAM =====================
void loadSettings();
void saveSettings();
String getTimeStr();
void setupWiFi();
void setupWeb();
void sendToSlave(bool feed);
void sendIRMapToSlave();
void handleSlave();
void checkLogic();
void handleMQTT();
void handleBootButton();
void mqttCallback(char* topic, byte* payload, unsigned int length);

// ===================== HAM SETUP =====================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  
  Serial2.begin(UART_SLAVE_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial2.setTimeout(50);

  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, LOW);

  Serial.println("\n==================================================");
  Serial.println("   HE THONG BE CA THONG MINH - ESP32 MASTER (v3)");
  Serial.println("==================================================");

  loadSettings();
  setupWiFi();

  mqtt.setServer(mqtt_server.c_str(), mqtt_port);
  mqtt.setCallback(mqttCallback);

  setupWeb();

  // Gui ban do IR mac dinh xuong Slave
  delay(200);
  sendIRMapToSlave();
}

// ===================== HAM LOOP =====================
void loop() {
  unsigned long ms = millis();

  // 1. Phuc vu Web Server
  server.handleClient();

  // 2. Doc du lieu tu Slave (UART)
  handleSlave();

  // 3. Kiem tra Logic tu dong moi 1 giay
  if (ms - lastLogicCheck >= 1000) {
    lastLogicCheck = ms;
    checkLogic();
  }

  // 4. Kiem tra nut BOOT Factory Reset
  handleBootButton();

  // 5. Quan ly ket noi WiFi & Den bao LED GPIO 2
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_STATUS, HIGH); // Sang dung khi truc tuyen

    if (!mdnsStarted) {
      if (MDNS.begin("beca")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[mDNS] Da dang ky ten mien: http://beca.local");
        mdnsStarted = true;
      }
    }
  } else {
    mdnsStarted = false;
    
    // Nhay LED 300ms bao loi mat WiFi
    if (ms - lastLedBlink >= 300) {
      lastLedBlink = ms;
      wifiLedState = !wifiLedState;
      digitalWrite(PIN_LED_STATUS, wifiLedState ? HIGH : LOW);
    }

    // Tu dong thu ket noi lai moi 10s
    if (ms - lastWifiAttempt > 10000) {
      lastWifiAttempt = ms;
      Serial.println("[WIFI] Mat ket noi, dang thu ket noi lai toi: " + sta_ssid);
      WiFi.disconnect();
      WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    }
  }

  // 6. Xu ly MQTT (Chi chay khi duoc bat va co token)
  handleMQTT();
}

// ===================== KHOI TAO WIFI =====================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.print("[WIFI] Dang ket noi toi WiFi: ");
  Serial.println(sta_ssid);

  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());

  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 20) {
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_STATUS, HIGH);
    Serial.println("\n[WIFI] DA KET NOI THANH CONG!");
    Serial.print("[WEB]  1. Dia chi IP    : http://"); Serial.println(WiFi.localIP());
    
    configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
    
    if (MDNS.begin("beca")) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("[WEB]  2. Ten mien mDNS : http://beca.local");
      mdnsStarted = true;
    }
    Serial.println("==================================================\n");
  } else {
    Serial.println("[WIFI] Chua ket noi duoc WiFi. He thong tiep tuc chay offline va thu ket noi lai.");
  }
}

// ===================== THIET LAP WEB SERVER =====================
void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.on("/api/data", HTTP_GET, []() {
    StaticJsonDocument<768> d;
    d["wt"]   = waterTemp;
    d["at"]   = airTemp;
    d["ah"]   = airHum;
    d["wcm"]  = waterLevelCm;
    d["time"] = getTimeStr();

    d["h"]    = heaterState;
    d["f"]    = fanState;
    d["p"]    = pumpState;
    d["o"]    = oxyState;
    d["d"]    = drainState;
    d["l"]    = ledState;
    d["om"]   = oxyModeContinuous;

    d["sh_on"]  = th_heater_on;
    d["sh_off"] = th_heater_off;
    d["sf_on"]  = th_fan_on;
    d["sf_off"] = th_fan_off;

    d["th_h"]   = th_tank_height;
    d["th_we"]  = th_water_empty;
    d["th_wl"]  = th_water_low;
    d["th_wf"]  = th_water_full;
    d["sap"]    = auto_pump;
    d["sad"]    = auto_drain;

    d["slm"]    = led_timer_mode;
    d["sl_on"]  = led_on_time;
    d["sl_off"] = led_off_time;

    d["th"]     = timer_heater;
    d["tf"]     = timer_fan;
    d["td"]     = timer_drain;

    d["ssid"]   = sta_ssid;
    d["pass"]   = sta_password;
    d["cam"]    = cameraIP;

    d["mqe"]    = mqtt_enabled;
    d["mqs"]    = mqtt_server;
    d["mqt"]    = mqtt_token;
    d["mqc"]    = mqtt.connected();

    d["wst"]    = (WiFi.status() == WL_CONNECTED) ? 2 : 0;
    d["wip"]    = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";

    d["ir1"]    = ir1; d["ir2"] = ir2; d["ir3"] = ir3; d["ir4"] = ir4;
    d["ir5"]    = ir5; d["ir6"] = ir6; d["ir7"] = ir7; d["ir0"] = ir0;

    String resp;
    serializeJson(d, resp);
    server.send(200, "application/json", resp);
  });

  server.on("/api/ctrl", HTTP_POST, []() {
    StaticJsonDocument<128> d;
    deserializeJson(d, server.arg("plain"));
    String dev = d["d"].as<String>();
    bool feed = false;

    if (dev == "heater") { heaterState = !heaterState; if (heaterState) start_heater = millis(); }
    else if (dev == "fan") { fanState = !fanState; if (fanState) start_fan = millis(); }
    else if (dev == "pump") { pumpState = !pumpState; }
    else if (dev == "drain") { drainState = !drainState; if (drainState) start_drain = millis(); }
    else if (dev == "led") { ledState = !ledState; }
    else if (dev == "oxy") { oxyModeContinuous = false; oxyState = !oxyState; }
    else if (dev == "feed") { feed = true; }

    sendToSlave(feed);
    server.send(200, "application/json", "{}");
  });

  server.on("/api/set", HTTP_POST, []() {
    StaticJsonDocument<768> d;
    deserializeJson(d, server.arg("plain"));

    th_heater_on  = d["sh_on"];
    th_heater_off = d["sh_off"];
    th_fan_on     = d["sf_on"];
    th_fan_off    = d["sf_off"];

    th_tank_height= d["th_h"];
    th_water_empty= d["th_we"];
    th_water_low  = d["th_wl"];
    th_water_full = d["th_wf"];
    auto_pump     = d["sap"];
    auto_drain    = d["sad"];

    oxyModeContinuous = d["om"];
    led_timer_mode    = d["slm"];
    led_on_time       = d["sl_on"].as<String>();
    led_off_time      = d["sl_off"].as<String>();

    timer_heater      = d["th"];
    timer_fan         = d["tf"];
    timer_drain       = d["td"];

    cameraIP          = d["cam"].as<String>();

    bool wifiChanged = false;
    if (d.containsKey("ssid") && d.containsKey("pass")) {
      String ns = d["ssid"].as<String>();
      String np = d["pass"].as<String>();
      if (ns.length() > 0 && (ns != sta_ssid || np != sta_password)) {
        sta_ssid = ns;
        sta_password = np;
        wifiChanged = true;
      }
    }

    if (d.containsKey("mqe")) mqtt_enabled = d["mqe"].as<bool>();
    if (d.containsKey("mqs")) mqtt_server = d["mqs"].as<String>();
    if (d.containsKey("mqt")) mqtt_token = d["mqt"].as<String>();

    if (d.containsKey("ir1")) ir1 = d["ir1"].as<String>();
    if (d.containsKey("ir2")) ir2 = d["ir2"].as<String>();
    if (d.containsKey("ir3")) ir3 = d["ir3"].as<String>();
    if (d.containsKey("ir4")) ir4 = d["ir4"].as<String>();
    if (d.containsKey("ir5")) ir5 = d["ir5"].as<String>();
    if (d.containsKey("ir6")) ir6 = d["ir6"].as<String>();
    if (d.containsKey("ir7")) ir7 = d["ir7"].as<String>();
    if (d.containsKey("ir0")) ir0 = d["ir0"].as<String>();

    saveSettings();
    sendIRMapToSlave();
    sendToSlave(false);

    server.send(200, "application/json", "{}");

    if (wifiChanged) {
      Serial.println("[WIFI] Chuyen sang mang WiFi moi: " + sta_ssid);
      WiFi.disconnect();
      WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    }
  });

  server.onNotFound([]() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("[WEB] Web Server da khoi chay tren cong 80!");
}

// ===================== DOC & LUU FLASH (PREFERENCES) =====================
void loadSettings() {
  prefs.begin("beca", false);
  sta_ssid        = prefs.getString("ssid", "NONNET");
  sta_password    = prefs.getString("pass", "12345678");
  cameraIP        = prefs.getString("cam", "");

  mqtt_enabled    = prefs.getBool("mqe", false);
  mqtt_server     = prefs.getString("mqs", "demo.thingsboard.io");
  mqtt_token      = prefs.getString("mqt", "");

  th_heater_on    = prefs.getFloat("h_on", 24.0);
  th_heater_off   = prefs.getFloat("h_off", 28.0);
  th_fan_on       = prefs.getFloat("f_on", 30.0);
  th_fan_off      = prefs.getFloat("f_off", 28.0);

  th_tank_height  = prefs.getFloat("th_h", 40.0);
  th_water_empty  = prefs.getFloat("th_we", 10.0);
  th_water_low    = prefs.getFloat("th_wl", 15.0);
  th_water_full   = prefs.getFloat("th_wf", 35.0);
  auto_pump       = prefs.getBool("ap", true);
  auto_drain      = prefs.getBool("ad", true);

  oxyModeContinuous = prefs.getBool("om", false);
  led_timer_mode  = prefs.getBool("lm", false);
  led_on_time     = prefs.getString("lon", "07:00");
  led_off_time    = prefs.getString("loff", "21:00");

  timer_heater    = prefs.getInt("th", 0);
  timer_fan       = prefs.getInt("tf", 0);
  timer_drain     = prefs.getInt("td", 30);

  ir1 = prefs.getString("ir1", "45");
  ir2 = prefs.getString("ir2", "46");
  ir3 = prefs.getString("ir3", "47");
  ir4 = prefs.getString("ir4", "44");
  ir5 = prefs.getString("ir5", "40");
  ir6 = prefs.getString("ir6", "43");
  ir7 = prefs.getString("ir7", "07");
  ir0 = prefs.getString("ir0", "16");
  prefs.end();

  Serial.println("[FLASH] Da tai xong cau hinh tu bo nho Flash.");
}

void saveSettings() {
  prefs.begin("beca", false);
  prefs.putString("ssid", sta_ssid);
  prefs.putString("pass", sta_password);
  prefs.putString("cam", cameraIP);

  prefs.putBool("mqe", mqtt_enabled);
  prefs.putString("mqs", mqtt_server);
  prefs.putString("mqt", mqtt_token);

  prefs.putFloat("h_on", th_heater_on);
  prefs.putFloat("h_off", th_heater_off);
  prefs.putFloat("f_on", th_fan_on);
  prefs.putFloat("f_off", th_fan_off);

  prefs.putFloat("th_h", th_tank_height);
  prefs.putFloat("th_we", th_water_empty);
  prefs.putFloat("th_wl", th_water_low);
  prefs.putFloat("th_wf", th_water_full);
  prefs.putBool("ap", auto_pump);
  prefs.putBool("ad", auto_drain);

  prefs.putBool("om", oxyModeContinuous);
  prefs.putBool("lm", led_timer_mode);
  prefs.putString("lon", led_on_time);
  prefs.putString("loff", led_off_time);

  prefs.putInt("th", timer_heater);
  prefs.putInt("tf", timer_fan);
  prefs.putInt("td", timer_drain);

  prefs.putString("ir1", ir1);
  prefs.putString("ir2", ir2);
  prefs.putString("ir3", ir3);
  prefs.putString("ir4", ir4);
  prefs.putString("ir5", ir5);
  prefs.putString("ir6", ir6);
  prefs.putString("ir7", ir7);
  prefs.putString("ir0", ir0);
  prefs.end();

  Serial.println("[FLASH] Da luu cau hinh moi vao Flash.");
}

// ===================== TIEN ICH THOI GIAN =====================
String getTimeStr() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "--:--";
  char b[6];
  strftime(b, 6, "%H:%M", &ti);
  return String(b);
}

// ===================== GIAO TIEP UART VOI SLAVE =====================
void sendToSlave(bool feed) {
  StaticJsonDocument<256> d;
  d["cmd"]      = "relay";
  d["heater"]   = heaterState;
  d["fan"]      = fanState;
  d["pump"]     = pumpState;
  d["oxy"]      = oxyState;
  d["drain"]    = drainState;
  d["led"]      = ledState;
  d["oxy_mode"] = oxyModeContinuous;
  if (feed) d["feed"] = 1;

  String js;
  serializeJson(d, js);
  Serial.println("[MASTER -> SLAVE] " + js);
  Serial2.println(js);
}

void sendIRMapToSlave() {
  StaticJsonDocument<256> cmd;
  cmd["cmd"] = "ir_map";
  cmd["ir1"] = ir1; cmd["ir2"] = ir2; cmd["ir3"] = ir3; cmd["ir4"] = ir4;
  cmd["ir5"] = ir5; cmd["ir6"] = ir6; cmd["ir7"] = ir7; cmd["ir0"] = ir0;

  String js;
  serializeJson(cmd, js);
  Serial.println("[MASTER -> SLAVE IR] " + js);
  Serial2.println(js);
}

void handleSlave() {
  if (Serial2.available()) {
    String s = Serial2.readStringUntil('\n');
    s.trim();
    if (s.length() == 0) return;

    StaticJsonDocument<300> d;
    if (deserializeJson(d, s)) return;

    waterTemp     = d["water_temp"];
    airTemp       = d["air_temp"];
    airHum        = d["air_hum"];
    if (d.containsKey("water_cm")) waterLevelCm = d["water_cm"];

    bool sh = d["heater"];
    bool sf = d["fan"];
    bool sp = d["pump"];
    bool so = d["oxy"];
    bool sd = d["drain"];
    bool sl = d["led"];
    bool som = d["oxy_mode"];

    // Cap nhat neu co su thay doi tu nut bam Remote IR o phia Slave
    if (sh != heaterState) { heaterState = sh; if (sh) start_heater = millis(); }
    if (sf != fanState)    { fanState = sf;    if (sf) start_fan = millis(); }
    if (sd != drainState)  { drainState = sd;  if (sd) start_drain = millis(); }
    pumpState = sp;
    oxyState = so;
    ledState = sl;
    oxyModeContinuous = som;

    Serial.printf("[SLAVE -> MASTER] Nuoc=%.1fC KK=%.1fC Am=%.1f%% MucNuoc=%.1fcm\n",
                  waterTemp, airTemp, airHum, waterLevelCm);
  }
}

// ===================== LOGIC TU DONG =====================
void checkLogic() {
  bool stateChanged = false;
  unsigned long ms = millis();

  // 1. Cross-check Nhiệt độ (Chi chay khi co du lieu hop le > 0)
  if (waterTemp > 0 && airTemp > 0) {
    // Bat suoi khi ca 2 deu lanh
    if (waterTemp < th_heater_on && airTemp < th_heater_on && !heaterState) {
      heaterState = true;
      start_heater = ms;
      stateChanged = true;
      Serial.println("[LOGIC] Nhiet do thap -> BAT Suoi");
    }
    // Tat suoi khi nuoc dat nguong
    if (waterTemp >= th_heater_off && heaterState) {
      heaterState = false;
      stateChanged = true;
      Serial.println("[LOGIC] Nhiet do du -> TAT Suoi");
    }

    // Bat quat khi ca 2 deu nong
    if (waterTemp > th_fan_on && airTemp > th_fan_on && !fanState) {
      fanState = true;
      start_fan = ms;
      stateChanged = true;
      Serial.println("[LOGIC] Nhiet do cao -> BAT Quat");
    }
    // Tat quat khi nuoc da mat
    if (waterTemp <= th_fan_off && fanState) {
      fanState = false;
      stateChanged = true;
      Serial.println("[LOGIC] Nhiet do da mat -> TAT Quat");
    }
  }

  // 2. Logic Muc Nuoc Sieu Am (HC-SR04)
  if (waterLevelCm > 0) {
    bool is_empty = (waterLevelCm < th_water_empty);
    bool is_low   = (waterLevelCm < th_water_low);
    bool is_full  = (waterLevelCm >= th_water_full);

    // Tu dong bom bu
    if (auto_pump) {
      if (is_empty && !pumpState) {
        pumpState = true;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc can -> BAT Bom Bu");
      } else if (is_full && pumpState) {
        pumpState = false;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc day -> TAT Bom Bu");
      }
    }

    // Tu dong bom thay
    if (auto_drain) {
      if (is_low && !drainState) {
        drainState = true;
        start_drain = ms;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc thap -> BAT Bom Thay");
      } else if (is_full && drainState) {
        drainState = false;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc day -> TAT Bom Thay");
      }
    }
  }

  // 3. Hen gio Den LED
  if (led_timer_mode) {
    String t = getTimeStr();
    if (t == led_on_time && !ledState) {
      ledState = true;
      stateChanged = true;
      Serial.println("[LOGIC] Den gio hen -> BAT Den LED");
    }
    if (t == led_off_time && ledState) {
      ledState = false;
      stateChanged = true;
      Serial.println("[LOGIC] Den gio hen -> TAT Den LED");
    }
  }

  // 4. Timer Countdown tu tat
  if (timer_heater > 0 && heaterState && (ms - start_heater >= timer_heater * 60000UL)) {
    heaterState = false;
    stateChanged = true;
    Serial.println("[LOGIC] Suoi het timer -> TAT");
  }
  if (timer_fan > 0 && fanState && (ms - start_fan >= timer_fan * 60000UL)) {
    fanState = false;
    stateChanged = true;
    Serial.println("[LOGIC] Quat het timer -> TAT");
  }
  if (timer_drain > 0 && drainState && (ms - start_drain >= timer_drain * 60000UL)) {
    drainState = false;
    stateChanged = true;
    Serial.println("[LOGIC] Bom thay het timer -> TAT");
  }

  // Gui lenh cap nhat xuong Slave neu co thay doi
  if (stateChanged) {
    sendToSlave(false);
  }
}

// ===================== XU LY NUT BOOT (3S RESET) =====================
void handleBootButton() {
  unsigned long ms = millis();

  if (digitalRead(PIN_BOOT_BTN) == LOW) {
    if (bootPressStart == 0) bootPressStart = ms;
    else if (ms - bootPressStart >= 3000) {
      Serial.println("\n[SYSTEM] !!! GIU NUT BOOT 3S -> FACTORY RESET !!!");
      prefs.begin("beca", false);
      prefs.clear();
      prefs.end();
      Serial.println("[SYSTEM] Da xoa sach Flash! Dang khoi dong lai...");
      delay(500);
      ESP.restart();
    }
  } else {
    bootPressStart = 0;
  }
}

// ===================== XU LY MQTT THINGSBOARD =====================
void handleMQTT() {
  // Neu MQTT bi tat hoac chua co token -> khong lam gi ca
  if (!mqtt_enabled || mqtt_token.length() == 0) return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long ms = millis();

  // Ket noi lai MQTT (Non-blocking moi 15s)
  if (!mqtt.connected()) {
    if (ms - lastMqttAttempt >= 15000) {
      lastMqttAttempt = ms;
      Serial.println("[MQTT] Dang ket noi toi: " + mqtt_server);
      mqtt.setServer(mqtt_server.c_str(), mqtt_port);
      if (mqtt.connect("ESP32_Master", mqtt_token.c_str(), NULL)) {
        Serial.println("[MQTT] Ket noi thanh cong!");
        mqtt.subscribe("v1/devices/me/rpc/request/+");
      } else {
        Serial.printf("[MQTT] Ket noi that bai (rc=%d), se thu lai sau 15s.\n", mqtt.state());
      }
    }
    return;
  }

  mqtt.loop();

  // Gui Telemetry moi 5s
  if (ms - lastMqttPublish >= 5000) {
    lastMqttPublish = ms;

    StaticJsonDocument<300> d;
    d["water_temp"] = waterTemp;
    d["air_temp"]   = airTemp;
    d["air_hum"]    = airHum;
    d["water_cm"]   = waterLevelCm;
    d["heater"]     = heaterState;
    d["fan"]        = fanState;
    d["pump"]       = pumpState;
    d["oxy"]        = oxyState;
    d["drain"]      = drainState;
    d["led"]        = ledState;

    char buf[300];
    serializeJson(d, buf);
    Serial.println("[MQTT SEND] " + String(buf));
    mqtt.publish("v1/devices/me/telemetry", buf);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("[MQTT RECV] " + msg);

  StaticJsonDocument<200> d;
  if (deserializeJson(d, msg)) return;

  String method = d["method"].as<String>();
  bool val = d["params"].as<bool>();

  if (method == "setHeater") { heaterState = val; if (val) start_heater = millis(); }
  else if (method == "setFan") { fanState = val; if (val) start_fan = millis(); }
  else if (method == "setDrain") { drainState = val; if (val) start_drain = millis(); }
  else if (method == "setLed") { ledState = val; }
  else if (method == "setOxy") { oxyModeContinuous = false; oxyState = val; }
  else if (method == "setFeed" && val) { sendToSlave(true); return; }

  sendToSlave(false);
}
