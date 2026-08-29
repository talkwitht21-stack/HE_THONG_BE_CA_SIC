import cv2
import time
import threading
import requests
import io

class TelegramBotService:
    """
    Dịch vụ Telegram Bot:
      - Tự động gửi cảnh báo khẩn cấp đính kèm ảnh snapshot khi có sự cố.
      - Có cơ chế Cooldown chống spam.
      - Hỗ trợ tương tác 2 chiều: /status, /snapshot, /data, /feed.
    """
    def __init__(self, bot_token, chat_id, cooldown_minutes=10, video_stream=None, gemini_ai=None, esp32_client=None):
        self.bot_token = bot_token
        self.chat_id = str(chat_id)
        self.cooldown_sec = cooldown_minutes * 60
        self.video_stream = video_stream
        self.gemini_ai = gemini_ai
        self.esp32_client = esp32_client
        
        self.enabled = bool(bot_token and chat_id and bot_token != "YOUR_TELEGRAM_BOT_TOKEN")
        self.last_alert_time = 0
        self.last_update_id = 0
        self.running = False
        self.thread = None

    def start(self):
        if not self.enabled:
            print("[TELEGRAM WARN] Telegram Bot chua duoc cau hinh Token hoac Chat ID.")
            return
            
        self.running = True
        self.thread = threading.Thread(target=self._polling_loop, daemon=True)
        self.thread.start()
        print(f"[TELEGRAM] Bot da khoi dong va san sang nhan lenh cho Chat ID: {self.chat_id}")
        self.send_message("<b>HE THONG BE CA SIC</b>\nBot AI da khoi dong thanh cong tren Raspberry Pi 5!\nGo <code>/help</code> de xem danh sach lenh.")

    def send_message(self, text):
        if not self.enabled:
            return False
        try:
            url = f"https://api.telegram.org/bot{self.bot_token}/sendMessage"
            payload = {"chat_id": self.chat_id, "text": text, "parse_mode": "HTML"}
            r = requests.post(url, json=payload, timeout=5)
            return r.status_code == 200
        except Exception as e:
            print(f"[TELEGRAM ERROR] Loi gui text: {e}")
            return False

    def send_photo(self, image_bytes, caption=""):
        if not self.enabled or not image_bytes:
            return False
        try:
            url = f"https://api.telegram.org/bot{self.bot_token}/sendPhoto"
            files = {"photo": ("snapshot.jpg", image_bytes, "image/jpeg")}
            data = {"chat_id": self.chat_id, "caption": caption, "parse_mode": "HTML"}
            r = requests.post(url, data=data, files=files, timeout=10)
            return r.status_code == 200
        except Exception as e:
            print(f"[TELEGRAM ERROR] Loi gui anh: {e}")
            return False

    def check_and_send_alert(self, ai_result):
        """
        Kiểm tra nếu có sự cố nguy hiểm và gửi cảnh báo (có chống spam cooldown).
        """
        if not self.enabled:
            return
            
        is_alert = ai_result.get("is_alert", False)
        dead = ai_result.get("dead_fish", 0)
        turb = ai_result.get("water_turbidity", 0)
        
        if not is_alert and dead == 0 and turb < 40:
            return
            
        now = time.time()
        if now - self.last_alert_time < self.cooldown_sec:
            return # Dang trong thoi gian cooldown
            
        self.last_alert_time = now
        
        alert_msg = f"<b>CANH BAO KHAN CAP TU BE CA!</b>\n"
        if dead > 0:
            alert_msg += f"- Phat hien: <b>{dead} ca the bat thuong / tu vong!</b>\n"
        if turb >= 40:
            alert_msg += f"- Do duc cua nuoc rat cao: <b>{turb}%</b>\n"
        alert_msg += f"- Nhan xet AI: <i>{ai_result.get('summary', '')}</i>\n"
        alert_msg += f"- Thoi gian: {time.strftime('%H:%M:%S %d/%m/%Y')}"
        
        img_bytes = self.video_stream.get_jpeg_bytes() if self.video_stream else None
        if img_bytes:
            self.send_photo(img_bytes, caption=alert_msg)
        else:
            self.send_message(alert_msg)
        print(f"[TELEGRAM ALERT SENT] Da gui canh bao vao Telegram: {alert_msg}")

    def _polling_loop(self):
        """
        Lắng nghe tin nhắn và lệnh từ người dùng trên Telegram.
        """
        while self.running:
            try:
                url = f"https://api.telegram.org/bot{self.bot_token}/getUpdates"
                params = {"offset": self.last_update_id + 1, "timeout": 20}
                r = requests.get(url, params=params, timeout=25)
                
                if r.status_code == 200:
                    data = r.json()
                    for update in data.get("result", []):
                        self.last_update_id = update["update_id"]
                        msg = update.get("message", {})
                        from_chat_id = str(msg.get("chat", {}).get("id", ""))
                        text = msg.get("text", "").strip()
                        
                        # Chi phan hoi cho dung Chat ID cau hinh
                        if from_chat_id == self.chat_id:
                            self._handle_command(text)
            except Exception as e:
                time.sleep(3.0)
            time.sleep(0.5)

    def _handle_command(self, cmd_text):
        cmd = cmd_text.lower()
        if cmd == "/help" or cmd == "/start":
            msg = (
                "<b>DANH SACH LENH TELEGRAM BOT BE CA SIC:</b>\n"
                "• <code>/status</code>: Chup anh va phan tich tinh trang AI ngay lap tuc\n"
                "• <code>/snapshot</code>: Chup va gui anh truc tiep tu Camera\n"
                "• <code>/data</code>: Doc cac thong so moi truong tu ESP32\n"
                "• <code>/feed</code>: Kich hoat quay Servo cho ca an tu xa\n"
            )
            self.send_message(msg)

        elif cmd == "/snapshot":
            img_bytes = self.video_stream.get_jpeg_bytes() if self.video_stream else None
            if img_bytes:
                caption = f"Anh chup truc tiep luc: {time.strftime('%H:%M:%S %d/%m/%Y')}"
                self.send_photo(img_bytes, caption=caption)
            else:
                self.send_message("Khong the lay anh tu Camera.")

        elif cmd == "/status":
            self.send_message("Dang chup anh va phan tich qua Gemini Vision AI, vui long doi giay lat...")
            frame = self.video_stream.get_frame() if self.video_stream else None
            ai_res = self.gemini_ai.analyze_frame(frame) if self.gemini_ai else {}
            
            caption = (
                f"<b>KET QUA PHAN TICH AI ({ai_res.get('ai_engine', 'AI')})</b>\n"
                f"• Tong so ca: <b>{ai_res.get('total_fish', 0)}</b>\n"
                f"• Ca bat thuong/chet: <b>{ai_res.get('dead_fish', 0)}</b>\n"
                f"• Do duc nuoc: <b>{ai_res.get('water_turbidity', 0)}%</b>\n"
                f"• Nhan xet: <i>{ai_res.get('summary', '')}</i>\n"
                f"• Thoi gian: {time.strftime('%H:%M:%S %d/%m/%Y')}"
            )
            img_bytes = self.video_stream.get_jpeg_bytes() if self.video_stream else None
            if img_bytes:
                self.send_photo(img_bytes, caption=caption)
            else:
                self.send_message(caption)

        elif cmd == "/data":
            if self.esp32_client:
                data = self.esp32_client.get_data()
                if data:
                    msg = (
                        f"<b>THONG SO CAM BIEN BE CA (ESP32):</b>\n"
                        f"• Nhiet do nuoc: <b>{data.get('wt', '--')} °C</b>\n"
                        f"• Nhiet do K.Khi: <b>{data.get('at', '--')} °C</b>\n"
                        f"• Do am K.Khi: <b>{data.get('ah', '--')} %</b>\n"
                        f"• Muc nuoc cach nap: <b>{data.get('wcm', '--')} cm</b>\n"
                        f"• Suoi: <b>{'BAT' if data.get('h') else 'TAT'}</b> | Quat: <b>{'BAT' if data.get('f') else 'TAT'}</b>\n"
                        f"• Bom bu: <b>{'BAT' if data.get('p') else 'TAT'}</b> | Oxy: <b>{'BAT' if data.get('o') else 'TAT'}</b>\n"
                        f"• Gio he thong: {data.get('time', '--')}"
                    )
                    self.send_message(msg)
                else:
                    self.send_message("Khong the ket noi toi ESP32 qua LAN.")
            else:
                self.send_message("Chua khoi tao ket noi ESP32 LAN.")

        elif cmd == "/feed":
            if self.esp32_client:
                ok = self.esp32_client.control_device("feed")
                if ok:
                    self.send_message("Da gui lenh rot thuc an xuong ESP32 thanh cong!")
                else:
                    self.send_message("Loi gui lenh rot thuc an xuong ESP32.")
            else:
                self.send_message("Chua khoi tao ket noi ESP32 LAN.")
