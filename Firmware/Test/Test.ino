/*
 * ============================================================
 *  ESP32 TEST WEBSERVER (GPIO 2 LED Control) - STA ONLY
 * ============================================================
 *  WiFi STA : NONNET / 12345678
 *  Port Web : 80
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 2

const char* STA_SSID = "NONNET";
const char* STA_PASS = "12345678";

WebServer server(80);
bool ledState = false;

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 WebServer Test (Station Mode)</title>
  <style>
    * { margin:0; padding:0; box-sizing:border-box; font-family:'Segoe UI',sans-serif; }
    body { background:#0f172a; color:#e2e8f0; display:flex; justify-content:center; align-items:center; min-height:100vh; padding:20px; }
    .card { background:#1e293b; border-radius:16px; padding:24px; max-width:400px; width:100%; text-align:center; box-shadow:0 10px 25px rgba(0,0,0,0.5); }
    h1 { color:#38bdf8; font-size:1.4rem; margin-bottom:8px; }
    p { color:#94a3b8; font-size:0.85rem; margin-bottom:20px; }
    .status-box { background:#334155; padding:15px; border-radius:10px; margin-bottom:20px; }
    .status-text { font-size:1.2rem; font-weight:bold; }
    .btn { display:block; width:100%; padding:15px; border:none; border-radius:10px; font-size:1.1rem; font-weight:bold; cursor:pointer; transition:0.2s; color:#fff; }
    .btn-on { background:#22c55e; }
    .btn-off { background:#ef4444; }
    .btn:active { transform:scale(0.98); }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 TEST STA MODE</h1>
    <p>Dieu khien LED Build-in (GPIO 2)</p>
    
    <div class="status-box">
      <div>Trang thai LED:</div>
      <div id="st" class="status-text">DANG DANG TAI...</div>
    </div>

    <button id="btn" class="btn btn-off" onclick="toggleLED()">DANG TAI...</button>
  </div>

  <script>
    function updateUI(s) {
      const st = document.getElementById('st');
      const btn = document.getElementById('btn');
      if(s) {
        st.textContent = "DANG BAT (ON)";
        st.style.color = "#22c55e";
        btn.textContent = "TAT LED";
        btn.className = "btn btn-off";
      } else {
        st.textContent = "DANG TAT (OFF)";
        st.style.color = "#ef4444";
        btn.textContent = "BAT LED";
        btn.className = "btn btn-on";
      }
    }

    function getStatus() {
      fetch('/api/status').then(r=>r.json()).then(d=>{
        updateUI(d.led);
      });
    }

    function toggleLED() {
      fetch('/api/toggle', {method:'POST'}).then(r=>r.json()).then(d=>{
        updateUI(d.led);
      });
    }

    getStatus();
    setInterval(getStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatus() {
  String json = "{\"led\":" + String(ledState ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.printf("[TEST] LED GPIO 2 -> %s\n", ledState ? "BAT (HIGH)" : "TAT (LOW)");
  handleStatus();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n--- ESP32 WEBSERVER TEST (STA ONLY) ---");

  // Chế độ Station thuần túy
  WiFi.mode(WIFI_STA);
  Serial.print("[WIFI] Dang ket noi toi WiFi: ");
  Serial.println(STA_SSID);

  WiFi.begin(STA_SSID, STA_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("[WIFI] Da ket noi thanh cong!");
  Serial.print("[WEB] Dia chi Web: http://");
  Serial.println(WiFi.localIP());

  // Web Routes
  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/toggle", HTTP_POST, handleToggle);
  
  server.onNotFound([]() {
    server.send(200, "text/html", HTML_PAGE);
  });

  server.begin();
  Serial.println("[WEB] Web Server da khoi chay tren cong 80!");
}

void loop() {
  server.handleClient();
}
