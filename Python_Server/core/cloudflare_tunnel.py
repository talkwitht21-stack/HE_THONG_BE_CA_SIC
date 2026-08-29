import subprocess
import threading
import time
import re
import os
import shutil

class CloudflareTunnel:
    """
    Tự động tạo đường hầm Cloudflare Quick Tunnel (miễn phí, không cần mở port router)
    và trích xuất Public URL (https://*.trycloudflare.com).
    """
    def __init__(self, port=5000, on_url_ready=None):
        self.port = port
        self.on_url_ready = on_url_ready
        self.public_url = None
        self.process = None
        self.running = False
        self.thread = None

    def start(self):
        if self.running:
            return
        self.running = True
        self.thread = threading.Thread(target=self._run_tunnel, daemon=True)
        self.thread.start()

    def _run_tunnel(self):
        print(f"[CLOUDFLARE] Dang khoi tao Cloudflare Tunnel cho port {self.port}...")
        
        # Kiem tra thu vien pycloudflared hoac binary cloudflared
        cmd = None
        if shutil.which("cloudflared"):
            cmd = ["cloudflared", "tunnel", "--url", f"http://127.0.0.1:{self.port}"]
        else:
            try:
                import pycloudflared
                # pycloudflared ho tro tu tai binary
                cmd = ["pycloudflared", "tunnel", "--url", f"http://127.0.0.1:{self.port}"]
            except ImportError:
                # Fallback: Thu chay qua python -m pycloudflared
                cmd = [os.sys.executable, "-m", "pycloudflared", "tunnel", "--url", f"http://127.0.0.1:{self.port}"]

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
                # Tim link trycloudflare
                match = url_pattern.search(line)
                if match and not self.public_url:
                    self.public_url = match.group(0)
                    print("=" * 65)
                    print(f" CLOUDFLARE PUBLIC URL: {self.public_url}")
                    print("=" * 65)
                    
                    if self.on_url_ready:
                        try:
                            self.on_url_ready(self.public_url)
                        except Exception as e:
                            print(f"[CLOUDFLARE CALLBACK ERROR] {e}")

        except Exception as e:
            print(f"[CLOUDFLARE ERROR] Khong the chay Cloudflare Tunnel: {e}")
            print("[CLOUDFLARE TIP] Cai dat bang cach chay: pip install pycloudflared")

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
        print("[CLOUDFLARE] Da dung Cloudflare Tunnel.")
