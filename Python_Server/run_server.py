import time
import threading
import os
import sys

from core.config_manager import ConfigManager
from core.video_stream import VideoStream
from core.gemini_vision import GeminiVisionAnalyzer
from core.telegram_bot import TelegramBotService
from gateway.mqtt_ai_client import MQTTAIClient
from gateway.esp32_lan_client import ESP32LANClient
from web.app import create_app

def main():
    print("================================================================")
    print("   HE THONG BE CA SIC - PYTHON SERVER (EDGE AI & IOT GATEWAY)   ")
    print("================================================================")

    # 1. Tai cau hinh
    cfg = ConfigManager("config.yaml")

    # 2. Khoi tao Camera Video Stream
    cam_cfg = cfg.get("camera")
    video_stream = VideoStream(
        source=cam_cfg.get("source", 0),
        width=cam_cfg.get("width", 640),
        height=cam_cfg.get("height", 480),
        fps=cam_cfg.get("fps", 15)
    )
    video_stream.start()

    # 3. Khoi tao Gemini Vision AI
    gem_cfg = cfg.get("gemini")
    gemini_ai = GeminiVisionAnalyzer(
        api_key=gem_cfg.get("api_key", ""),
        model_name=gem_cfg.get("model", "gemini-2.0-flash"),
        temperature=gem_cfg.get("temperature", 0.2)
    )

    # 4. Khoi tao ESP32 LAN Client
    esp_cfg = cfg.get("esp32_lan")
    esp32_client = ESP32LANClient(
        base_url=esp_cfg.get("base_url", "http://beca.local"),
        timeout=esp_cfg.get("timeout_sec", 2.0)
    )

    # 5. Khoi tao ThingsBoard AI MQTT Client (Device rieng)
    tb_cfg = cfg.get("thingsboard")
    mqtt_ai = MQTTAIClient(
        host=tb_cfg.get("host", "demo.thingsboard.io"),
        port=tb_cfg.get("port", 1883),
        access_token=tb_cfg.get("access_token", "")
    )
    if tb_cfg.get("enabled", False):
        mqtt_ai.start()

    # 6. Khoi tao Telegram Bot Service
    tg_cfg = cfg.get("telegram")
    telegram_bot = TelegramBotService(
        bot_token=tg_cfg.get("bot_token", ""),
        chat_id=tg_cfg.get("chat_id", ""),
        cooldown_minutes=tg_cfg.get("cooldown_minutes", 10),
        video_stream=video_stream,
        gemini_ai=gemini_ai,
        esp32_client=esp32_client
    )
    if tg_cfg.get("enabled", False):
        telegram_bot.start()

    # Trang thai dung chung (Shared State)
    shared_state = {
        "ai_result": {
            "total_fish": 0, "dead_fish": 0, "abnormal_fish": 0,
            "water_turbidity": 10, "is_alert": False,
            "summary": "Dang khoi tao he thong AI...", "ai_engine": "Init"
        }
    }

    # 7. Vong lap phan tich AI dinh ky tren Background Thread
    analyze_interval = cam_cfg.get("analyze_interval_sec", 10)

    def ai_worker_loop():
        print(f"[AI WORKER] Bat dau chu ky phan tich anh dinh ky moi {analyze_interval}s...")
        while True:
            try:
                frame = video_stream.get_frame()
                if frame is not None:
                    res = gemini_ai.analyze_frame(frame)
                    shared_state["ai_result"] = res
                    
                    # Day Telemetry len ThingsBoard
                    if tb_cfg.get("enabled", False):
                        mqtt_ai.publish_ai_telemetry(res, fps=video_stream.actual_fps)
                        
                    # Kiem tra canh bao Telegram
                    if tg_cfg.get("enabled", False):
                        telegram_bot.check_and_send_alert(res)
                        
            except Exception as e:
                print(f"[AI WORKER ERROR] {e}")
                
            time.sleep(analyze_interval)

    ai_thread = threading.Thread(target=ai_worker_loop, daemon=True)
    ai_thread.start()

    # 8. Khoi chay Web Server Flask
    web_cfg = cfg.get("web")
    app = create_app(video_stream, gemini_ai, esp32_client, shared_state)
    
    host = web_cfg.get("host", "0.0.0.0")
    port = web_cfg.get("port", 5000)
    print(f"[WEB SERVER] Dang chay tai: http://{host}:{port}/ (hoac http://localhost:{port}/)")
    app.run(host=host, port=port, debug=False, threaded=True)

if __name__ == "__main__":
    main()
