import json
import time
import threading
import uuid
import paho.mqtt.client as mqtt

class MQTTAIClient:
    """
    Kết nối MQTT gửi Telemetry AI lên Device trên ThingsBoard Cloud.
    Tương thích 100% cả Paho-MQTT v1 và v2, tự động kết nối lại và đẩy dữ liệu tức thì.
    """
    def __init__(self, host="thingsboard.cloud", port=1883, access_token="", shared_state=None):
        self.host = host
        self.port = port
        self.access_token = str(access_token).strip()
        self.shared_state = shared_state
        self.enabled = bool(self.access_token and self.access_token != "YOUR_RPI5_AI_DEVICE_TOKEN" and len(self.access_token) > 2)
        
        self.client = None
        self.connected = False
        
        if self.enabled:
            self._init_mqtt()

    def update_token(self, new_token):
        """
        Cập nhật Access Token mới trực tiếp từ Web Dashboard và kết nối lại ThingsBoard.
        """
        self.access_token = str(new_token).strip()
        self.enabled = bool(self.access_token and self.access_token != "YOUR_RPI5_AI_DEVICE_TOKEN" and len(self.access_token) > 2)
        
        if self.client:
            try:
                self.client.loop_stop()
                self.client.disconnect()
            except Exception:
                pass
            self.client = None
            self.connected = False

        if self.enabled:
            self._init_mqtt()
            self.start()
            print(f"[MQTT AI] Da cap nhat Access Token moi ({self.access_token[:6]}...) va dang ket noi ThingsBoard...")
            return True
        return False

    def _init_mqtt(self):
        try:
            # Tạo client ID ngẫu nhiên chống trùng lặp phiên kết nối
            client_id = f"AI_Node_{uuid.uuid4().hex[:8]}"
            
            # Tương thích cả Paho MQTT v2 và v1
            if hasattr(mqtt, "CallbackAPIVersion"):
                self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=client_id)
            else:
                self.client = mqtt.Client(client_id=client_id)

            self.client.username_pw_set(self.access_token)
            self.client.on_connect = self._on_connect
            self.client.on_disconnect = self._on_disconnect
            print(f"[MQTT AI] Khoi tao client MQTT voi Token: {self.access_token[:6]}***")
        except Exception as e:
            print(f"[MQTT AI ERROR] Loi khoi tao: {e}")

    def _on_connect(self, *args, **kwargs):
        # args trong paho v1: (client, userdata, flags, rc)
        # args trong paho v2: (client, userdata, flags, reason_code, properties)
        rc = 0
        if len(args) >= 4:
            rc = args[3]
        if hasattr(rc, "value"):
            rc = rc.value
        elif hasattr(rc, "is_failure") and rc.is_failure:
            rc = 1

        if rc == 0:
            self.connected = True
            print("=" * 65)
            print(" [MQTT AI] KET NOI THANH CONG TOI THINGSBOARD CLOUD!")
            print("=" * 65)
            
            # 1. Gui attributes ban dau
            try:
                attr = {
                    "node_type": "Edge AI Python Gateway",
                    "ai_engine": "Gemini 3.5 Flash VLM",
                    "status": "Online"
                }
                self.client.publish("v1/devices/me/attributes", json.dumps(attr))
            except Exception:
                pass

            # 2. ĐẨY NGAY LẬP TỨC TELEMETRY BAN ĐẦU LÊN THINGSBOARD (Không phải chờ 120s)
            total = 10
            if self.shared_state:
                total = self.shared_state.get("total_fish_configured", 10)
                
            init_telemetry = {
                "total_fish": total,
                "alive_fish": total,
                "dead_fish": 0,
                "water_turbidity": 10,
                "summary": "AI Gateway da ket noi va san sang giam sat.",
                "ai_engine": "Gemini 3.5 Flash",
                "ai_fps": 15.0,
                "is_alert": False
            }
            self.publish_ai_telemetry(init_telemetry, fps=15.0)
        else:
            self.connected = False
            print(f"[MQTT AI WARN] Ket noi ThingsBoard that bai (Ma loi rc={rc}). Kiem tra lai Access Token!")

    def _on_disconnect(self, *args, **kwargs):
        self.connected = False
        print("[MQTT AI] Mat ket noi voi ThingsBoard, se tu dong thu lai...")

    def start(self):
        if not self.enabled or not self.client:
            print("[MQTT AI WARN] ThingsBoard AI chua duoc bat hoac chua co Access Token hop le.")
            return
        try:
            print(f"[MQTT AI] Dang ket noi toi {self.host}:{self.port}...")
            self.client.connect(self.host, self.port, 60)
            self.client.loop_start()
        except Exception as e:
            print(f"[MQTT AI ERROR] Khong the ket noi toi {self.host}:{self.port} - {e}")

    def publish_ai_telemetry(self, ai_data, fps=0.0):
        if not self.enabled or not self.client:
            return False
            
        total = int(ai_data.get("total_fish", 10))
        dead = int(ai_data.get("dead_fish", 0))
        alive = int(ai_data.get("alive_fish", max(0, total - dead)))
        turb = int(ai_data.get("water_turbidity", 0))
        summary = str(ai_data.get("summary", ""))
        engine = str(ai_data.get("ai_engine", "Gemini 3.5 Flash VLM"))
        is_alert = bool(ai_data.get("is_alert", dead > 0 or turb >= 40))

        telemetry = {
            # Key chuẩn khớp 100% với Widget ThingsBoard
            "total_fish": total,
            "alive_fish": alive,
            "dead_fish": dead,
            "water_turbidity": turb,
            "summary": summary,
            "ai_engine": engine,
            "ai_fps": round(float(fps), 1),
            "is_alert": is_alert,
            
            # Key dự phòng có prefix
            "ai_total_fish": total,
            "ai_alive_fish": alive,
            "ai_dead_fish": dead,
            "ai_water_turbidity": turb,
            "ai_summary": summary,
            "ai_alert": is_alert
        }
        
        try:
            payload = json.dumps(telemetry)
            self.client.publish("v1/devices/me/telemetry", payload)
            print(f"[MQTT AI TELEMETRY -> THINGSBOARD] {payload}")
            return True
        except Exception as e:
            print(f"[MQTT AI ERROR] Loi publish telemetry: {e}")
            return False

    def stop(self):
        if self.client:
            try:
                self.client.loop_stop()
                self.client.disconnect()
            except Exception:
                pass
