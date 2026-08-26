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

// Mã IR (Ví dụ cho NEC 21/20 phím, có thể thay đổi tùy remote thực tế)
#define IR_BTN_1     0x45
#define IR_BTN_2     0x46
#define IR_BTN_3     0x47
#define IR_BTN_4     0x44
#define IR_BTN_5     0x40
#define IR_BTN_6     0x43
#define IR_BTN_7     0x07
#define IR_BTN_0     0x19 // Hoặc 0x16

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

  // IR
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("[SLAVE] Khởi động v2 (IR Remote)...");
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
    // Luôn in ra mã nhận được để dễ debug phím
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
      // Bỏ qua phím đè
    } else {
      uint8_t command = IrReceiver.decodedIRData.command;
      Serial.printf("[SLAVE] IR Code: 0x%02X (Proto: %d, Raw: 0x%X)\n", 
                    command, IrReceiver.decodedIRData.protocol, IrReceiver.decodedIRData.decodedRawData);
      
      unsigned long now = millis();
      
      switch (command) {
        case IR_BTN_1:
          heaterState = !heaterState;
          forceStatusUpdate = true;
          break;
          case IR_BTN_2:
            fanState = !fanState;
            forceStatusUpdate = true;
            break;
          case IR_BTN_3:
            pumpState = !pumpState;
            forceStatusUpdate = true;
            break;
          case IR_BTN_4:
            // Sục oxy (1 lần -> chu kỳ, 3 lần/3s -> liên tục)
            if (now - lastPress4Time <= 3000) {
              press4Count++;
            } else {
              press4Count = 1;
            }
            lastPress4Time = now;
            
            if (press4Count >= 3) {
              oxyModeContinuous = true;
              oxyState = true; // Bật luôn
              Serial.println("[SLAVE] Oxy -> LIÊN TỤC");
              press4Count = 0;
            } else {
              // Bật/tắt thủ công hoặc reset chu kỳ
              oxyModeContinuous = false;
              oxyState = !oxyState;
              oxyLastChange = millis();
              Serial.println(oxyState ? "[SLAVE] Oxy -> BẬT (Chu kỳ)" : "[SLAVE] Oxy -> TẮT");
            }
            forceStatusUpdate = true;
            break;
          case IR_BTN_5:
            drainState = !drainState;
            forceStatusUpdate = true;
            break;
          case IR_BTN_6:
            ledState = !ledState;
            forceStatusUpdate = true;
            break;
          case IR_BTN_7:
            startFeeding();
            break;
          case IR_BTN_0:
            heaterState = fanState = pumpState = oxyState = drainState = ledState = false;
            oxyModeContinuous = false;
            forceStatusUpdate = true;
            Serial.println("[SLAVE] EMERGENCY OFF");
            break;
        }
        applyRelayStates();
    }
    IrReceiver.resume();
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

    StaticJsonDocument<300> doc;
    DeserializationError err = deserializeJson(doc, incoming);
    if (err) return;

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
  float waterTemp = ds18b20.getTempCByIndex(0);
  float airTemp   = dht.readTemperature();
  float airHum    = dht.readHumidity();

  // Đọc siêu âm
  digitalWrite(HC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(HC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(HC_TRIG_PIN, LOW);
  long duration = pulseIn(HC_ECHO_PIN, HIGH, 30000); // Timeout 30ms
  if (duration > 0) {
    waterLevelCm = (duration / 2.0) * 0.0343;
  } else {
    waterLevelCm = -1; // Lỗi
  }

  if (isnan(airTemp)) airTemp = -999.0;
  if (isnan(airHum))  airHum  = -999.0;

  StaticJsonDocument<300> doc;
  doc["water_temp"] = waterTemp;
  doc["air_temp"]   = airTemp;
  doc["air_hum"]    = airHum;
  doc["water_cm"]   = waterLevelCm;
  doc["heater"]     = heaterState ? 1 : 0;
  doc["fan"]        = fanState    ? 1 : 0;
  doc["pump"]       = pumpState   ? 1 : 0;
  doc["oxy"]        = oxyState    ? 1 : 0;
  doc["drain"]      = drainState  ? 1 : 0;
  doc["led"]        = ledState    ? 1 : 0;
  doc["oxy_mode"]   = oxyModeContinuous ? 1 : 0;

  String jsonString;
  serializeJson(doc, jsonString);
  Serial2.println(jsonString);
  
  Serial.printf("[SLAVE] Send: NhietNuoc=%.1f NhietKK=%.1f AmKK=%.1f MucNuoc=%.1fcm\n",
                waterTemp, airTemp, airHum, waterLevelCm);
}
