import subprocess
import threading
import time
import re
import os
import shutil

class CloudflareTunnel:
    """
    Tự động tạo đường hầm Cloudflare Quick Tunnel (miễn phí, không cần mở port router)
    và trích xuất Public URL (https://*.trycloudflare.com) siêu nhanh (< 2 giây).
    Hỗ trợ auto-download binary, --no-autoupdate và restart tunnel theo yêu cầu.
    """
    def __init__(self, port=5000, on_url_ready=None):
        self.port = port
        self.on_url_ready = on_url_ready
        self.public_url = None
        self.process = None
        self.running = False
        self.thread = None
        self.exe_path = None
        self._ensure_binary()

    def _ensure_binary(self):
        # 1. Kiem tra binary trong PATH
        if shutil.which("cloudflared"):
            self.exe_path = "cloudflared"
            return

        # 2. Kiem tra qua pycloudflared
        try:
            import pycloudflared.util
            # Tai ve san neu chua co
            self.exe_path = str(pycloudflared.util.download())
            print(f"[CLOUDFLARE] Su dung binary cloudflared tai: {self.exe_path}")
        except Exception as e:
            print(f"[CLOUDFLARE WARN] Khong the tai binary qua pycloudflared ({e}), se thu fallback lenh he thong.")
            self.exe_path = "cloudflared"

    def start(self):
        if self.running:
            return
        self.running = True
        self.public_url = None
        self.thread = threading.Thread(target=self._run_tunnel, daemon=True)
        self.thread.start()

    def _run_tunnel(self):
        print(f"[CLOUDFLARE] Dang khoi tao Cloudflare Tunnel cho port {self.port}...")
        
        exe = self.exe_path or "cloudflared"
        cmd = [exe, "tunnel", "--no-autoupdate", "--url", f"http://127.0.0.1:{self.port}"]

        try:
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True
            )

            url_pattern = re.compile(r'https://[a-zA-Z0-9-]+\.trycloudflare\.com')

            for line in iter(self.process.stdout.readline, ''):
                if not self.running:
                    break
                match = url_pattern.search(line)
                if match and not self.public_url:
                    self.public_url = match.group(0)
                    self._notify_ready()

        except Exception as e:
            print(f"[CLOUDFLARE ERROR] Khong the chay Cloudflare Tunnel: {e}")

    def _notify_ready(self):
        print("=" * 65)
        print(f" [CLOUDFLARE] PUBLIC WEB URL: {self.public_url}")
        print("=" * 65)
        if self.on_url_ready and self.public_url:
            try:
                self.on_url_ready(self.public_url)
            except Exception as e:
                print(f"[CLOUDFLARE CALLBACK ERROR] {e}")

    def restart(self):
        print("[CLOUDFLARE] Dang khoi dong lai duong ham Cloudflare...")
        self.stop()
        time.sleep(0.5)
        self.start()
        return True

    def get_url(self):
        return self.public_url

    def stop(self):
        self.running = False
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=2.0)
            except Exception:
                pass
            self.process = None
        print("[CLOUDFLARE] Da dung Cloudflare Tunnel.")
