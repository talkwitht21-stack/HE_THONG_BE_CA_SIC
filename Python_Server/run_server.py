import time
import threading
import os
import sys

from core.config_manager import ConfigManager
from core.video_stream import VideoStream
from core.gemini_vision import GeminiVisionAnalyzer
from core.telegram_bot import TelegramBotService
from core.cloudflare_tunnel import CloudflareTunnel
from gateway.mqtt_ai_client import MQTTAIClient
from gateway.esp32_lan_client import ESP32LANClient
from web.app import create_app

def main():
    print("================================================================")
    print("   HE THONG BE CA SIC - PYTHON SERVER (EDGE AI & IOT GATEWAY)   ")
    print("================================================================")

    # 1. Tai cau hinh
    cfg = ConfigManager("config.yaml")

    # Trang thai dung chung (Shared State)
    total_fish_init = cfg.get("aquarium", "total_fish", 10)
    chat_id_init = cfg.get("telegram", "chat_id", "")

    shared_state = {
        "total_fish_configured": total_fish_init,
        "telegram_chat_id": chat_id_init,
        "public_url": "",
        "ai_result": {
            "dead_fish": 0, "abnormal_fish": 0,
            "water_turbidity": 10, "is_alert": False,
            "summary": "Dang khoi tao he thong AI...", "ai_engine": "Init",
            "analyzed_at": time.strftime("%H:%M:%S")
        }
    }

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
        chat_id=chat_id_init,
        cooldown_minutes=tg_cfg.get("cooldown_minutes", 10),
        video_stream=video_stream,
        gemini_ai=gemini_ai,
        esp32_client=esp32_client,
        shared_state=shared_state
    )
    if tg_cfg.get("enabled", False):
        telegram_bot.start()

    # 7. Khoi tao Cloudflare Tunnel
    web_cfg = cfg.get("web")
    port = web_cfg.get("port", 5000)

    def on_cloudflare_ready(pub_url):
        shared_state["public_url"] = pub_url
        if tg_cfg.get("enabled", False):
            telegram_bot.send_tunnel_url(pub_url)

    if web_cfg.get("enable_cloudflare", True):
        tunnel = CloudflareTunnel(port=port, on_url_ready=on_cloudflare_ready)
        tunnel.start()

    # 8. Vong lap phan tich AI dinh ky linh hoat:
    # - Binh thuong: Moi 2 phut (120s) chup 1 lan
    # - Khi phat hien ca chet (dead_fish > 0): Chup lien tuc moi 10s
    interval_normal = cam_cfg.get("interval_normal_sec", 120)
    interval_alert = cam_cfg.get("interval_alert_sec", 10)

    def ai_worker_loop():
        print(f"[AI WORKER] Bat dau chu ky: Binh thuong {interval_normal}s / Khi co ca chet {interval_alert}s...")
        while True:
            try:
                frame = video_stream.get_frame()
                if frame is not None:
                    res = gemini_ai.analyze_frame(frame)
                    shared_state["ai_result"] = res
                    
                    dead = res.get("dead_fish", 0)
                    total = shared_state.get("total_fish_configured", 10)
                    alive = max(0, total - dead)
                    
                    # Day Telemetry len ThingsBoard Device rieng
                    if tb_cfg.get("enabled", False):
                        ai_telemetry = {
                            "total_fish": total,
                            "alive_fish": alive,
                            "dead_fish": dead,
                            "water_turbidity": res.get("water_turbidity", 0),
                            "summary": res.get("summary", ""),
                            "ai_engine": res.get("ai_engine", "AI"),
                            "is_alert": res.get("is_alert", False)
                        }
                        mqtt_ai.publish_ai_telemetry(ai_telemetry, fps=video_stream.actual_fps)
                        
                    # Kiem tra gui canh bao Telegram
                    if tg_cfg.get("enabled", False):
                        telegram_bot.check_and_send_alert(res)

                    # Tinh toan thoi gian sleep tiep theo
                    sleep_time = interval_alert if dead > 0 else interval_normal
                    time.sleep(sleep_time)
                else:
                    time.sleep(5.0)
            except Exception as e:
                print(f"[AI WORKER ERROR] {e}")
                time.sleep(5.0)

    ai_thread = threading.Thread(target=ai_worker_loop, daemon=True)
    ai_thread.start()

    # 9. Khoi chay Web Server Flask
    app = create_app(video_stream, gemini_ai, esp32_client, shared_state, config_mgr=cfg, telegram_bot=telegram_bot)
    host = web_cfg.get("host", "0.0.0.0")
    print(f"[WEB SERVER] Dang chay tai: http://{host}:{port}/ (hoac http://localhost:{port}/)")
    app.run(host=host, port=port, debug=False, threaded=True)

if __name__ == "__main__":
    main()
