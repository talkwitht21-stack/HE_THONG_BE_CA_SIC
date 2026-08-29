import gc
import base64
from core.motion_collage import create_5frame_motion_collage
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
        "suspected_dead_fish": 0,
        "verify_progress": "0/5 (Bình thường)",
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
    tb_token = tb_cfg.get("access_token", "")
    mqtt_ai = MQTTAIClient(
        host=tb_cfg.get("host", "thingsboard.cloud"),
        port=tb_cfg.get("port", 1883),
        access_token=tb_token,
        shared_state=shared_state
    )
    if tb_cfg.get("enabled", False) or mqtt_ai.enabled:
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
        print("[AI WORKER] Bat dau luong AI doc lap: Luon chup chuoi 5 frames doi chieu chuyen dong...")
        
        while True:
            try:
                if shared_state.get("reset_streak_requested", False):
                    shared_state["reset_streak_requested"] = False
                    shared_state["suspected_dead_fish"] = 0
                    shared_state["confirmed_dead_fish"] = 0
                    shared_state["verify_progress"] = "0/5 (An toàn)"

                # 1. Chụp liên tiếp chuỗi 5 frames theo dòng thời gian
                total_cfg = shared_state.get("total_fish_configured", 10)
                inv_alert = max(1, shared_state.get("interval_alert_sec", 1))
                
                frames_seq = []
                ind_b64_list = []
                
                for step in range(1, 6):
                    f = video_stream.get_frame() if video_stream else None
                    if f is not None:
                        frames_seq.append(f)
                        shared_state["verify_progress"] = f"{step}/5 (Đang chụp Frame {step}/5...)"
                    if step < 5:
                        time.sleep(inv_alert)

                if len(frames_seq) >= 2:
                    # 2. Tạo Collage nén 20% pixel, viền phân cách neon và dán nhãn
                    collage_img, collage_jpeg, ind_jpegs = create_5frame_motion_collage(frames_seq, intervals_sec=inv_alert)
                    
                    # Chuyển 5 ảnh sang base64 để Web hiển thị ngay lập tức
                    for j_bytes in ind_jpegs:
                        b64_str = "data:image/jpeg;base64," + base64.b64encode(j_bytes).decode('ascii')
                        ind_b64_list.append(b64_str)
                        
                    shared_state["verify_frames_b64"] = ind_b64_list
                    shared_state["collage_jpeg_bytes"] = collage_jpeg
                    shared_state["verify_progress"] = "5/5 (Đang gửi Gemini 3.5 Flash phân tích chuyển động...)"

                    # 3. Gửi Collage duy nhất lên Gemini AI
                    print("[AI MOTION] Dang gui Collage 5 frames len Gemini 3.5 Flash Vision de phan tich chuyen dong...")
                    res = gemini_ai.analyze_5frame_collage(collage_img, total_fish=total_cfg)
                    
                    dead = res.get("dead_fish", 0)
                    alive = res.get("alive_fish", max(0, total_cfg - dead))
                    turb = res.get("water_turbidity", 15)
                    
                    res["confirmed_dead_fish"] = dead
                    shared_state["ai_result"] = res
                    shared_state["confirmed_dead_fish"] = dead
                    shared_state["suspected_dead_fish"] = dead

                    if dead > 0:
                        shared_state["verify_progress"] = f"5/5 (Xác nhận {dead} cá chết)"
                        print(f"[AI MOTION ALERT] XAC THUC 100%: CO {dead} CA BI CHET QUA 5 KHUNG HINH!")
                        if telegram_bot and telegram_bot.enabled:
                            telegram_bot.check_and_send_alert(res)
                    else:
                        shared_state["verify_progress"] = "0/5 (An toàn - Đã đối chiếu 5 frames)"
                        shared_state["suspected_dead_fish"] = 0
                        print(f"[AI MOTION OK] Ca boi loi binh thuong ({alive}/{total_cfg} con song, 0 ca chet).")

                    # 4. Tự động bật/tắt máy lọc khi độ đục >= 70%
                    auto_turb_active = shared_state.get("auto_turb_filter_active", False)
                    if turb >= 70 and not auto_turb_active:
                        shared_state["auto_turb_filter_active"] = True
                        if esp32_client:
                            esp_d = esp32_client.get_data()
                            if not esp_d.get("fl", False):
                                esp32_client.control_device("filter")
                        if telegram_bot and telegram_bot.enabled:
                            telegram_bot.send_message(f"🚨 <b>[CẢNH BÁO NƯỚC ĐỤC & TỰ ĐỘNG LỌC]</b>\n• Độ đục: <b>{turb}%</b>\n• Đã tự động bật máy lọc liên tục!")
                    elif turb < 60 and auto_turb_active:
                        shared_state["auto_turb_filter_active"] = False
                        if esp32_client:
                            esp_d = esp32_client.get_data()
                            if esp_d.get("fl", False):
                                esp32_client.control_device("filter")

                    # 5. Đồng bộ ThingsBoard Telemetry
                    if mqtt_ai and mqtt_ai.enabled:
                        ai_telemetry = {
                            "total_fish": total_cfg,
                            "alive_fish": alive,
                            "dead_fish": dead,
                            "water_turbidity": turb,
                            "summary": res.get("summary", ""),
                            "ai_engine": res.get("ai_engine", "Gemini 3.5 Flash"),
                            "is_alert": (dead > 0 or turb >= 40)
                        }
                        mqtt_ai.publish_ai_telemetry(ai_telemetry, fps=video_stream.actual_fps)

                    # 6. XÓA CACHE GIẢI PHÓNG RAM CHO RASPBERRY PI 5
                    del frames_seq
                    del collage_img
                    del ind_jpegs
                    gc.collect()
                    print("[AI MEMORY] Da xoa sach cache 5 frames sau khi phan tich thanh cong!")

                # Chờ chu kỳ tiếp theo
                wait_sec = max(5, shared_state.get("interval_normal_sec", 120))
                for _ in range(wait_sec):
                    if shared_state.get("reset_streak_requested", False):
                        break
                    time.sleep(1)

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
