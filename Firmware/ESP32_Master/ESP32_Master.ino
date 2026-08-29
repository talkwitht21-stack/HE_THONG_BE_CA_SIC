/*
 * ============================================================================
 *  ESP32-MASTER FIRMWARE (v3 - REBUILD)
 *  Du an: He thong Giam sat & Dieu khien Tu dong Be Ca Thong Minh
 * ============================================================================
 *  Kien truc:
 *    - WiFi Station Mode (WIFI_STA) ket noi truc tiep vao mang nha.
 *    - Web Server tren cong 80 (truy cap qua dia chi IP cuc bo).
 *    - Giao tiep UART2 voi ESP32-Slave (RX2: GPIO 16, TX2: GPIO 17, 9600 baud).
 *    - Logic tu dong: Cross-check nhiet do, muc nuoc sieu am, hen gio den, countdown timer.
 *    - Cai dat va hoc ma Remote IR truc tiep tren Web.
 *    - Ho tro MQTT ThingsBoard qua co bat/tat tren Web.
 *    - Nut BOOT (GPIO 0): Nhan giu 3 giay de Factory Reset Flash.
 *    - LED GPIO 2: Nhay chot 300ms khi mat mang, sang dung khi da ket noi.
 * ============================================================================
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
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
const char* HOST_NAME = "beca"; // Ten mien Local mac dinh: http://beca.local

// MQTT ThingsBoard
bool   mqtt_enabled   = false;
String mqtt_server    = "thingsboard.cloud";
int    mqtt_port      = 1883;
String mqtt_token     = "";

// NTP Time
const char* NTP_SERVER      = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = 7 * 3600; // GMT+7
unsigned long lastNtpSync   = 0;

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
String last_ir        = "";

bool heaterState      = false;
bool fanState         = false;
bool pumpState        = false;
bool oxyState         = true;
bool drainState       = false;
bool ledState         = false;
bool oxyModeContinuous= false;

// ===================== BIEN CAI DAT (Luu Flash) =====================
// Nguong nhiet do
float th_heater_on    = 18.0; // Bat suoi khi < 18.0C
float th_heater_off   = 20.0; // Tat suoi khi >= 20.0C
float th_fan_on       = 30.0;
float th_fan_off      = 28.0;

// Nguong muc nuoc sieu am (HC-SR04 do tu tren xuong)
// Nuoc CANG DAY -> khoang cach CANG NHO
// Nuoc CANG CAN -> khoang cach CANG LON
float th_tank_height  = 40.0; // Chieu cao be (cm)
float th_water_full   = 10.0; // Khoang cach cam bien khi nuoc DAY (cm) -> Tat bom bu, dung rut nuoc
float th_water_low    = 18.0; // Khoang cach cam bien khi nuoc THAP (cm) -> Bat bom bu tu dong
float th_water_empty  = 28.0; // Khoang cach cam bien khi nuoc CAN NGUY HIEM (cm) -> Cat khan cap bom rut, tat suoi
bool  auto_pump       = true;  // Tu dong bom bu

// Hen gio Den LED
bool   led_timer_mode = false;
String led_on_time    = "07:00";
String led_off_time   = "21:00";

// Timer countdown tu tat (GIAY). 0 = khong dung
uint32_t timer_heater_sec = 0;
uint32_t timer_fan_sec    = 0;
uint32_t timer_drain_sec  = 180; // Mac dinh 3 phut
uint32_t timer_led_sec    = 0;

// Co trang thai hen gio dang chay (Chi bat khi nguoi dung bam nut HEN GIO)
bool timer_heater_active  = false;
bool timer_fan_active     = false;
bool timer_drain_active   = false;
bool timer_led_active     = false;

// Che do Loc Nuoc (Chay song song ca Bom Rut & Bom Bu)
bool filterMode           = false;
bool timer_filter_active  = false;
uint32_t timer_filter_sec = 900; // Mac dinh 15 phut (900s)
bool filterCycleMode      = false;
uint16_t filter_on_min    = 15;
uint16_t filter_off_min   = 45;
unsigned long filterCycleLastChange = 0;
bool filterCyclePhaseOn   = true;

// Co ghi nho do Logic Tu Dong bat (de Auto chi tat nhung gi do chinh no tu bat)
bool heater_auto_triggered= false;
bool fan_auto_triggered   = false;
bool pump_manual_triggered= false;

// Cau hinh chu ky Suc Oxy (phut)
uint16_t oxy_on_min  = 5;
uint16_t oxy_off_min = 15;

// Lich hen gio cho an tu dong (toi da 3 moc/ngay, dinh dang HH:MM:SS)
bool   feed_en_1 = false;  String feed_time_1 = "08:00:00";
bool   feed_en_2 = false;  String feed_time_2 = "12:00:00";
bool   feed_en_3 = false;  String feed_time_3 = "18:00:00";
int    last_feed_day   = -1; // Theo doi ngay de reset cac moc
bool   feed_done_slot1 = false;
bool   feed_done_slot2 = false;
bool   feed_done_slot3 = false;
int feed_angle       = 180; // Goc quay cho an (Khong gioi han goc)

int    led_last_transition_min = -1; // Theo doi moc chuyen trang thai Den LED

// Cong tac tong he thong
bool systemEnabled = true; // false = tat toan bo relay, khoa khong cho bat

// Ma phím IR Remote (Hex)
String ir1 = "45", ir2 = "46", ir3 = "47", ir4 = "44";
String ir5 = "40", ir6 = "43", ir7 = "07", ir8 = "15", ir0 = "16";

// ===================== BIEN THOI GIAN & HE THONG =====================
unsigned long start_heater    = 0;
unsigned long start_fan       = 0;
unsigned long start_pump      = 0;
unsigned long start_drain     = 0;
unsigned long start_led       = 0;
unsigned long start_filter    = 0;
unsigned long autoPumpStart   = 0;
bool          autoPumpTimeoutAlarm = false;
unsigned long lastSlaveContact= 0;

unsigned long lastWifiAttempt = 0;
unsigned long lastLedBlink    = 0;
bool          wifiLedState    = false;

unsigned long lastMqttAttempt = 0;
unsigned long lastMqttPublish = 0;
unsigned long bootPressStart  = 0;
unsigned long lastLogicCheck  = 0;

// ===================== NGUYEN MAU HAM =====================
void loadSettings();
void saveSettings();
String getTimeStr();
int parseTimeToMinutes(String tStr);
bool isTimeInWindow(int curMin, int onMin, int offMin);
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
  mqtt.setBufferSize(1024); // Tăng buffer từ 128 lên 1024 bytes cho JSON Telemetry

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

    // Tu dong cap nhat lai NTP moi 1 gio
    if (ms - lastNtpSync >= 3600000UL) {
      lastNtpSync = ms;
      configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
      Serial.println("[NTP] Da dong bo lai gio he thong.");
    }
  } else {
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
  WiFi.setHostname(HOST_NAME);
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
    Serial.print("[WEB]  Dia chi IP: http://"); Serial.println(WiFi.localIP());

    // Khoi tao mDNS Local DNS: http://beca.local
    if (MDNS.begin(HOST_NAME)) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[mDNS] Ten mien Local: http://%s.local\n", HOST_NAME);
    }

    configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
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
    StaticJsonDocument<1536> d;
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
    d["om"]   = oxyModeContinuous;    d["sys"]  = systemEnabled;
    d["fl"]   = filterMode;
    d["fcm"]  = filterCycleMode;
    d["fon"]  = filter_on_min;
    d["fof"]  = filter_off_min;

    d["sh_on"]  = th_heater_on;
    d["sh_off"] = th_heater_off;
    d["sf_on"]  = th_fan_on;
    d["sf_off"] = th_fan_off;

    d["th_h"]   = th_tank_height;
    d["th_we"]  = th_water_empty;
    d["th_wl"]  = th_water_low;
    d["th_wf"]  = th_water_full;
    d["sap"]    = auto_pump;

    d["slm"]    = led_timer_mode;
    d["sl_on"]  = led_on_time;
    d["sl_off"] = led_off_time;

    // Timer countdown (giay) + thoi gian con lai (giay) - Chi tra ve > 0 khi dang chay timer active
    d["ths"]  = timer_heater_sec;
    d["tfs"]  = timer_fan_sec;
    d["tds"]  = timer_drain_sec;
    d["tls"]  = timer_led_sec;
    d["tfls"] = timer_filter_sec;

    d["th_a"] = timer_heater_active;
    d["tf_a"] = timer_fan_active;
    d["td_a"] = timer_drain_active;
    d["tl_a"] = timer_led_active;
    d["tfl_a"]= timer_filter_active;

    unsigned long ms = millis();
    d["th_r"] = (heaterState && timer_heater_active && timer_heater_sec > 0) ? (long)max(0L, (long)timer_heater_sec - (long)((ms - start_heater)/1000)) : 0;
    d["tf_r"] = (fanState    && timer_fan_active    && timer_fan_sec    > 0) ? (long)max(0L, (long)timer_fan_sec    - (long)((ms - start_fan)   /1000)) : 0;
    d["td_r"] = (drainState  && timer_drain_active  && timer_drain_sec  > 0) ? (long)max(0L, (long)timer_drain_sec  - (long)((ms - start_drain) /1000)) : 0;
    d["tl_r"] = (ledState    && timer_led_active    && timer_led_sec    > 0) ? (long)max(0L, (long)timer_led_sec    - (long)((ms - start_led)   /1000)) : 0;
    d["tfl_r"]= (filterMode  && timer_filter_active && timer_filter_sec > 0) ? (long)max(0L, (long)timer_filter_sec - (long)((ms - start_filter)/1000)) : 0;

    // Cau hinh chu ky Suc Oxy
    d["oo"]   = oxy_on_min;
    d["of"]   = oxy_off_min;

    // Lich hen gio cho an
    d["fen1"] = feed_en_1; d["ft1"] = feed_time_1;
    d["fen2"] = feed_en_2; d["ft2"] = feed_time_2;
    d["fen3"] = feed_en_3; d["ft3"] = feed_time_3;
    d["fa"]   = feed_angle;

    d["ssid"]   = sta_ssid;
    d["pass"]   = sta_password;
    d["hn"]     = HOST_NAME;

    d["mqe"]    = mqtt_enabled;
    d["mqs"]    = mqtt_server;
    d["mqt"]    = mqtt_token;
    d["mqc"]    = mqtt.connected();

    d["wst"]    = (WiFi.status() == WL_CONNECTED) ? 2 : 0;
    d["wip"]    = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    d["rssi"]   = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    d["heap"]   = ESP.getFreeHeap();

    d["ir1"]    = ir1; d["ir2"] = ir2; d["ir3"] = ir3; d["ir4"] = ir4;
    d["ir5"]    = ir5; d["ir6"] = ir6; d["ir7"] = ir7; d["ir8"] = ir8; d["ir0"] = ir0;
    d["last_ir"]= last_ir;

    String resp;
    serializeJson(d, resp);
    server.send(200, "application/json", resp);
  });

  server.on("/api/ctrl", HTTP_POST, []() {
    StaticJsonDocument<128> d;
    deserializeJson(d, server.arg("plain"));
    String dev = d["d"].as<String>();
    bool feed = false;

    if (dev == "system") {
      systemEnabled = !systemEnabled;
      if (!systemEnabled) {
        heaterState = fanState = pumpState = oxyState = drainState = ledState = filterMode = filterCycleMode = false;
        timer_heater_active = timer_fan_active = timer_drain_active = timer_led_active = timer_filter_active = false;
        oxyModeContinuous = false;
      }
      saveSettings();
    } else {
      if (!systemEnabled && dev != "feed") {
        // Neu he thong bi tat, chi cho phep bat lai he thong hoac cho an khan cap
        server.send(200, "application/json", "{}");
        return;
      }
      // Dieu khien Thu Cong: Luon HUY hen gio dem lui de chay thu cong
      if (dev == "heater") {
        heaterState = !heaterState;
        timer_heater_active = false;
        if (heaterState) start_heater = millis();
      }
      else if (dev == "fan") {
        fanState = !fanState;
        timer_fan_active = false;
        if (fanState) start_fan = millis();
      }
      else if (dev == "pump") {
        pumpState = !pumpState;
        if (pumpState) {
          pump_manual_triggered = true;
          autoPumpTimeoutAlarm = false;
          autoPumpStart = millis();
        } else {
          pump_manual_triggered = false;
          autoPumpStart = 0;
          if (filterMode) { filterMode = filterCycleMode = false; timer_filter_active = false; }
        }
      }
      else if (dev == "drain") {
        drainState = !drainState;
        timer_drain_active = false;
        if (!drainState && filterMode) { filterMode = filterCycleMode = false; timer_filter_active = false; }
        if (drainState) start_drain = millis();
      }
      else if (dev == "filter") {
        filterCycleMode = false;
        filterMode = !filterMode;
        timer_filter_active = false;
        drainState = pumpState = filterMode; // Bật hoặc tắt đồng thời cả 2 bơm
        if (filterMode) { start_drain = start_pump = start_filter = millis(); }
      }
      else if (dev == "filter_cycle") {
        filterCycleMode = !filterCycleMode;
        timer_filter_active = false;
        if (filterCycleMode) {
          filterCyclePhaseOn = true;
          filterCycleLastChange = millis();
          filterMode = drainState = pumpState = true;
          start_drain = start_pump = start_filter = millis();
        } else {
          filterMode = drainState = pumpState = false;
        }
        saveSettings();
      }
      else if (dev == "led") {
        ledState = !ledState;
        timer_led_active = false;
        if (ledState) start_led = millis();
      }
      else if (dev == "oxy") {
        oxyState = !oxyState;
      }
      else if (dev == "feed") {
        feed = true;
      }
    }

    sendToSlave(feed);
    server.send(200, "application/json", "{}");
  });

  // Endpoint chuyen dung de KICH HOAT HEN GIO (Co dem lui tu tat)
  server.on("/api/timer", HTTP_POST, []() {
    StaticJsonDocument<256> d;
    deserializeJson(d, server.arg("plain"));
    String dev = d["d"].as<String>();
    uint32_t sec = d["sec"].as<uint32_t>();

    if (!systemEnabled) {
      server.send(200, "application/json", "{}");
      return;
    }

    unsigned long ms = millis();

    if (dev == "heater") {
      heaterState = true;
      timer_heater_active = (sec > 0);
      heater_auto_triggered = false;
      timer_heater_sec = sec;
      start_heater = ms;
      Serial.printf("[TIMER] Bat Hen Gio Suoi: %u giay\n", sec);
    }
    else if (dev == "fan") {
      fanState = true;
      timer_fan_active = (sec > 0);
      fan_auto_triggered = false;
      timer_fan_sec = sec;
      start_fan = ms;
      Serial.printf("[TIMER] Bat Hen Gio Quat: %u giay\n", sec);
    } else if (dev == "drain") {
      drainState = true;
      timer_drain_active = (sec > 0);
      timer_drain_sec = sec;
      start_drain = ms;
      Serial.printf("[TIMER] Bat Hen Gio Bom Rut: %u giay\n", sec);
    } else if (dev == "filter") {
      filterCycleMode = false;
      filterMode = true;
      drainState = pumpState = true; // Chay song song ca 2 bom
      timer_filter_active = (sec > 0);
      timer_filter_sec = sec;
      start_filter = start_pump = start_drain = ms;
      Serial.printf("[TIMER] Bat Hen Gio Loc Nuoc Song Song: %u giay\n", sec);
    } else if (dev == "led") {
      ledState = true;
      timer_led_active = (sec > 0);
      timer_led_sec = sec;
      start_led = ms;
      Serial.printf("[TIMER] Bat Hen Gio Den LED: %u giay\n", sec);
    }

    sendToSlave(false);
    server.send(200, "application/json", "{}");
  });

  server.on("/api/set", HTTP_POST, []() {
    StaticJsonDocument<1024> d;
    deserializeJson(d, server.arg("plain"));

    if (d.containsKey("sh_on"))  th_heater_on  = d["sh_on"];
    if (d.containsKey("sh_off")) th_heater_off = d["sh_off"];
    if (d.containsKey("sf_on"))  th_fan_on     = d["sf_on"];
    if (d.containsKey("sf_off")) th_fan_off    = d["sf_off"];

    if (d.containsKey("th_h"))   th_tank_height= d["th_h"];
    if (d.containsKey("th_we"))  th_water_empty= d["th_we"];
    if (d.containsKey("th_wl"))  th_water_low  = d["th_wl"];
    if (d.containsKey("th_wf"))  th_water_full = d["th_wf"];
    if (d.containsKey("sap"))    auto_pump     = d["sap"];

    if (d.containsKey("om"))     oxyModeContinuous = d["om"];
    if (d.containsKey("oo"))     oxy_on_min        = d["oo"];
    if (d.containsKey("of"))     oxy_off_min       = d["of"];

    if (d.containsKey("slm"))    led_timer_mode    = d["slm"];
    if (d.containsKey("sl_on"))  led_on_time       = d["sl_on"].as<String>();
    if (d.containsKey("sl_off")) led_off_time      = d["sl_off"].as<String>();

    if (d.containsKey("fcm")) {
      bool new_fcm = d["fcm"].as<bool>();
      if (new_fcm != filterCycleMode) {
        filterCycleMode = new_fcm;
        timer_filter_active = false;
        if (filterCycleMode) {
          filterCyclePhaseOn = true;
          filterCycleLastChange = millis();
          filterMode = drainState = pumpState = true;
          start_drain = start_pump = start_filter = millis();
        } else {
          filterMode = drainState = pumpState = false;
        }
      }
    }
    if (d.containsKey("fon")) {
      uint16_t new_fon = d["fon"];
      if (new_fon > 0 && new_fon != filter_on_min) {
        filter_on_min = new_fon;
        filterCycleLastChange = millis();
      }
    }
    if (d.containsKey("fof")) {
      uint16_t new_fof = d["fof"];
      if (new_fof > 0 && new_fof != filter_off_min) {
        filter_off_min = new_fof;
        filterCycleLastChange = millis();
      }
    }

    bool feedChanged = false;
    if (d.containsKey("fen1"))   { feed_en_1   = d["fen1"]; feedChanged = true; }
    if (d.containsKey("ft1"))    { feed_time_1 = d["ft1"].as<String>(); feedChanged = true; }
    if (d.containsKey("fen2"))   { feed_en_2   = d["fen2"]; feedChanged = true; }
    if (d.containsKey("ft2"))    { feed_time_2 = d["ft2"].as<String>(); feedChanged = true; }
    if (d.containsKey("fen3"))   { feed_en_3   = d["fen3"]; feedChanged = true; }
    if (d.containsKey("ft3"))    { feed_time_3 = d["ft3"].as<String>(); feedChanged = true; }
    if (d.containsKey("fa"))     { feed_angle  = d["fa"].as<int>(); feedChanged = true; }
    if (feedChanged) {
      feed_done_slot1 = feed_done_slot2 = feed_done_slot3 = false;
      Serial.println("[FEED] Da cap nhat lich cho an -> Reset co feed_done de co hieu luc ngay!");
    }

    if (d.containsKey("ths"))    timer_heater_sec  = d["ths"];
    if (d.containsKey("tfs"))    timer_fan_sec     = d["tfs"];
    if (d.containsKey("tds"))    timer_drain_sec   = d["tds"];
    if (d.containsKey("tfls"))   timer_filter_sec  = d["tfls"];
    if (d.containsKey("tls"))    timer_led_sec     = d["tls"];

    if (d.containsKey("sys"))    systemEnabled     = d["sys"];

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

    bool irChanged = false;
    if (d.containsKey("ir1")) { ir1 = d["ir1"].as<String>(); irChanged = true; }
    if (d.containsKey("ir2")) { ir2 = d["ir2"].as<String>(); irChanged = true; }
    if (d.containsKey("ir3")) { ir3 = d["ir3"].as<String>(); irChanged = true; }
    if (d.containsKey("ir4")) { ir4 = d["ir4"].as<String>(); irChanged = true; }
    if (d.containsKey("ir5")) { ir5 = d["ir5"].as<String>(); irChanged = true; }
    if (d.containsKey("ir6")) { ir6 = d["ir6"].as<String>(); irChanged = true; }
    if (d.containsKey("ir7")) { ir7 = d["ir7"].as<String>(); irChanged = true; }
    if (d.containsKey("ir8")) { ir8 = d["ir8"].as<String>(); irChanged = true; }
    if (d.containsKey("ir0")) { ir0 = d["ir0"].as<String>(); irChanged = true; }

    saveSettings();
    if (irChanged) {
      sendIRMapToSlave();
    }
    // Cap nhat cau hinh Oxy va Relay xuong Slave
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

  mqtt_enabled    = prefs.getBool("mqe", false);
  mqtt_server     = prefs.getString("mqs", "thingsboard.cloud");
  mqtt_token      = prefs.getString("mqt", "");

  systemEnabled   = prefs.getBool("sys", true);

  th_heater_on    = prefs.getFloat("h_on", 18.0);
  th_heater_off   = prefs.getFloat("h_off", 20.0);
  th_fan_on       = prefs.getFloat("f_on", 30.0);
  th_fan_off      = prefs.getFloat("f_off", 28.0);

  th_tank_height  = prefs.getFloat("th_h", 40.0);
  th_water_full   = prefs.getFloat("th_wf", 10.0);
  th_water_low    = prefs.getFloat("th_wl", 18.0);
  th_water_empty  = prefs.getFloat("th_we", 28.0);
  auto_pump       = prefs.getBool("ap", true);

  oxyModeContinuous = prefs.getBool("om", false);
  oxy_on_min      = prefs.getUShort("oo", 5);
  oxy_off_min     = prefs.getUShort("of", 15);

  led_timer_mode  = prefs.getBool("lm", false);
  led_on_time     = prefs.getString("lon", "07:00");
  led_off_time    = prefs.getString("loff", "21:00");

  feed_en_1       = prefs.getBool("fe1", false);
  feed_time_1     = prefs.getString("ft1", "08:00:00");
  feed_en_2       = prefs.getBool("fe2", false);
  feed_time_2     = prefs.getString("ft2", "12:00:00");
  feed_en_3       = prefs.getBool("fe3", false);
  feed_time_3     = prefs.getString("ft3", "18:00:00");
  feed_angle      = prefs.getInt("fa", 180);

  timer_heater_sec= prefs.getUInt("ths", 0);
  timer_fan_sec   = prefs.getUInt("tfs", 0);
  timer_drain_sec = prefs.getUInt("tds", 180);
  timer_filter_sec= prefs.getUInt("tfls", 900);
  timer_led_sec   = prefs.getUInt("tls", 0);

  filterCycleMode = prefs.getBool("fcm", false);
  filter_on_min   = prefs.getUShort("fon", 15);
  filter_off_min  = prefs.getUShort("fof", 45);

  ir1 = prefs.getString("ir1", "45");
  ir2 = prefs.getString("ir2", "46");
  ir3 = prefs.getString("ir3", "47");
  ir4 = prefs.getString("ir4", "44");
  ir5 = prefs.getString("ir5", "40");
  ir6 = prefs.getString("ir6", "43");
  ir7 = prefs.getString("ir7", "07");
  ir8 = prefs.getString("ir8", "15");
  ir0 = prefs.getString("ir0", "16");
  prefs.end();

  Serial.println("[FLASH] Da tai xong cau hinh tu bo nho Flash.");
}

void saveSettings() {
  // Validate va rang buoc thong so hop ly
  if (th_heater_off <= th_heater_on) th_heater_off = th_heater_on + 1.0;
  if (th_fan_on <= th_fan_off) th_fan_on = th_fan_off + 1.0;
  if (th_water_low <= th_water_full) th_water_low = th_water_full + 5.0;
  if (th_water_empty <= th_water_low) th_water_empty = th_water_low + 5.0;

  prefs.begin("beca", false);
  prefs.putString("ssid", sta_ssid);
  prefs.putString("pass", sta_password);

  prefs.putBool("mqe", mqtt_enabled);
  prefs.putString("mqs", mqtt_server);
  prefs.putString("mqt", mqtt_token);

  prefs.putBool("sys", systemEnabled);

  prefs.putFloat("h_on", th_heater_on);
  prefs.putFloat("h_off", th_heater_off);
  prefs.putFloat("f_on", th_fan_on);
  prefs.putFloat("f_off", th_fan_off);

  prefs.putFloat("th_h", th_tank_height);
  prefs.putFloat("th_we", th_water_empty);
  prefs.putFloat("th_wl", th_water_low);
  prefs.putFloat("th_wf", th_water_full);
  prefs.putBool("ap", auto_pump);

  prefs.putBool("om", oxyModeContinuous);
  prefs.putUShort("oo", oxy_on_min);
  prefs.putUShort("of", oxy_off_min);

  prefs.putBool("lm", led_timer_mode);
  prefs.putString("lon", led_on_time);
  prefs.putString("loff", led_off_time);

  prefs.putBool("fe1", feed_en_1);
  prefs.putString("ft1", feed_time_1);
  prefs.putBool("fe2", feed_en_2);
  prefs.putString("ft2", feed_time_2);
  prefs.putBool("fe3", feed_en_3);
  prefs.putString("ft3", feed_time_3);
  prefs.putInt("fa", feed_angle);

  prefs.putUInt("ths", timer_heater_sec);
  prefs.putUInt("tfs", timer_fan_sec);
  prefs.putUInt("tds", timer_drain_sec);
  prefs.putUInt("tfls", timer_filter_sec);
  prefs.putUInt("tls", timer_led_sec);

  prefs.putBool("fcm", filterCycleMode);
  prefs.putUShort("fon", filter_on_min);
  prefs.putUShort("fof", filter_off_min);

  prefs.putString("ir1", ir1);
  prefs.putString("ir2", ir2);
  prefs.putString("ir3", ir3);
  prefs.putString("ir4", ir4);
  prefs.putString("ir5", ir5);
  prefs.putString("ir6", ir6);
  prefs.putString("ir7", ir7);
  prefs.putString("ir8", ir8);
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

int parseTimeToMinutes(String tStr) {
  tStr.trim();
  int colon1 = tStr.indexOf(':');
  if (colon1 == -1) return -1;
  int colon2 = tStr.indexOf(':', colon1 + 1);
  int h = tStr.substring(0, colon1).toInt();
  int m = (colon2 != -1) ? tStr.substring(colon1 + 1, colon2).toInt() : tStr.substring(colon1 + 1).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 60 + m;
}

bool isTimeInWindow(int curMin, int onMin, int offMin) {
  if (onMin == offMin || onMin < 0 || offMin < 0) return false;
  if (onMin < offMin) {
    // Trong cung 1 ngay, vi du 07:00 (420) -> 21:00 (1260)
    return (curMin >= onMin && curMin < offMin);
  } else {
    // Qua dem, vi du 22:00 (1320) -> 06:00 (360) sang hom sau
    return (curMin >= onMin || curMin < offMin);
  }
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
  d["oo"]       = oxy_on_min;
  d["of"]       = oxy_off_min;
  d["fa"]       = feed_angle;
  if (feed) d["feed"] = true;

  String js;
  serializeJson(d, js);
  Serial.println("[MASTER -> SLAVE] " + js);
  Serial2.println(js);
}

void sendIRMapToSlave() {
  StaticJsonDocument<256> cmd;
  cmd["cmd"] = "ir_map";
  cmd["ir1"] = ir1; cmd["ir2"] = ir2; cmd["ir3"] = ir3; cmd["ir4"] = ir4;
  cmd["ir5"] = ir5; cmd["ir6"] = ir6; cmd["ir7"] = ir7; cmd["ir8"] = ir8; cmd["ir0"] = ir0;

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

    StaticJsonDocument<512> d;
    if (deserializeJson(d, s)) return;

    // Kiem tra neu Slave gui PING Heartbeat (kiem tra 15s)
    if (d.containsKey("cmd") && d["cmd"].as<String>() == "ping") {
      Serial2.println("{\"cmd\":\"pong\"}");
      lastSlaveContact = millis();
      Serial.println("[MASTER] Nhan PING tu Slave -> Da tra PONG ngay lap tuc");
      return;
    }

    lastSlaveContact = millis();

    waterTemp     = d["water_temp"];
    airTemp       = d["air_temp"];
    airHum        = d["air_hum"];
    if (d.containsKey("water_cm")) waterLevelCm = d["water_cm"];

    // CHI DONG BO RELAY TU SLAVE KHI CO SU KIEN BAM PHIM REMOTE IR
    // (Tranh viec cac ban tin cam bien dinh ky 2s ghi de trang thai relay cua Master)
    if (d.containsKey("last_ir") && d["last_ir"].as<String>().length() > 0) {
      last_ir = d["last_ir"].as<String>();

      bool sh = d["heater"];
      bool sf = d["fan"];
      bool sp = d["pump"];
      bool so = d["oxy"];
      bool sd = d["drain"];
      bool sl = d["led"];
      bool som = d["oxy_mode"];

      heaterState = sh; timer_heater_active = false; heater_auto_triggered = false; if (sh) start_heater = millis();
      fanState    = sf; timer_fan_active    = false; fan_auto_triggered    = false; if (sf) start_fan    = millis();
      pumpState   = sp;
      if (sp) { pump_manual_triggered = true; autoPumpTimeoutAlarm = false; autoPumpStart = millis(); }
      else { pump_manual_triggered = false; autoPumpStart = 0; }
      oxyState    = so;
      drainState  = sd; timer_drain_active  = false; if (sd) start_drain  = millis();
      ledState    = sl; timer_led_active    = false; if (sl) start_led    = millis();
      oxyModeContinuous = som;
      filterMode  = (sp && sd); // Dong bo filterMode khi ca 2 bom deu bat
      filterCycleMode = false;  // Huy chu ky tu dong khi nguoi dung bam remote IR
      timer_filter_active = false;
      if (filterMode) start_filter = start_pump = start_drain = millis();

      Serial.printf("[MASTER] Nhan su kien IR tu Slave: %s -> Da dong bo relay (Filter: %d)\n", last_ir.c_str(), filterMode);
    }

    Serial.printf("[SLAVE -> MASTER] Nuoc=%.1fC KK=%.1fC Am=%.1f%% MucNuoc=%.1fcm\n",
                  waterTemp, airTemp, airHum, waterLevelCm);
  }
}

// ===================== LOGIC TU DONG =====================
void checkLogic() {
  bool stateChanged = false;
  unsigned long ms = millis();

  // 0. Neu HE THONG BI TAT (Kill Switch), khoa tat ca relay
  if (!systemEnabled) {
    if (heaterState || fanState || pumpState || oxyState || drainState || ledState || filterMode || filterCycleMode) {
      heaterState = fanState = pumpState = oxyState = drainState = ledState = filterMode = filterCycleMode = false;
      timer_heater_active = timer_fan_active = timer_drain_active = timer_led_active = timer_filter_active = false;
      oxyModeContinuous = false;
      sendToSlave(false);
      Serial.println("[LOGIC] He thong dang bi KHOA -> Tat tat ca relay");
    }
    return;
  }

  // ===================== 1. FAILSAFE NGUY HIEM TUYET DOI =====================
  // KHI NGUY HIEM: DU NGUOI DUNG CO BAT BANG TAY / REMOTE / WEB CUNG PHAI CAT NGAY LAP TUC!
  // Gioi han <= th_tank_height + 5.0cm de tranh truong hop de board tren ban test ngoai phong
  bool is_empty = (waterLevelCm >= th_water_empty && waterLevelCm > 0 && waterLevelCm <= th_tank_height + 5.0);
  bool is_overheat = (waterTemp >= 35.0);

  // [FAILSAFE NGUY HIEM]: Qua nhiet >= 35C hoac Can nuoc -> BAT BUOC CAT SUOI NGAY DANG BAT BANG BAT KY CACH NAO
  if (is_overheat || is_empty) {
    if (heaterState) {
      heaterState = false;
      timer_heater_active = false;
      stateChanged = true;
      Serial.println("[FAILSAFE NGUY HIEM] !!! CAT KHAN CAP SUOI (Qua nhiet >= 35C hoac Can nuoc) !!!");
    }
  }

  // [FAILSAFE NGUY HIEM]: Can nuoc -> BAT BUOC CAT BOM RUT VA CHE DO LOC NUOC NGAY
  if (is_empty) {
    if (drainState || filterMode || filterCycleMode) {
      drainState = false;
      filterMode = false;
      filterCycleMode = false;
      timer_drain_active = false;
      timer_filter_active = false;
      stateChanged = true;
      Serial.println("[FAILSAFE NGUY HIEM] !!! CAT BOM RUT & LOC NUOC (Nuoc can nguy hiem) !!!");
    }
  }

  // ===================== 2. AUTO LOGIC NHIET DO (KHI DUOI 35C) =====================
  if (waterTemp > 0.0 && waterTemp < 55.0 && !is_overheat && !is_empty) {
    // --- DIEU KHIEN SUOI TU DONG (Dua tren nhiet do NUOC DS18B20) ---
    if (waterTemp < th_heater_on) {
      if (!heaterState && !timer_heater_active) {
        heaterState = true;
        start_heater = ms;
        stateChanged = true;
        Serial.printf("[LOGIC AUTO] Nuoc lanh (%.1fC < %.1fC) -> Tu dong BAT Suoi\n", waterTemp, th_heater_on);
      }
    } else if (waterTemp >= th_heater_off) {
      if (heaterState) {
        heaterState = false;
        timer_heater_active = false;
        stateChanged = true;
        Serial.printf("[LOGIC AUTO] Nuoc da du am (%.1fC >= %.1fC) -> Tu dong TAT Suoi\n", waterTemp, th_heater_off);
      }
    }

    // --- DIEU KHIEN QUAT TU DONG (Dua tren nhiet do NUOC DS18B20) ---
    if (waterTemp > th_fan_on) {
      if (!fanState && !timer_fan_active) {
        fanState = true;
        start_fan = ms;
        stateChanged = true;
        Serial.printf("[LOGIC AUTO] Nuoc nong (%.1fC > %.1fC) -> Tu dong BAT Quat\n", waterTemp, th_fan_on);
      }
    } else if (waterTemp <= th_fan_off) {
      if (fanState) {
        fanState = false;
        timer_fan_active = false;
        stateChanged = true;
        Serial.printf("[LOGIC AUTO] Nuoc da mat (%.1fC <= %.1fC) -> Tu dong TAT Quat\n", waterTemp, th_fan_off);
      }
    }
  }

  // ===================== 3. LOGIC MUC NUOC SIEU AM =====================
  // Khoang cach NHO = nuoc DAY | Khoang cach LON = nuoc CAN
  if (waterLevelCm > 0) {
    bool is_low   = (waterLevelCm >= th_water_low);   // Xa vua -> nuoc thap, can bom bu
    bool is_full  = (waterLevelCm <= th_water_full);  // Rat gan -> nuoc day, dung bom bu

    // Tu dong bom bu nuoc (chi chay khi KHONG trong che do loc nuoc song song)
    if (auto_pump && !filterMode && !filterCycleMode) {
      // 1. Khi nuoc thap -> Tu dong bat bom bu
      if (is_low && !pumpState && !autoPumpTimeoutAlarm) {
        pumpState = true;
        autoPumpStart = ms;
        pump_manual_triggered = false;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc thap -> BAT Bom Bu Tu Dong");
      }
      // 2. Khi nuoc day -> Ngat bom bu (chong tran) va reset toan bo co
      else if (is_full && pumpState) {
        pumpState = false;
        autoPumpStart = 0;
        autoPumpTimeoutAlarm = false;
        pump_manual_triggered = false;
        stateChanged = true;
        Serial.println("[LOGIC] Nuoc day -> TAT Bom Bu (chong tran)");
      }

      // [FAILSAFE NGUY HIEM]: Bom bu chay lien tuc qua 10 phut (600s) ma chua day -> Ngat khan cap
      if (pumpState && autoPumpStart > 0 && (ms - autoPumpStart >= 600000UL)) {
        pumpState = false;
        autoPumpTimeoutAlarm = true;
        pump_manual_triggered = false;
        stateChanged = true;
        Serial.println("[FAILSAFE NGUY HIEM] !!! BOM BU CHAY QUA 10 PHUT -> NGAT KHAN CAP CHONG TRAN / CHAY BOM !!!");
      }
    }
  }

  // ===================== 4. HEN GIO DEN LED THEO LICH (EDGE-TRIGGER) =====================
  struct tm ti;
  bool hasTime = getLocalTime(&ti, 50); // Timeout 50ms tranh blocking
  if (hasTime) {
    int curMin = ti.tm_hour * 60 + ti.tm_min;
    int curDay = ti.tm_yday;

    if (led_timer_mode) {
      int onMin  = parseTimeToMinutes(led_on_time);
      int offMin = parseTimeToMinutes(led_off_time);

      if (onMin >= 0 && offMin >= 0 && onMin != offMin) {
        // Chi chuyen doi trang thai dung vao phut chuyen tiep moc gio
        if (curMin == onMin && led_last_transition_min != onMin) {
          led_last_transition_min = onMin;
          ledState = true;
          start_led = ms;
          stateChanged = true;
          Serial.printf("[LOGIC] Lich Den LED: Dung moc gio BAT (%02d:%02d) -> BAT Den LED\n", ti.tm_hour, ti.tm_min);
        } else if (curMin == offMin && led_last_transition_min != offMin) {
          led_last_transition_min = offMin;
          ledState = false;
          timer_led_active = false;
          stateChanged = true;
          Serial.printf("[LOGIC] Lich Den LED: Dung moc gio TAT (%02d:%02d) -> TAT Den LED\n", ti.tm_hour, ti.tm_min);
        } else if (curMin != onMin && curMin != offMin) {
          // Giai phong co san sang cho moc tiep theo
          if (led_last_transition_min == onMin || led_last_transition_min == offMin) {
            led_last_transition_min = -1;
          }
        }
      }
    }

    // ===================== 5. HEN GIO CHO AN TU DONG (NON-BLOCKING) =====================
    // Tu dong reset sach se tat ca cac moc cho an khi sang ngay moi
    if (curDay != last_feed_day) {
      last_feed_day = curDay;
      feed_done_slot1 = feed_done_slot2 = feed_done_slot3 = false;
      Serial.printf("[LOGIC] Sang ngay moi (Day %d) -> Reset toan bo cac moc cho an\n", curDay);
    }

    auto checkFeed = [&](bool en, String tStr, bool &feedDone, int slot) {
      if (!en || feedDone) return;
      int fMin = parseTimeToMinutes(tStr);
      if (fMin >= 0 && curMin == fMin) {
        feedDone = true;
        sendToSlave(true);
        Serial.printf("[LOGIC] Hen gio Cho An MOC %d (%s) -> Kich hoat Servo xan moi\n", slot, tStr.c_str());
      }
    };

    checkFeed(feed_en_1, feed_time_1, feed_done_slot1, 1);
    checkFeed(feed_en_2, feed_time_2, feed_done_slot2, 2);
    checkFeed(feed_en_3, feed_time_3, feed_done_slot3, 3);
  }

  // ===================== 6. TIMER COUNTDOWN TU TAT =====================
  if (timer_heater_active && heaterState && (ms - start_heater >= timer_heater_sec * 1000UL)) {
    heaterState = false;
    timer_heater_active = false;
    stateChanged = true;
    Serial.println("[LOGIC] Suoi het timer -> TAT");
  }
  if (timer_fan_active && fanState && (ms - start_fan >= timer_fan_sec * 1000UL)) {
    fanState = false;
    timer_fan_active = false;
    stateChanged = true;
    Serial.println("[LOGIC] Quat het timer -> TAT");
  }
  if (timer_drain_active && drainState && (ms - start_drain >= timer_drain_sec * 1000UL)) {
    drainState = false;
    timer_drain_active = false;
    stateChanged = true;
    Serial.println("[LOGIC] Bom rut het timer -> TAT");
  }
  if (timer_filter_active && filterMode && (ms - start_filter >= timer_filter_sec * 1000UL)) {
    filterMode = false;
    drainState = false;
    pumpState = false;
    timer_filter_active = false;
    stateChanged = true;
    Serial.println("[LOGIC] Loc nuoc song song het timer -> TAT");
  }
  if (timer_led_active && ledState && (ms - start_led >= timer_led_sec * 1000UL)) {
    ledState = false;
    timer_led_active = false;
    stateChanged = true;
    Serial.println("[LOGIC] Den LED het timer -> TAT");
  }

  // ===================== 7. CHU KY LOC NUOC TU DONG =====================
  if (filterCycleMode && !is_empty) {
    unsigned long onMs  = (unsigned long)max(1, (int)filter_on_min) * 60000UL;
    unsigned long offMs = (unsigned long)max(1, (int)filter_off_min) * 60000UL;

    if (filterCyclePhaseOn) {
      if (!filterMode || !drainState || !pumpState) {
        filterMode = drainState = pumpState = true;
        stateChanged = true;
      }
      if (ms - filterCycleLastChange >= onMs) {
        filterCyclePhaseOn = false;
        filterCycleLastChange = ms;
        filterMode = drainState = pumpState = false;
        stateChanged = true;
        Serial.printf("[LOGIC] Chu ky Loc Nuoc: Het %u phut CHAY -> Chuyen sang NGHI (%u phut)\n", filter_on_min, filter_off_min);
      }
    } else {
      if (filterMode || drainState || pumpState) {
        filterMode = drainState = pumpState = false;
        stateChanged = true;
      }
      if (ms - filterCycleLastChange >= offMs) {
        filterCyclePhaseOn = true;
        filterCycleLastChange = ms;
        filterMode = drainState = pumpState = true;
        stateChanged = true;
        Serial.printf("[LOGIC] Chu ky Loc Nuoc: Het %u phut NGHI -> Chuyen sang CHAY (%u phut)\n", filter_off_min, filter_on_min);
      }
    }
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
      mqtt.setBufferSize(1024); // Đảm bảo buffer 1024 bytes cho packet lớn
      if (mqtt.connect("ESP32_Master_BeCa", mqtt_token.c_str(), NULL)) {
        Serial.println("[MQTT] Ket noi thanh cong!");
        mqtt.subscribe("v1/devices/me/rpc/request/+");

        // Gui Client Attributes len ThingsBoard khi vua ket noi
        StaticJsonDocument<256> attr;
        attr["firmware_version"] = "v3.1";
        attr["device_name"]      = "ESP32_Master_BeCa";
        attr["mdns_url"]         = "http://beca.local";
        attr["mac_address"]      = WiFi.macAddress();
        attr["local_ip"]         = WiFi.localIP().toString();
        char attrBuf[256];
        serializeJson(attr, attrBuf);
        mqtt.publish("v1/devices/me/attributes", attrBuf);
      } else {
        Serial.printf("[MQTT] Ket noi that bai (rc=%d), se thu lai sau 15s.\n", mqtt.state());
      }
    }
    return;
  }

  mqtt.loop();

  // Gui Telemetry day du moi 5s
  if (ms - lastMqttPublish >= 5000) {
    lastMqttPublish = ms;

    StaticJsonDocument<512> d;
    d["water_temp"]   = (waterTemp > -500.0) ? waterTemp : 0.0;
    d["air_temp"]     = (airTemp > -500.0)   ? airTemp   : 0.0;
    d["air_hum"]      = (airHum > -500.0)    ? airHum    : 0.0;
    d["water_cm"]     = waterLevelCm;
    d["heater"]       = heaterState;
    d["fan"]          = fanState;
    d["pump"]         = pumpState;
    d["oxy"]          = oxyState;
    d["drain"]        = drainState;
    d["led"]          = ledState;
    d["filter"]       = filterMode;
    d["filter_cycle"] = filterCycleMode;
    d["system"]       = systemEnabled;
    d["rssi"]         = WiFi.RSSI();
    d["free_heap"]    = ESP.getFreeHeap();
    d["ip"]           = WiFi.localIP().toString();
    if (last_ir.length() > 0) d["last_ir"] = last_ir;

    char buf[512];
    serializeJson(d, buf);
    bool ok = mqtt.publish("v1/devices/me/telemetry", buf);
    if (ok) {
      Serial.println("[MQTT TELEMETRY OK] " + String(buf));
    } else {
      Serial.println("[MQTT TELEMETRY FAILED] Packet qua dai hoac socket bi loi!");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("[MQTT RECV] " + msg);

  StaticJsonDocument<256> d;
  if (deserializeJson(d, msg)) return;

  String method = d["method"].as<String>();
  bool val = d["params"].as<bool>();
  bool feedTriggered = false;

  if (method == "setHeater") { 
    heaterState = val; 
    timer_heater_active = false; 
    if (val) start_heater = millis(); 
  }
  else if (method == "setFan") { 
    fanState = val; 
    timer_fan_active = false; 
    if (val) start_fan = millis(); 
  }
  else if (method == "setPump") { 
    pumpState = val; 
    if (pumpState) {
      pump_manual_triggered = true;
      autoPumpTimeoutAlarm = false;
      autoPumpStart = millis();
    } else {
      pump_manual_triggered = false;
      autoPumpStart = 0;
      if (filterMode) { filterMode = filterCycleMode = false; timer_filter_active = false; }
    }
  }
  else if (method == "setDrain") { 
    drainState = val; 
    timer_drain_active = false; 
    if (!drainState && filterMode) { filterMode = filterCycleMode = false; timer_filter_active = false; } 
    if (drainState) start_drain = millis(); 
  }
  else if (method == "setLed") { 
    ledState = val; 
    timer_led_active = false; 
    if (val) start_led = millis(); 
  }
  else if (method == "setOxy") { 
    oxyModeContinuous = false; 
    oxyState = val; 
  }
  else if (method == "setFilter") {
    filterCycleMode = false;
    filterMode = val;
    timer_filter_active = false;
    drainState = pumpState = val;
    if (val) start_drain = start_pump = start_filter = millis();
  }
  else if (method == "setFilterCycle") {
    filterCycleMode = val;
    timer_filter_active = false;
    if (val) {
      filterCyclePhaseOn = true;
      filterCycleLastChange = millis();
      filterMode = drainState = pumpState = true;
      start_drain = start_pump = start_filter = millis();
    } else {
      filterMode = drainState = pumpState = false;
    }
    saveSettings();
  }
  else if (method == "setSystem") {
    systemEnabled = val;
    if (!systemEnabled) {
      heaterState = fanState = pumpState = oxyState = drainState = ledState = filterMode = filterCycleMode = false;
      timer_heater_active = timer_fan_active = timer_drain_active = timer_led_active = timer_filter_active = false;
      oxyModeContinuous = false;
    }
    saveSettings();
  }
  else if (method == "setFeed" && val) {
    feedTriggered = true;
  }

  sendToSlave(feedTriggered);

  // Tra loi 2-way RPC response ve ThingsBoard de widget tren Dashboard khong bi timeout
  String topicStr = String(topic);
  int reqIdx = topicStr.lastIndexOf('/');
  if (reqIdx != -1) {
    String reqId = topicStr.substring(reqIdx + 1);
    String respTopic = "v1/devices/me/rpc/response/" + reqId;
    mqtt.publish(respTopic.c_str(), "{\"status\":\"SUCCESS\"}");
  }
}
