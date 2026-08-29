from flask import Flask, render_template, Response, jsonify, request, send_file
import time
import os
import io

def create_app(video_stream, gemini_ai, esp32_client, shared_state, config_mgr=None, telegram_bot=None, ref_manager=None):
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
        return jsonify({
            "ai": shared_state.get("ai_result", {}),
            "confirmed_dead_fish": shared_state.get("confirmed_dead_fish", 0),
            "verify_progress": shared_state.get("verify_progress", "0/5"),
            "total_fish_configured": shared_state.get("total_fish_configured", 10),
            "public_url": shared_state.get("public_url", ""),
            "telegram_chat_id": shared_state.get("telegram_chat_id", ""),
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
