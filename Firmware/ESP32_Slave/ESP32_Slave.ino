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
uint8_t ir_btn_0 = 0x16;

bool heaterState = false;
bool fanState    = false;
bool pumpState   = false;
bool oxyState    = true;  // Mặc định BẬT
bool drainState  = false;
bool ledState    = false;

float waterLevelCm = 0.0; // Khoảng cách siêu âm


// Trạng thái Servo
bool feedingActive     = false;
unsigned long feedStart = 0;
const unsigned long FEED_DURATION_MS = 2000; // 2 giây

// Logic Sục Oxy
bool oxyModeContinuous = false; // false = chu kỳ, true = liên tục
unsigned long oxyLastChange = 0;
const unsigned long OXY_ON_TIME  = 5 * 60 * 1000UL;  // 5 phút
const unsigned long OXY_OFF_TIME = 15 * 60 * 1000UL; // 15 phút

// Logic phím 4 (Triple press)
unsigned long lastPress4Time = 0;
int press4Count = 0;

// Giao tiếp UART
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 2000;
bool forceStatusUpdate = false;
uint8_t lastIrCode = 0;

// ===================== HÀM SETUP =====================

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  ds18b20.begin();
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

  // Servo
  servoFeed.attach(SERVO_FEED_PIN);
  servoFeed.write(0);

  // Load ma phim IR tu Flash (neu da tu gan truoc do)
  prefs.begin("beca", false);
  ir_btn_1 = prefs.getUChar("ir1", 0x45);
  ir_btn_2 = prefs.getUChar("ir2", 0x46);
  ir_btn_3 = prefs.getUChar("ir3", 0x47);
  ir_btn_4 = prefs.getUChar("ir4", 0x44);
  ir_btn_5 = prefs.getUChar("ir5", 0x40);
  ir_btn_6 = prefs.getUChar("ir6", 0x43);
  ir_btn_7 = prefs.getUChar("ir7", 0x07);
  ir_btn_0 = prefs.getUChar("ir0", 0x16);
  prefs.end();

  // IR - TAT LED_FEEDBACK de tranh xung dot GPIO 13 (Servo)
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  Serial.println("[SLAVE] Khoi dong v2 OK - IR san sang!");
  Serial.printf("[SLAVE] Ma phim: 1=0x%02X 2=0x%02X 3=0x%02X 4=0x%02X 5=0x%02X 6=0x%02X 7=0x%02X 0=0x%02X\n",
                ir_btn_1, ir_btn_2, ir_btn_3, ir_btn_4, ir_btn_5, ir_btn_6, ir_btn_7, ir_btn_0);
}

// ===================== HÀM LOOP =====================

void loop() {
  handleIR();
  handleFeeding();
  handleOxyCycle();
  handleMasterCommand();

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
        oxyState = true;
        Serial.println("[SLAVE] Oxy -> LIEN TUC");
        press4Count = 0;
      } else {
        oxyModeContinuous = false;
        oxyState = !oxyState;
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
    } else if (command == ir_btn_0) {
      heaterState = fanState = pumpState = oxyState = drainState = ledState = false;
      oxyModeContinuous = false;
      Serial.println("[SLAVE] EMERGENCY OFF - Tat tat ca!");
    } else {
      Serial.printf("[SLAVE] Phim chua gan chuc nang: 0x%02X\n", command);
    }
    applyRelayStates();
  }
}

// ===================== LOGIC THIẾT BỊ =====================

void startFeeding() {
  if (!feedingActive) {
    Serial.println("[SLAVE] Servo: Cho an START");
    feedingActive = true;
    feedStart = millis();
    servoFeed.write(180);
  }
}

void handleFeeding() {
  if (feedingActive && (millis() - feedStart >= FEED_DURATION_MS)) {
    Serial.println("[SLAVE] Servo: Cho an STOP");
    feedingActive = false;
    servoFeed.write(0);
  }
}

