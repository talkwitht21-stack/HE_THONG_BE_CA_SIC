import json
import time
import threading
import paho.mqtt.client as mqtt

class MQTTAIClient:
    """
    Kết nối MQTT gửi Telemetry AI lên Device riêng trên ThingsBoard Cloud.
    Hoàn toàn độc lập với ESP32.
    """
    def __init__(self, host="demo.thingsboard.io", port=1883, access_token=""):
        self.host = host
        self.port = port
        self.access_token = access_token
        self.enabled = bool(access_token and access_token != "YOUR_RPI5_AI_DEVICE_TOKEN")
        
        self.client = None
        self.connected = False
        
        if self.enabled:
            self._init_mqtt()

    def _init_mqtt(self):
        try:
            self.client = mqtt.Client(client_id="RPi5_AI_Node")
            self.client.username_pw_set(self.access_token)
            self.client.on_connect = self._on_connect
            self.client.on_disconnect = self._on_disconnect
        except Exception as e:
            print(f"[MQTT AI ERROR] Loi khoi tao: {e}")

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print("[MQTT AI] Da ket noi thanh cong toi ThingsBoard (Device AI Node)!")
            # Gui attributes ban dau
            attr = {
                "node_type": "Edge AI Raspberry Pi 5",
                "ai_model": "Gemini Flash Vision VLM",
                "version": "v1.0"
            }
            self.client.publish("v1/devices/me/attributes", json.dumps(attr))
        else:
            self.connected = False
            print(f"[MQTT AI WARN] Ket noi that bai voi ma loi rc={rc}")

    def _on_disconnect(self, client, userdata, rc):
        self.connected = False

    def start(self):
        if not self.enabled or not self.client:
            print("[MQTT AI WARN] ThingsBoard AI Device chua duoc bat hoac chua co Access Token.")
            return
        try:
            self.client.connect(self.host, self.port, 60)
            self.client.loop_start()
        except Exception as e:
            print(f"[MQTT AI ERROR] Khong the ket noi toi {self.host}:{self.port} - {e}")

    def publish_ai_telemetry(self, ai_data, fps=0.0):
        if not self.enabled or not self.client or not self.connected:
            return False
            
        telemetry = {
            "ai_total_fish": ai_data.get("total_fish", 0),
            "ai_dead_fish": ai_data.get("dead_fish", 0),
            "ai_abnormal_fish": ai_data.get("abnormal_fish", 0),
            "ai_water_turbidity": ai_data.get("water_turbidity", 0),
            "ai_summary": ai_data.get("summary", ""),
            "ai_engine": ai_data.get("ai_engine", "AI"),
            "ai_fps": fps,
            "ai_alert": ai_data.get("is_alert", False)
        }
        
        try:
            payload = json.dumps(telemetry)
            self.client.publish("v1/devices/me/telemetry", payload)
            print(f"[MQTT AI TELEMETRY] Da gui len ThingsBoard: {payload}")
            return True
        except Exception as e:
            print(f"[MQTT AI ERROR] Loi publish telemetry: {e}")
            return False

    def stop(self):
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
