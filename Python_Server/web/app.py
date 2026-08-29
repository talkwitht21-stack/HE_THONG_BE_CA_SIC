from flask import Flask, render_template, Response, jsonify, request
import os
import time

def create_app(video_stream, gemini_ai, esp32_client, shared_state):
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
        return jsonify({
            "ai": shared_state.get("ai_result", {}),
            "fps": video_stream.actual_fps if video_stream else 0.0,
            "esp32": esp_data,
            "timestamp": time.time()
        })

    @app.route("/api/lan_control", methods=["POST"])
    def api_lan_control():
        data = request.get_json() or {}
        device = data.get("device")
        if esp32_client and device:
            ok = esp32_client.control_device(device)
            return jsonify({"status": "success" if ok else "failed", "device": device})
        return jsonify({"status": "error", "message": "Invalid request"}), 400

    return app
