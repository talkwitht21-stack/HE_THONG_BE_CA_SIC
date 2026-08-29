import cv2
import time
import threading
import requests

class TelegramBotService:
    """
    Dịch vụ Telegram Bot thông minh:
      - Tự động gửi link Cloudflare Tunnel và thông báo khi khởi động.
      - Tự động chụp và gửi ảnh snapshot khi phát hiện cá chết hoặc nước đục.
      - Có cơ chế Cooldown chống spam.
      - Hỗ trợ tương tác 2 chiều: /status, /snapshot, /data, /feed, /url, /setref, /refstatus.
    """
    def __init__(self, bot_token, chat_id, cooldown_minutes=10, video_stream=None, gemini_ai=None, esp32_client=None, shared_state=None, ref_manager=None):
        self.bot_token = str(bot_token).strip()
        self.chat_id = str(chat_id).strip()
        self.cooldown_sec = cooldown_minutes * 60
        self.video_stream = video_stream
        self.gemini_ai = gemini_ai
        self.esp32_client = esp32_client
        self.shared_state = shared_state or {}
        self.ref_manager = ref_manager
        
        self.enabled = bool(self.bot_token and self.chat_id and self.bot_token != "YOUR_TELEGRAM_BOT_TOKEN" and self.chat_id != "YOUR_TELEGRAM_CHAT_ID")
        self.last_alert_time = 0
        self.last_update_id = 0
        self.running = False
        self.thread = None

    def update_chat_id(self, new_chat_id):
        self.chat_id = str(new_chat_id).strip()
        self.enabled = bool(self.bot_token and self.chat_id and self.bot_token != "YOUR_TELEGRAM_BOT_TOKEN" and self.chat_id != "YOUR_TELEGRAM_CHAT_ID")
        print(f"[TELEGRAM] Cap nhat Chat ID moi: {self.chat_id}")
        if self.enabled:
            if not self.running:
                self.start()
            pub_url = self.shared_state.get("public_url", "")
            if pub_url:
                self.send_tunnel_url(pub_url)
            else:
                self.send_message("✅ <b>Da lien ket thanh cong voi He Thong Be Ca SIC!</b>")

    def update_bot_token(self, new_token):
        self.bot_token = str(new_token).strip()
        self.enabled = bool(self.bot_token and self.chat_id and self.bot_token != "YOUR_TELEGRAM_BOT_TOKEN" and self.chat_id != "YOUR_TELEGRAM_CHAT_ID")
        print(f"[TELEGRAM] Cap nhat Bot Token moi thanh cong!")
        if self.enabled and not self.running:
            self.start()
        return self.enabled

    def start(self):
        tok = str(self.bot_token).strip()
        cid = str(self.chat_id).strip()
        if not tok or not cid or tok == "YOUR_TELEGRAM_BOT_TOKEN" or cid == "YOUR_TELEGRAM_CHAT_ID":
            print("[TELEGRAM WARN] Telegram Bot chua duoc cau hinh Token hoac Chat ID hop le.")
            return
            
        self.running = True
        self.enabled = True
        self.thread = threading.Thread(target=self._polling_loop, daemon=True)
        self.thread.start()
        print(f"[TELEGRAM] Bot da khoi dong cho Chat ID: {self.chat_id}")
        self.send_message("🟢 <b>HE THONG BE CA SIC DA KHOI DONG!</b>\nTrung tam AI & Gateway tren Raspberry Pi 5 da san sang 24/7!\nGo <code>/help</code> de xem danh sach lenh.")

    def send_message(self, text):
        tok = str(self.bot_token).strip()
        cid = str(self.chat_id).strip()
        if not tok or not cid or tok == "YOUR_TELEGRAM_BOT_TOKEN" or cid == "YOUR_TELEGRAM_CHAT_ID":
            return False
        try:
            url = f"https://api.telegram.org/bot{tok}/sendMessage"
            payload = {"chat_id": cid, "text": text, "parse_mode": "HTML"}
            r = requests.post(url, json=payload, timeout=8)
            if r.status_code == 200:
                print(f"[TELEGRAM] Da gui tin nhan thanh cong toi Chat ID: {cid}")
                return True
            else:
                print(f"[TELEGRAM WARN] Gui tin that bai (HTTP {r.status_code}): {r.text}")
                return False
        except Exception as e:
            print(f"[TELEGRAM ERROR] Loi gui text: {e}")
            return False

    def send_photo(self, image_bytes, caption=""):
        tok = str(self.bot_token).strip()
        cid = str(self.chat_id).strip()
        if not tok or not cid or tok == "YOUR_TELEGRAM_BOT_TOKEN" or cid == "YOUR_TELEGRAM_CHAT_ID" or not image_bytes:
            return False
        try:
            url = f"https://api.telegram.org/bot{tok}/sendPhoto"
            files = {"photo": ("snapshot.jpg", image_bytes, "image/jpeg")}
            data = {"chat_id": cid, "caption": caption[:1000], "parse_mode": "HTML"}
            r = requests.post(url, data=data, files=files, timeout=12)
            if r.status_code == 200:
                print(f"[TELEGRAM PHOTO] Da gui anh snapshot thanh cong toi Telegram!")
                return True
            else:
                print(f"[TELEGRAM PHOTO WARN] Gui anh that bai (HTTP {r.status_code}): {r.text}")
                return False
        except Exception as e:
            print(f"[TELEGRAM ERROR] Loi gui anh: {e}")
            return False

    def send_tunnel_url(self, public_url):
        if not public_url:
            return False
        msg = (
            "🟢 <b>HE THONG BE CA SIC DA KHOI DONG THANH CONG!</b>\n\n"
            "🌐 <b>DUONG DAN TRUY CAP WEB TU XA (CLOUDFLARE):</b>\n"
            f"🔗 <a href='{public_url}'>{public_url}</a>\n\n"
            "📱 <i>Ban co the mo link tren tu dien thoai de xem Camera AI va dieu khien be ca truc tiep!</i>"
        )
        return self.send_message(msg)

    def check_and_send_alert(self, ai_result):
        tok = str(self.bot_token).strip()
        cid = str(self.chat_id).strip()
        if not tok or not cid or tok == "YOUR_TELEGRAM_BOT_TOKEN" or cid == "YOUR_TELEGRAM_CHAT_ID":
            return
            
        dead = ai_result.get("confirmed_dead_fish", ai_result.get("dead_fish", 0))
        turb = ai_result.get("water_turbidity", 0)
        
        now = time.time()
        if now - self.last_alert_time < self.cooldown_sec:
            return
            
        self.last_alert_time = now
        
        total = self.shared_state.get("total_fish_configured", 10)
        alive = max(0, total - dead)
        
        alert_msg = f"🚨 <b>CANH BAO KHAN CAP TU BE CA SIC!</b>\n\n"
        if dead > 0:
            alert_msg += f"☠️ <b>XAC NHAN 100%: CO {dead} CA THE BI CHET!</b>\n"
            alert_msg += f"🐟 So ca con song: <b>{alive} / {total}</b> con\n"
        if turb >= 40:
            alert_msg += f"🌊 Do duc cua nuoc: <b>{turb}%</b> (Nguy co o nhiem)\n"
        alert_msg += f"🤖 Nhan xet AI: <i>{ai_result.get('summary', '')}</i>\n"
        alert_msg += f"⏱️ Thoi gian: {time.strftime('%H:%M:%S %d/%m/%Y')}"
        
        img_bytes = self.video_stream.get_jpeg_bytes() if self.video_stream else None
        if img_bytes:
            ok = self.send_photo(img_bytes, caption=alert_msg[:1000])
            if not ok:
                self.send_message(alert_msg)
        else:
            self.send_message(alert_msg)

    def _polling_loop(self):
        while self.running:
            try:
                tok = str(self.bot_token).strip()
                if not tok or tok == "YOUR_TELEGRAM_BOT_TOKEN":
                    time.sleep(3.0)
                    continue
                url = f"https://api.telegram.org/bot{tok}/getUpdates"
                params = {"offset": self.last_update_id + 1, "timeout": 20}
                r = requests.get(url, params=params, timeout=25)
                
                if r.status_code == 200:
                    data = r.json()
                    for update in data.get("result", []):
                        self.last_update_id = update["update_id"]
                        msg = update.get("message", {})
                        from_chat_id = str(msg.get("chat", {}).get("id", ""))
                        text = msg.get("text", "").strip()
                        
                        if from_chat_id == self.chat_id:
                            self._handle_command(text)
            except Exception:
                time.sleep(3.0)
            time.sleep(0.5)

    def _handle_command(self, cmd_text):
        parts = cmd_text.split()
        if not parts:
            return
        cmd = parts[0].lower()

        if cmd == "/help" or cmd == "/start":
            msg = (
                "<b>DANH SACH LENH TELEGRAM BOT BE CA SIC:</b>\n"
                "• <code>/status</code>: Chup anh va phan tich tinh trang AI ngay lap tuc\n"
                "• <code>/snapshot</code>: Chup va gui anh truc tiep tu Camera\n"
                "• <code>/url</code>: Lay lai link Web Cloudflare truy cap tu xa\n"
                "• <code>/setref 1..5</code>: Chup frame hien tai luu lam anh mau so 1-5\n"
                "• <code>/refstatus</code>: Xem danh sach 5 anh mau tham chieu da luu\n"
                "• <code>/data</code>: Doc cac thong so moi truong tu ESP32\n"
                "• <code>/feed</code>: Kich hoat quay Servo cho ca an tu xa\n"
            )
            self.send_message(msg)

        elif cmd == "/url":
            pub_url = self.shared_state.get("public_url", "")
            if pub_url:
                self.send_tunnel_url(pub_url)
            else:
                self.send_message("Cloudflare Tunnel chua duoc khoi tao hoac chua co URL.")

        elif cmd == "/setref":
            if len(parts) >= 2 and parts[1].isdigit():
                slot = int(parts[1])
                if 1 <= slot <= 5:
                    frame = self.video_stream.get_frame() if self.video_stream else None
                    if frame is not None and self.ref_manager:
                        ok = self.ref_manager.save_frame_to_slot(slot, frame)
                        if ok:
                            self.send_message(f"Da chup va luu anh mau tham chieu vao <b>Slot {slot}/5</b> thanh cong!")
                        else:
                            self.send_message(f"Loi khi luu anh mau vao Slot {slot}.")
                    else:
                        self.send_message("Khong the lay frame tu Camera.")
                else:
                    self.send_message("Vui long chon slot tu 1 den 5 (Vi du: <code>/setref 1</code>).")
            else:
                self.send_message("Cu phap: <code>/setref &lt;slot 1-5&gt;</code> (Vi du: <code>/setref 1</code>).")

        elif cmd == "/refstatus":
            if self.ref_manager:
                st = self.ref_manager.get_slot_status()
                msg = "<b>DANH SACH 5 ANH MAU THAM CHIEU (BASELINE):</b>\n"
                for s in st:
                    icon = "DA LUU" if s["exists"] else "CHUA CO"
                    mtime = f"({s['mtime']})" if s["exists"] else ""
                    msg += f"• Slot {s['slot']}: <b>{icon}</b> {mtime}\n"
                msg += "\nGo <code>/setref &lt;1-5&gt;</code> de chup luu anh mau tu camera."
                self.send_message(msg)
            else:
                self.send_message("Chua khoi tao Reference Manager.")

        elif cmd == "/snapshot":
            img_bytes = self.video_stream.get_jpeg_bytes() if self.video_stream else None
            if img_bytes:
                caption = f"📸 Anh chup truc tiep luc: {time.strftime('%H:%M:%S %d/%m/%Y')}"
                self.send_photo(img_bytes, caption=caption)
            else:
                self.send_message("Khong the lay anh tu Camera.")

        elif cmd == "/status":
            self.send_message("⏳ Dang chup anh va phan tich qua Gemini 3.5 Flash VLM, vui long doi giay lat...")
            frame = self.video_stream.get_frame() if self.video_stream else None
            ai_res = self.gemini_ai.analyze_frame(frame) if self.gemini_ai else {}
            
            self.shared_state["ai_result"] = ai_res
            dead = ai_res.get("dead_fish", 0)
            total = self.shared_state.get("total_fish_configured", 10)
            alive = max(0, total - dead)
            
            caption = (
                f"<b>KET QUA PHAN TICH AI ({ai_res.get('ai_engine', 'Gemini 3.5 Flash')})</b>\n"
                f"• Tong so ca tha: <b>{total}</b>\n"
                f"• Ca dang song khoe: <b>{alive}</b>\n"
                f"• Ca chet / bat thuong: <b>{dead}</b>\n"
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
                    self.send_message("🍱 Da gui lenh rot thuc an xuong ESP32 thanh cong!")
                else:
                    self.send_message("❌ Loi gui lenh rot thuc an xuong ESP32.")
            else:
                self.send_message("Chua khoi tao ket noi ESP32 LAN.")
