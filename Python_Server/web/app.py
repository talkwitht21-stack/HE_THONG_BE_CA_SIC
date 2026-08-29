from flask import Flask, render_template, Response, jsonify, request, send_file
import time
import os
import io

def create_app(video_stream, gemini_ai, esp32_client, shared_state, config_mgr=None, telegram_bot=None, ref_manager=None, mqtt_ai=None):
    app = Flask(__name__, template_folder="templates")
    app.config["SECRET_KEY"] = "beca_secret_key"

    @app.route("/")
    def index():
        return render_template("index.html")

    def gen_frames():
        while True:
            ai_info = shared_state.get("ai_result", {})
            jpeg = video_stream.get_jpeg_bytes(overlay_info=ai_info)
            if jpeg:
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + jpeg + b'\r\n')
            time.sleep(0.04)

    @app.route("/video_feed")
    def video_feed():
        return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

    @app.route("/api/status")
    def api_status():
        esp_data = esp32_client.get_data() if esp32_client else {}
        ref_status = ref_manager.get_slot_status() if ref_manager else []
        
        current_api_key = config_mgr.get("gemini", "api_key", "") if config_mgr else ""
        has_key = bool(current_api_key and current_api_key != "YOUR_GEMINI_API_KEY")
        masked_key = (current_api_key[:6] + "..." + current_api_key[-4:]) if len(current_api_key) > 10 else ("Đã lưu" if has_key else "")

        current_tb_token = config_mgr.get("thingsboard", "access_token", "") if config_mgr else ""
        has_tb = bool(current_tb_token and current_tb_token != "YOUR_RPI5_AI_DEVICE_TOKEN")
        masked_tb = (current_tb_token[:6] + "..." + current_tb_token[-4:]) if len(current_tb_token) > 10 else ("Đã lưu" if has_tb else "")

        current_tg_token = config_mgr.get("telegram", "bot_token", "") if config_mgr else ""
        has_tg = bool(current_tg_token and current_tg_token != "YOUR_TELEGRAM_BOT_TOKEN")
        masked_tg = (current_tg_token[:6] + "..." + current_tg_token[-4:]) if len(current_tg_token) > 10 else ("Đã lưu" if has_tg else "")

        tb_dash_url = config_mgr.get("thingsboard", "dashboard_url", "https://thingsboard.cloud/dashboard/1f6621a0-a3ae-11f1-9b46-e7fbeb690c95?publicId=05e0b4a0-a3b0-11f1-8523-a9586d32bc6e") if config_mgr else ""
        cam_src = config_mgr.get("camera", "source", 0) if config_mgr else 0

        return jsonify({
            "ai": shared_state.get("ai_result", {}),
            "confirmed_dead_fish": shared_state.get("confirmed_dead_fish", 0),
            "verify_progress": shared_state.get("verify_progress", "0/5"),
            "total_fish_configured": shared_state.get("total_fish_configured", 10),
            "public_url": shared_state.get("public_url", ""),
            "telegram_chat_id": shared_state.get("telegram_chat_id", ""),
            "has_gemini_key": has_key,
            "gemini_api_key_masked": masked_key,
            "has_tb_token": has_tb,
            "tb_token_masked": masked_tb,
            "has_tg_token": has_tg,
            "tg_token_masked": masked_tg,
            "tb_dashboard_url": tb_dash_url,
            "camera_source": str(cam_src),
            "fps": video_stream.actual_fps if video_stream else 0.0,
            "ref_status": ref_status,
            "esp32": esp_data,
            "timestamp": time.time()
        })

    @app.route("/api/instant_analyze", methods=["POST", "GET"])
    def api_instant_analyze():
        frame = video_stream.get_frame() if video_stream else None
        if frame is not None and gemini_ai:
            res = gemini_ai.analyze_frame(frame)
            dead = res.get("dead_fish", 0)
            res["confirmed_dead_fish"] = dead
            shared_state["ai_result"] = res
            shared_state["confirmed_dead_fish"] = dead
            return jsonify({"status": "success", "data": res})
        return jsonify({"status": "error", "message": "Camera hoac AI chua san sang"}), 500

    # API QUAN LY 5 ANH MAU THAM CHIEU
    @app.route("/api/reference/status", methods=["GET"])
    def api_ref_status():
        if ref_manager:
            return jsonify(ref_manager.get_slot_status())
        return jsonify([])

    @app.route("/api/reference/image/<int:slot_id>")
    def api_ref_image(slot_id):
        if ref_manager:
            path = ref_manager.get_slot_path(slot_id)
            if path and os.path.exists(path):
                return send_file(path, mimetype="image/jpeg")
        return "Image not found", 404

    @app.route("/api/reference/capture", methods=["POST"])
    def api_ref_capture():
        data = request.get_json() or {}
        slot_id = data.get("slot")
        if ref_manager and slot_id:
            frame = video_stream.get_frame() if video_stream else None
            if frame is not None:
                ok = ref_manager.save_frame_to_slot(slot_id, frame)
                return jsonify({"status": "success" if ok else "failed", "slot": slot_id})
        return jsonify({"status": "error", "message": "Khong the chup anh mau"}), 400

    @app.route("/api/reference/upload", methods=["POST"])
    def api_ref_upload():
        slot_id = request.form.get("slot")
        file = request.files.get("file")
        if ref_manager and slot_id and file:
            img_bytes = file.read()
            ok = ref_manager.save_bytes_to_slot(int(slot_id), img_bytes)
            return jsonify({"status": "success" if ok else "failed", "slot": slot_id})
        return jsonify({"status": "error", "message": "Du lieu tai len khong hop le"}), 400

    @app.route("/api/reference/delete", methods=["POST"])
    def api_ref_delete():
        data = request.get_json() or {}
        slot_id = data.get("slot")
        if ref_manager and slot_id:
            ok = ref_manager.delete_slot(int(slot_id))
            return jsonify({"status": "success" if ok else "failed", "slot": slot_id})
        return jsonify({"status": "error"}), 400

    @app.route("/api/update_settings", methods=["POST"])
    def api_update_settings():
        data = request.get_json() or {}
        changed = False

        if "camera_source" in data:
            src = str(data["camera_source"]).strip()
            if src:
                src_val = int(src) if src.isdigit() else src
                if config_mgr:
                    config_mgr.set("camera", "source", src_val)
                if video_stream:
                    video_stream.update_source(src_val)
                changed = True

        if "gemini_api_key" in data:
            key = str(data["gemini_api_key"]).strip()
            if key:
                if config_mgr:
                    config_mgr.set("gemini", "api_key", key)
                    config_mgr.set("gemini", "enabled", True)
                if gemini_ai:
                    gemini_ai.update_api_key(key)
                changed = True

        if "thingsboard_token" in data:
            tb_tok = str(data["thingsboard_token"]).strip()
            if tb_tok:
                if config_mgr:
                    config_mgr.set("thingsboard", "access_token", tb_tok)
                    config_mgr.set("thingsboard", "enabled", True)
                if mqtt_ai:
                    mqtt_ai.update_token(tb_tok)
                changed = True

        if "thingsboard_dashboard_url" in data:
            tb_url = str(data["thingsboard_dashboard_url"]).strip()
            if tb_url:
                if config_mgr:
                    config_mgr.set("thingsboard", "dashboard_url", tb_url)
                changed = True

        if "telegram_bot_token" in data:
            tg_tok = str(data["telegram_bot_token"]).strip()
            if tg_tok:
                if config_mgr:
                    config_mgr.set("telegram", "bot_token", tg_tok)
                    config_mgr.set("telegram", "enabled", True)
                if telegram_bot:
                    telegram_bot.update_bot_token(tg_tok)
                changed = True

        if "total_fish" in data:
            try:
                tf = int(data["total_fish"])
                shared_state["total_fish_configured"] = tf
                if config_mgr:
                    config_mgr.set("aquarium", "total_fish", tf)
                changed = True
            except ValueError:
                pass

        if "telegram_chat_id" in data:
            cid = str(data["telegram_chat_id"]).strip()
            shared_state["telegram_chat_id"] = cid
            if config_mgr:
                config_mgr.set("telegram", "chat_id", cid)
            if telegram_bot:
                telegram_bot.update_chat_id(cid)
            changed = True

        if changed and config_mgr:
            config_mgr.save_config()

        return jsonify({"status": "success", "message": "Da luu cai dat thanh cong!"})

    @app.route("/api/lan_control", methods=["POST"])
    def api_lan_control():
        data = request.get_json() or {}
        device = data.get("device")
        if esp32_client and device:
            ok = esp32_client.control_device(device)
            return jsonify({"status": "success" if ok else "failed", "device": device})
        return jsonify({"status": "error", "message": "Invalid request"}), 400

    return app
