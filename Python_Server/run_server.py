import time
import threading
import os
import sys

from core.config_manager import ConfigManager
from core.reference_manager import ReferenceManager
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

    total_fish_init = cfg.get("aquarium", "total_fish", 10)
    chat_id_init = cfg.get("telegram", "chat_id", "")
    cam_cfg = cfg.get("camera")
    interval_normal_init = cam_cfg.get("interval_normal_sec", 120)
    interval_alert_init = cam_cfg.get("interval_alert_sec", 10)

    # Trang thai dung chung (Shared State)
    shared_state = {
        "total_fish_configured": total_fish_init,
        "telegram_chat_id": chat_id_init,
        "public_url": "",
        "confirmed_dead_fish": 0,
        "verify_progress": "0/5",
        "interval_normal_sec": interval_normal_init,
        "interval_alert_sec": interval_alert_init,
        "auto_turb_filter_active": False,
        "ai_result": {
            "dead_fish": 0, "abnormal_fish": 0,
            "water_turbidity": 10, "is_alert": False,
            "summary": "Dang khoi tao he thong AI...", "ai_engine": "Init",
            "analyzed_at": time.strftime("%H:%M:%S")
        }
    }

    # 2. Khoi tao Reference Manager (Quan ly 5 anh mau tham chieu)
    ref_manager = ReferenceManager("reference_images", max_slots=5)

    # 3. Khoi tao Camera Video Stream
    video_stream = VideoStream(
        source=cam_cfg.get("source", 0),
        width=cam_cfg.get("width", 640),
        height=cam_cfg.get("height", 480),
        fps=cam_cfg.get("fps", 15)
    )
    video_stream.start()

    # 4. Khoi tao Gemini Vision AI (Co doi chieu 5 anh mau)
    gem_cfg = cfg.get("gemini")
    gemini_ai = GeminiVisionAnalyzer(
        api_key=gem_cfg.get("api_key", ""),
        model_name=gem_cfg.get("model", "gemini-2.0-flash"),
        temperature=gem_cfg.get("temperature", 0.2),
        ref_manager=ref_manager
    )

    # 5. Khoi tao ESP32 LAN Client
    esp_cfg = cfg.get("esp32_lan")
    esp32_client = ESP32LANClient(
        base_url=esp_cfg.get("base_url", "http://beca.local"),
        timeout=esp_cfg.get("timeout_sec", 2.0)
    )

    # 6. Khoi tao ThingsBoard AI MQTT Client (Device rieng)
    tb_cfg = cfg.get("thingsboard")
    mqtt_ai = MQTTAIClient(
        host=tb_cfg.get("host", "thingsboard.cloud"),
        port=tb_cfg.get("port", 1883),
        access_token=tb_cfg.get("access_token", "")
    )
    if tb_cfg.get("enabled", False):
        mqtt_ai.start()

    # 7. Khoi tao Telegram Bot Service
    tg_cfg = cfg.get("telegram")
    telegram_bot = TelegramBotService(
        bot_token=tg_cfg.get("bot_token", ""),
        chat_id=chat_id_init,
        cooldown_minutes=tg_cfg.get("cooldown_minutes", 10),
        video_stream=video_stream,
        gemini_ai=gemini_ai,
        esp32_client=esp32_client,
        shared_state=shared_state,
        ref_manager=ref_manager
    )
    if telegram_bot.enabled or tg_cfg.get("enabled", False):
        telegram_bot.start()

    # 8. Khoi tao Cloudflare Tunnel
    web_cfg = cfg.get("web")
    port = web_cfg.get("port", 5000)

    def on_cloudflare_ready(pub_url):
        shared_state["public_url"] = pub_url
        if telegram_bot:
            telegram_bot.send_tunnel_url(pub_url)

    tunnel = None
    if web_cfg.get("enable_cloudflare", True):
        tunnel = CloudflareTunnel(port=port, on_url_ready=on_cloudflare_ready)
        tunnel.start()

    # 9. Vong lap phan tich AI voi co che XAC THUC 5 LAN LIEN TIEP (Chu ky dong):
    CONFIRM_THRESHOLD = 5

    def ai_worker_loop():
        dead_streak = 0
        confirmed_dead = 0
        print("[AI WORKER] Bat dau luong AI doc lap voi chu ky lay mau dong...")
        
        while True:
            try:
                frame = video_stream.get_frame()
                if frame is not None:
                    res = gemini_ai.analyze_frame(frame)
                    raw_dead = res.get("dead_fish", 0)
                    turb = res.get("water_turbidity", 0)

                    # --- TỰ ĐỘNG CHẠY MÁY LỌC KHI ĐỘ ĐỤC >= 70% CHO ĐẾN KHI < 60% ---
                    auto_turb_active = shared_state.get("auto_turb_filter_active", False)
                    if turb >= 70:
                        if not auto_turb_active:
                            shared_state["auto_turb_filter_active"] = True
                            print(f"[AI AUTO-FILTER] PHAT HIEN DO DUC CAO ({turb}% >= 70%) -> TU DONG BAT MAY LOC NUOC LIEN TUC!")
                            if esp32_client:
                                esp_data = esp32_client.get_data()
                                if not esp_data.get("fl", False):
                                    esp32_client.control_device("filter")
                            if telegram_bot and telegram_bot.enabled:
                                telegram_bot.send_message(
                                    f"🚨 <b>[CẢNH BÁO NƯỚC ĐỤC & TỰ ĐỘNG LỌC]</b>\n"
                                    f"• Độ đục nước đo được: <b>{turb}%</b> (Vượt ngưỡng 70%)\n"
                                    f"• Hệ thống AI đã <b>TỰ ĐỘNG BẬT MÁY LỌC NƯỚC LIÊN TỤC</b> để xử lý nước cho đến khi độ đục giảm xuống dưới 60%!"
                                )
                    elif turb < 60:
                        if auto_turb_active:
                            shared_state["auto_turb_filter_active"] = False
                            print(f"[AI AUTO-FILTER] DO DUC DA GIAM AN TOAN ({turb}% < 60%) -> TU DONG TAT MAY LOC NUOC TANG CUONG!")
                            if esp32_client:
                                esp_data = esp32_client.get_data()
                                if esp_data.get("fl", False):
                                    esp32_client.control_device("filter")
                            if telegram_bot and telegram_bot.enabled:
                                telegram_bot.send_message(
                                    f"✅ <b>[NƯỚC ĐÃ TRONG SẠCH]</b>\n"
                                    f"• Độ đục nước hiện tại: <b>{turb}%</b> (Đã an toàn &lt; 60%)\n"
                                    f"• Hệ thống AI đã <b>TỰ ĐỘNG TẮT MÁY LỌC TĂNG CƯỜNG</b>."
                                )
                    
                    if raw_dead > 0:
                        dead_streak += 1
                        shared_state["verify_progress"] = f"{dead_streak}/{CONFIRM_THRESHOLD}"
                        print(f"[AI VERIFY] Phat hien nghi ngo ca chet ({raw_dead} con): Xac thuc lan {dead_streak}/{CONFIRM_THRESHOLD}...")
                        
                        if dead_streak >= CONFIRM_THRESHOLD:
                            confirmed_dead = raw_dead
                            res["confirmed_dead_fish"] = confirmed_dead
                            res["is_alert"] = True
                            shared_state["confirmed_dead_fish"] = confirmed_dead
                            shared_state["ai_result"] = res
                            
                            if telegram_bot and telegram_bot.enabled:
                                telegram_bot.check_and_send_alert(res)
                                
                            print(f"[AI CONFIRMED] DA XAC THUC CHINH XAC 100% {confirmed_dead} CA THE BI CHET!")
                        else:
                            res["confirmed_dead_fish"] = confirmed_dead
                            shared_state["ai_result"] = res
                            
                        if mqtt_ai and mqtt_ai.enabled:
                            total = shared_state.get("total_fish_configured", 10)
                            alive = max(0, total - confirmed_dead)
                            ai_telemetry = {
                                "total_fish": total,
                                "alive_fish": alive,
                                "dead_fish": confirmed_dead,
                                "water_turbidity": turb,
                                "summary": res.get("summary", ""),
                                "ai_engine": res.get("ai_engine", "Gemini 3.5 Flash VLM"),
                                "is_alert": (confirmed_dead > 0 or turb >= 40)
                            }
                            mqtt_ai.publish_ai_telemetry(ai_telemetry, fps=video_stream.actual_fps)
                            
                        # Chu ky xac thuc lay dong tu shared_state
                        wait_sec = shared_state.get("interval_alert_sec", 10)
                        for _ in range(wait_sec):
                            time.sleep(1)
                    else:
                        if dead_streak > 0:
                            print(f"[AI RESET] Ca da boi lai binh thuong sau {dead_streak} lan nghi ngo. Reset streak!")
                            dead_streak = 0
                            shared_state["verify_progress"] = "0/5"
                            
                        confirmed_dead = 0
                        res["confirmed_dead_fish"] = 0
                        res["is_alert"] = False
                        shared_state["confirmed_dead_fish"] = 0
                        shared_state["ai_result"] = res
                        
                        if mqtt_ai and mqtt_ai.enabled:
                            total = shared_state.get("total_fish_configured", 10)
                            ai_telemetry = {
                                "total_fish": total,
                                "alive_fish": total,
                                "dead_fish": 0,
                                "water_turbidity": turb,
                                "summary": res.get("summary", ""),
                                "ai_engine": res.get("ai_engine", "Gemini 3.5 Flash VLM"),
                                "is_alert": (turb >= 40)
                            }
                            mqtt_ai.publish_ai_telemetry(ai_telemetry, fps=video_stream.actual_fps)
                            
                        # Chu ky binh thuong lay dong tu shared_state
                        wait_sec = shared_state.get("interval_normal_sec", 120)
                        for _ in range(wait_sec):
                            time.sleep(1)
                            
                else:
                    time.sleep(2)
            except Exception as e:
                print(f"[AI WORKER ERROR] {e}")
                time.sleep(5)

    ai_thread = threading.Thread(target=ai_worker_loop, daemon=True)
    ai_thread.start()

    # 10. Khoi chay Web Server Flask
    app = create_app(
        video_stream=video_stream,
        gemini_ai=gemini_ai,
        esp32_client=esp32_client,
        shared_state=shared_state,
        config_mgr=cfg,
        telegram_bot=telegram_bot,
        ref_manager=ref_manager,
        mqtt_ai=mqtt_ai,
        tunnel=tunnel
    )

    host = web_cfg.get("host", "0.0.0.0")
    print("=" * 65)
    print(f" WEB DASHBOARD SAN SANG TAI: http://localhost:{port}")
    print("=" * 65)
    app.run(host=host, port=port, debug=False, use_reloader=False)

if __name__ == "__main__":
    main()
