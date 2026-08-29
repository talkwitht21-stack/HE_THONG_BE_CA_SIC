/*
 * ============================================================
 *  ESP32-SLAVE FIRMWARE (v2)
 *  Dự án: Hệ thống Giám sát & Điều khiển Tự động Bể Cá
 * ============================================================
 *  Vai trò: Nút thực thi vật lý
 *    + Đọc cảm biến: DS18B20, DHT22, Phao từ
 *    + Điều khiển 6 Relay: Sưởi, Quạt, Bơm bù, Oxy, Bơm thay, Đèn
 *    + Điều khiển Servo cho ăn
 *    + Lắng nghe Remote IR (NEC 20 phím) -> toggle relay
 *    + Chạy logic Sục Oxy (chu kỳ 5p ON / 15p OFF hoặc Liên tục)
 *    + Gửi trạng thái lên Master qua UART
 * ============================================================
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <IRremote.hpp>

// ===================== CẤU HÌNH CHÂN GPIO =====================

// Cảm biến
#define DS18B20_PIN       4     // Nhiệt độ nước
#define DHT_PIN           5     // Nhiệt ẩm KK
#define DHT_TYPE          DHT22
#define HC_TRIG_PIN       14    // Siêu âm Trig
#define HC_ECHO_PIN       32    // Siêu âm Echo
#define IR_RECEIVE_PIN    15    // Mắt thu IR MH-R38

// Relay (OUTPUT)
#define HEATER_RELAY_PIN  25
#define FAN_RELAY_PIN     26
#define PUMP_RELAY_PIN    27    // Bơm bù nước
#define OXY_RELAY_PIN     12    // Sục oxy
#define DRAIN_RELAY_PIN   23    // Bơm thay nước
#define LED_RELAY_PIN     19    // Đèn LED

// Servo cho ăn
#define SERVO_FEED_PIN    13

// ===================== KHỞI TẠO ĐỐI TƯỢNG =====================

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);
Servo servoFeed;

// ===================== BIẾN TRẠNG THÁI =====================
#include <Preferences.h>
Preferences prefs;

uint8_t ir_btn_1 = 0x45;
uint8_t ir_btn_2 = 0x46;
uint8_t ir_btn_3 = 0x47;
uint8_t ir_btn_4 = 0x44;
uint8_t ir_btn_5 = 0x40;
uint8_t ir_btn_6 = 0x43;
uint8_t ir_btn_7 = 0x07;
uint8_t ir_btn_8 = 0x15;
uint8_t ir_btn_0 = 0x16;

bool heaterState = false;
bool fanState    = false;
bool pumpState   = false;
bool oxyState    = true;  // Mặc định BẬT
bool drainState  = false;
bool ledState    = false;

float waterLevelCm = 0.0; // Khoảng cách siêu âm


// Trạng thái Servo (Máy trạng thái 2 pha Zero-Delay)
enum FeedPhase {
  FEED_IDLE = 0,
  FEED_OPENING,
  FEED_CLOSING
};

FeedPhase feedPhase     = FEED_IDLE;
unsigned long feedPhaseStart = 0;
bool feedingActive     = false;
int feed_angle         = 180; // Góc quay cho ăn (Không giới hạn góc)

// Logic Sục Oxy
bool oxyModeContinuous = false; // false = chu kỳ, true = liên tục
bool oxyCycleEnabled   = true;  // true khi chu kỳ đang kích hoạt, false khi tắt hẳn
unsigned long oxyLastChange = 0;
uint16_t oxy_on_min  = 5;       // 5 phút
uint16_t oxy_off_min = 15;      // 15 phút

// Logic phím 4 (Triple press)
unsigned long lastPress4Time = 0;
int press4Count = 0;

// Heartbeat voi Master (PING/PONG moi 15s)
const unsigned long PING_INTERVAL_MS = 15000UL;
unsigned long lastPingSent   = 0;
bool          pongReceived   = true;  // true lúc đầu để không ngắt ngay khi boot
bool          masterAlive    = true;

// Giao tiếp UART
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 2000;
bool forceStatusUpdate = false;
uint8_t lastIrCode = 0;
bool hasNewIrCode = false;

// ===================== HÀM SETUP =====================

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  ds18b20.begin();
  ds18b20.setWaitForConversion(false); // Non-blocking: 0ms delay thay vi block 750ms
  ds18b20.requestTemperatures();       // Khoi tao lan doc dau tien
  dht.begin();
  
  pinMode(HC_TRIG_PIN, OUTPUT);
  pinMode(HC_ECHO_PIN, INPUT);

  // Relay
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(OXY_RELAY_PIN, OUTPUT);
  pinMode(DRAIN_RELAY_PIN, OUTPUT);
  pinMode(LED_RELAY_PIN, OUTPUT);
  
  applyRelayStates(); // Khởi tạo mặc định

  // Servo - attach khi dùng và detach khi xong để chống nóng & rung motor
  servoFeed.attach(SERVO_FEED_PIN);
  servoFeed.write(0);
  delay(150);
  servoFeed.detach();

  // Load ma phim IR & Goc quay tu Flash
  prefs.begin("beca", false);
  feed_angle = prefs.getInt("fa", 180);
  ir_btn_1 = prefs.getUChar("ir1", 0x45);
  ir_btn_2 = prefs.getUChar("ir2", 0x46);
  ir_btn_3 = prefs.getUChar("ir3", 0x47);
  ir_btn_4 = prefs.getUChar("ir4", 0x44);
  ir_btn_5 = prefs.getUChar("ir5", 0x40);
  ir_btn_6 = prefs.getUChar("ir6", 0x43);
  ir_btn_7 = prefs.getUChar("ir7", 0x07);
  ir_btn_8 = prefs.getUChar("ir8", 0x15);
  ir_btn_0 = prefs.getUChar("ir0", 0x16);
  prefs.end();

  // IR - TAT LED_FEEDBACK de tranh xung dot GPIO 13 (Servo)
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  Serial.println("[SLAVE] Khoi dong v2 OK - IR san sang!");
  Serial.printf("[SLAVE] Ma phim: 1=0x%02X 2=0x%02X 3=0x%02X 4=0x%02X 5=0x%02X 6=0x%02X 7=0x%02X 8=0x%02X 0=0x%02X\n",
                ir_btn_1, ir_btn_2, ir_btn_3, ir_btn_4, ir_btn_5, ir_btn_6, ir_btn_7, ir_btn_8, ir_btn_0);
}

// ===================== HÀM LOOP =====================

void loop() {
  handleIR();
  handleFeeding();
  handleOxyCycle();
  handleMasterCommand();
  checkHeartbeat();

  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS || forceStatusUpdate) {
    lastSendTime = now;
    forceStatusUpdate = false;
    sendStatusToMaster();
  }
}

// ===================== XỬ LÝ IR =====================

void handleIR() {
  if (IrReceiver.decode()) {
    uint8_t command = IrReceiver.decodedIRData.command;

    // Resume NGAY LAP TUC - phai goi truoc khi xu ly de khong bi tro
    IrReceiver.resume();

    // Bo qua phim de (repeat) - nhung da resume roi nen IRremote van tiep tuc nhan
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) return;

    lastIrCode = command;
    hasNewIrCode = true;
    forceStatusUpdate = true;
    Serial.printf("[SLAVE] IR Code: 0x%02X (Proto: %d)\n",
                  command, IrReceiver.decodedIRData.protocol);

    unsigned long now = millis();

    if (command == ir_btn_1) {
      heaterState = !heaterState;
      Serial.printf("[SLAVE] Suoi -> %s\n", heaterState ? "BAT" : "TAT");
    } else if (command == ir_btn_2) {
      fanState = !fanState;
      Serial.printf("[SLAVE] Quat -> %s\n", fanState ? "BAT" : "TAT");
    } else if (command == ir_btn_3) {
      pumpState = !pumpState;
      Serial.printf("[SLAVE] BomBu -> %s\n", pumpState ? "BAT" : "TAT");
    } else if (command == ir_btn_4) {
      if (now - lastPress4Time <= 3000) press4Count++;
      else press4Count = 1;
      lastPress4Time = now;
      if (press4Count >= 3) {
        oxyModeContinuous = true;
        oxyCycleEnabled = false;
        oxyState = true;
        Serial.println("[SLAVE] Oxy -> LIEN TUC");
        press4Count = 0;
      } else {
        oxyModeContinuous = false;
        oxyState = !oxyState;
        oxyCycleEnabled = oxyState; // Chi chay chu ky khi oxyState = true
        oxyLastChange = now;
        Serial.printf("[SLAVE] Oxy -> %s (Chu ky)\n", oxyState ? "BAT" : "TAT");
      }
    } else if (command == ir_btn_5) {
      drainState = !drainState;
      Serial.printf("[SLAVE] BomThay -> %s\n", drainState ? "BAT" : "TAT");
    } else if (command == ir_btn_6) {
      ledState = !ledState;
      Serial.printf("[SLAVE] LED -> %s\n", ledState ? "BAT" : "TAT");
    } else if (command == ir_btn_7) {
      startFeeding();
      Serial.println("[SLAVE] Cho an!");
    } else if (command == ir_btn_8) {
      bool newFilter = !(pumpState && drainState);
      pumpState = drainState = newFilter;
      Serial.printf("[SLAVE] Loc Nuoc (Song song) -> %s\n", newFilter ? "BAT" : "TAT");
    } else if (command == ir_btn_0) {
      heaterState = fanState = pumpState = oxyState = drainState = ledState = false;
      oxyModeContinuous = false;
      oxyCycleEnabled = false;
      Serial.println("[SLAVE] EMERGENCY OFF - Tat tat ca!");
    } else {
      Serial.printf("[SLAVE] Phim chua gan chuc nang: 0x%02X\n", command);
    }
    applyRelayStates();
  }
}

// ===================== LOGIC THIẾT BỊ =====================

void startFeeding() {
  if (feedPhase == FEED_IDLE) {
    Serial.printf("[SLAVE] Servo: Cho an START (Goc: %d do, Pha: MO)\n", feed_angle);
    feedingActive = true;
    feedPhase = FEED_OPENING;
    feedPhaseStart = millis();
    servoFeed.attach(SERVO_FEED_PIN, 500, 2500);
    servoFeed.write(feed_angle);
  }
}

void handleFeeding() {
  if (feedPhase == FEED_OPENING) {
    // Giu mo cua xa moi trong 1500ms
    if (millis() - feedPhaseStart >= 1500UL) {
      Serial.println("[SLAVE] Servo: Quay ve 0 do (Pha: DONG)");
      servoFeed.write(0);
      feedPhase = FEED_CLOSING;
      feedPhaseStart = millis();
    }
  } else if (feedPhase == FEED_CLOSING) {
    // Cho 1000ms de banh rang co khi dong kin 100% roi moi ngat PWM (Khong dung delay)
    if (millis() - feedPhaseStart >= 1000UL) {
      Serial.println("[SLAVE] Servo: Da dong kin 100% -> Detach PWM an toan");
      servoFeed.detach();
      feedPhase = FEED_IDLE;
      feedingActive = false;
    }
  }
}

void handleOxyCycle() {
  if (oxyModeContinuous) {
    if (!oxyState) {
      oxyState = true;
      applyRelayStates();
      forceStatusUpdate = true;
    }
    return; // Chế độ liên tục 24/7
  }

  // Chế độ Chu Kỳ Tự Động (Tự động luân phiên Bật / Nghỉ)
  unsigned long now = millis();
  unsigned long onMs  = (unsigned long)max(1, (int)oxy_on_min) * 60000UL;
  unsigned long offMs = (unsigned long)max(1, (int)oxy_off_min) * 60000UL;

  if (oxyState) {
    if (now - oxyLastChange >= onMs) {
      oxyState = false;
      oxyLastChange = now;
      applyRelayStates();
      forceStatusUpdate = true;
      Serial.printf("[SLAVE OXY] Het %u phut BAT -> Chuyen sang NGHI (%u phut)\n", oxy_on_min, oxy_off_min);
    }
  } else {
    if (now - oxyLastChange >= offMs) {
      oxyState = true;
      oxyLastChange = now;
      applyRelayStates();
      forceStatusUpdate = true;
      Serial.printf("[SLAVE OXY] Het %u phut NGHI -> Chuyen sang BAT (%u phut)\n", oxy_off_min, oxy_on_min);
    }
  }
}

void emergencyOff() {
  heaterState = fanState = pumpState = oxyState = drainState = ledState = false;
  oxyModeContinuous = false; // Ve che do chu ky an toan
  oxyCycleEnabled   = false;
  applyRelayStates();
  Serial.println("[SLAVE] !!! MAT PONG TU MASTER -> NGAT DIEN AN TOAN NGAY LAP TUC !!!");
}

void checkHeartbeat() {
  unsigned long now = millis();
  if (now - lastPingSent < PING_INTERVAL_MS) return;

  // Neu lan ping truoc (sau 15s) chua nhan duoc PONG tu Master -> Ngat dien ngay lap tuc!
  if (!pongReceived && masterAlive) {
    masterAlive = false;
    emergencyOff();
  }

  // Gui PING moi len Master
  Serial2.println("{\"cmd\":\"ping\"}");
  lastPingSent = now;
  pongReceived = false;
  Serial.println("[SLAVE -> MASTER] PING?");
}

void applyRelayStates() {
  digitalWrite(HEATER_RELAY_PIN, heaterState ? HIGH : LOW);
  digitalWrite(FAN_RELAY_PIN,    fanState    ? HIGH : LOW);
  digitalWrite(PUMP_RELAY_PIN,   pumpState   ? HIGH : LOW);
  digitalWrite(OXY_RELAY_PIN,    oxyState    ? HIGH : LOW);
  digitalWrite(DRAIN_RELAY_PIN,  drainState  ? HIGH : LOW);
  digitalWrite(LED_RELAY_PIN,    ledState    ? HIGH : LOW);
  
  Serial.printf("[SLAVE] Relay: Suoi=%d Quat=%d BomBu=%d Oxy=%d BomThay=%d Led=%d\n", 
                heaterState, fanState, pumpState, oxyState, drainState, ledState);
}

// ===================== GIAO TIẾP MASTER =====================

void handleMasterCommand() {
  if (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    incoming.trim();
    if (incoming.length() == 0) return;

    Serial.println("[SLAVE] Received from Master: " + incoming);

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, incoming);
    if (err) return;

    // Nhan phan hoi PONG tu Master
    if (doc.containsKey("cmd") && doc["cmd"].as<String>() == "pong") {
      pongReceived = true;
      masterAlive  = true;
      Serial.println("[SLAVE] Da nhan PONG <- Master OK");
      return;
    }

    if (doc.containsKey("cmd") && doc["cmd"].as<String>() == "ir_map") {
      prefs.begin("beca", false);
      if(doc.containsKey("ir1")) { const char* s = doc["ir1"]; if (s) { ir_btn_1 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir1", ir_btn_1); } }
      if(doc.containsKey("ir2")) { const char* s = doc["ir2"]; if (s) { ir_btn_2 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir2", ir_btn_2); } }
      if(doc.containsKey("ir3")) { const char* s = doc["ir3"]; if (s) { ir_btn_3 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir3", ir_btn_3); } }
      if(doc.containsKey("ir4")) { const char* s = doc["ir4"]; if (s) { ir_btn_4 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir4", ir_btn_4); } }
      if(doc.containsKey("ir5")) { const char* s = doc["ir5"]; if (s) { ir_btn_5 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir5", ir_btn_5); } }
      if(doc.containsKey("ir6")) { const char* s = doc["ir6"]; if (s) { ir_btn_6 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir6", ir_btn_6); } }
      if(doc.containsKey("ir7")) { const char* s = doc["ir7"]; if (s) { ir_btn_7 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir7", ir_btn_7); } }
      if(doc.containsKey("ir8")) { const char* s = doc["ir8"]; if (s) { ir_btn_8 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir8", ir_btn_8); } }
      if(doc.containsKey("ir0")) { const char* s = doc["ir0"]; if (s) { ir_btn_0 = (uint8_t)strtoul(s, NULL, 16); prefs.putUChar("ir0", ir_btn_0); } }
      prefs.end();
      Serial.printf("[SLAVE] Da cap nhat ma IR: 1=0x%02X 2=0x%02X 3=0x%02X 4=0x%02X 5=0x%02X 6=0x%02X 7=0x%02X 8=0x%02X 0=0x%02X\n",
                    ir_btn_1, ir_btn_2, ir_btn_3, ir_btn_4, ir_btn_5, ir_btn_6, ir_btn_7, ir_btn_8, ir_btn_0);
      return;
    }

    if (doc.containsKey("cmd") && String((const char*)doc["cmd"]) == "relay") {
      if (doc.containsKey("heater")) heaterState = doc["heater"].as<bool>();
      if (doc.containsKey("fan"))    fanState    = doc["fan"].as<bool>();
      if (doc.containsKey("pump"))   pumpState   = doc["pump"].as<bool>();
      if (doc.containsKey("drain"))  drainState  = doc["drain"].as<bool>();
      if (doc.containsKey("led"))    ledState    = doc["led"].as<bool>();
      
      if (doc.containsKey("oxy_mode")) {
        bool new_om = doc["oxy_mode"].as<bool>();
        if (new_om != oxyModeContinuous) {
          oxyModeContinuous = new_om;
          oxyLastChange = millis();
        }
      }
      if (doc.containsKey("oo")) {
        uint16_t new_oo = doc["oo"];
        if (new_oo != oxy_on_min && new_oo > 0) {
          oxy_on_min = new_oo;
          oxyLastChange = millis();
        }
      }
      if (doc.containsKey("of")) {
        uint16_t new_of = doc["of"];
        if (new_of != oxy_off_min && new_of > 0) {
          oxy_off_min = new_of;
          oxyLastChange = millis();
        }
      }
      if (doc.containsKey("oxy") && oxyModeContinuous) {
        oxyState = doc["oxy"].as<bool>();
      }

      if (doc.containsKey("fa")) {
        feed_angle = doc["fa"].as<int>();
        prefs.begin("beca", false);
        prefs.putInt("fa", feed_angle);
        prefs.end();
      }

      applyRelayStates();
      
      if (doc.containsKey("feed") && doc["feed"].as<bool>()) {
        startFeeding();
      }
      forceStatusUpdate = true;
    }
  }
}

// Ham doc sieu am loc trung vi 5 mau (Median Filter) de chong nhay so ao do gon song nuoc
float readUltrasonicMedian() {
  float samples[5];
  int validCount = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(HC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(HC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(HC_TRIG_PIN, LOW);
    long duration = pulseIn(HC_ECHO_PIN, HIGH, 25000); // Timeout 25ms
    if (duration > 0) {
      float d = (duration / 2.0) * 0.0343;
      if (d >= 2.0 && d <= 400.0) {
        samples[validCount++] = d;
      }
    }
    delay(3); // Nghi ngan giua cac lan ban xung
  }
  if (validCount == 0) return -1.0;

  // Sap xep Bubble sort de lay trung vi
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = i + 1; j < validCount; j++) {
      if (samples[i] > samples[j]) {
        float temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  return samples[validCount / 2];
}

void sendStatusToMaster() {
  float wt  = ds18b20.getTempCByIndex(0); // Doc gia tri da chuyen doi o lan truoc (0ms non-blocking)
  ds18b20.requestTemperatures();          // Yeu cau chuyen doi ngam cho lan sau
  float at  = dht.readTemperature();
  float ah  = dht.readHumidity();

  // Doc sieu am qua bo loc trung vi (Median Filter)
  waterLevelCm = readUltrasonicMedian();

  if (isnan(at)) at = -999.0;
  if (isnan(ah)) ah = -999.0;
  if (wt < -100) wt = -999.0; // DS18B20 mat ket noi tra ve -127

  StaticJsonDocument<384> doc;
  doc["water_temp"] = wt;
  doc["air_temp"]   = at;
  doc["air_hum"]    = ah;
  doc["water_cm"]   = waterLevelCm;
  doc["heater"]     = heaterState ? 1 : 0;
  doc["fan"]        = fanState    ? 1 : 0;
  doc["pump"]       = pumpState   ? 1 : 0;
  doc["oxy"]        = oxyState    ? 1 : 0;
  doc["drain"]      = drainState  ? 1 : 0;
  doc["led"]        = ledState    ? 1 : 0;
  doc["oxy_mode"]   = oxyModeContinuous ? 1 : 0;

  if (hasNewIrCode) {
    char hexBuf[5];
    sprintf(hexBuf, "%02X", lastIrCode);
    doc["last_ir"] = String(hexBuf);
    hasNewIrCode = false; // Reset sau khi da gui len Master
  } else {
    doc["last_ir"] = "";
  }

  String jsonString;
  serializeJson(doc, jsonString);
  Serial2.println(jsonString);

  Serial.printf("[SLAVE->MASTER] Nuoc=%.1fC KK=%.1fC Am=%.1f%% MucNuoc=%.1fcm IR=%02X Relay:H%d F%d P%d O%d D%d L%d\n",
                wt, at, ah, waterLevelCm, lastIrCode,
                heaterState, fanState, pumpState, oxyState, drainState, ledState);
}