void handleOxyCycle() {
  if (oxyModeContinuous) {
    if (!oxyState) {
      oxyState = true;
      applyRelayStates();
      forceStatusUpdate = true;
    }
    return; // Không chạy chu kỳ
  }
  
  unsigned long now = millis();
  if (oxyState) {
    if (now - oxyLastChange >= OXY_ON_TIME) {
      oxyState = false;
      oxyLastChange = now;
      applyRelayStates();
      forceStatusUpdate = true;
    }
  } else {
    if (now - oxyLastChange >= OXY_OFF_TIME) {
      oxyState = true;
      oxyLastChange = now;
      applyRelayStates();
      forceStatusUpdate = true;
    }
  }
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

    if (doc.containsKey("cmd") && doc["cmd"] == "ir_map") {
      prefs.begin("beca", false);
      if(doc.containsKey("ir1")) { ir_btn_1 = strtol(doc["ir1"], NULL, 16); prefs.putUChar("ir1", ir_btn_1); }
      if(doc.containsKey("ir2")) { ir_btn_2 = strtol(doc["ir2"], NULL, 16); prefs.putUChar("ir2", ir_btn_2); }
      if(doc.containsKey("ir3")) { ir_btn_3 = strtol(doc["ir3"], NULL, 16); prefs.putUChar("ir3", ir_btn_3); }
      if(doc.containsKey("ir4")) { ir_btn_4 = strtol(doc["ir4"], NULL, 16); prefs.putUChar("ir4", ir_btn_4); }
      if(doc.containsKey("ir5")) { ir_btn_5 = strtol(doc["ir5"], NULL, 16); prefs.putUChar("ir5", ir_btn_5); }
      if(doc.containsKey("ir6")) { ir_btn_6 = strtol(doc["ir6"], NULL, 16); prefs.putUChar("ir6", ir_btn_6); }
      if(doc.containsKey("ir7")) { ir_btn_7 = strtol(doc["ir7"], NULL, 16); prefs.putUChar("ir7", ir_btn_7); }
      if(doc.containsKey("ir0")) { ir_btn_0 = strtol(doc["ir0"], NULL, 16); prefs.putUChar("ir0", ir_btn_0); }
      prefs.end();
      Serial.println("[SLAVE] Da cap nhat ma IR tu Master");
      return;
    }

    if (doc.containsKey("cmd") && String((const char*)doc["cmd"]) == "relay") {
      if (doc.containsKey("heater")) heaterState = doc["heater"].as<bool>();
      if (doc.containsKey("fan"))    fanState    = doc["fan"].as<bool>();
      if (doc.containsKey("pump"))   pumpState   = doc["pump"].as<bool>();
      if (doc.containsKey("drain"))  drainState  = doc["drain"].as<bool>();
      if (doc.containsKey("led"))    ledState    = doc["led"].as<bool>();
      
      if (doc.containsKey("oxy")) {
        oxyState = doc["oxy"].as<bool>();
        oxyLastChange = millis(); // Reset timer chu kỳ nếu master gửi lệnh
      }
      if (doc.containsKey("oxy_mode")) {
        oxyModeContinuous = doc["oxy_mode"].as<bool>();
      }

      applyRelayStates();
      
      if (doc.containsKey("feed") && doc["feed"].as<bool>()) {
        startFeeding();
      }
      forceStatusUpdate = true;
    }
  }
}

void sendStatusToMaster() {
  ds18b20.requestTemperatures();
  float wt  = ds18b20.getTempCByIndex(0);
  float at  = dht.readTemperature();
  float ah  = dht.readHumidity();

  // Doc sieu am
  digitalWrite(HC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(HC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(HC_TRIG_PIN, LOW);
  long duration = pulseIn(HC_ECHO_PIN, HIGH, 30000); // Timeout 30ms
  if (duration > 0) {
    waterLevelCm = (duration / 2.0) * 0.0343;
  } else {
    waterLevelCm = -1; // Loi
  }

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

  if (lastIrCode > 0) {
    char hexBuf[5];
    sprintf(hexBuf, "%02X", lastIrCode);
    doc["last_ir"] = String(hexBuf);
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
