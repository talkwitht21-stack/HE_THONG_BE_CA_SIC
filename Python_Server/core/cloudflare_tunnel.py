import subprocess
import threading
import time
import re
import os
import sys
import platform
import requests
import shutil

class CloudflareTunnel:
    """
    Hệ thống tạo đường hầm Cloudflare Quick Tunnel (miễn phí, không cần mở port router)
    Lấy cảm hứng và tối ưu từ kiến trúc chuẩn của Nam19ti/IOT_:
    - Tự động nhận diện nền tảng: Windows x64, Raspberry Pi 5 (ARM64 / aarch64), Linux x64.
    - Tự động tải binary chính thức về thư mục cục bộ Python_Server/bin/ và cache vĩnh viễn.
    - Khởi chạy cực nhanh (< 1.5s), đọc stderr bắt URL thời gian thực và tự động gửi link qua Telegram.
    - Hỗ trợ restart tunnel theo yêu cầu.
    """
    def __init__(self, port=5000, on_url_ready=None):
        self.port = port
        self.on_url_ready = on_url_ready
        self.public_url = None
        self.process = None
        self.running = False
        self.thread = None
        
        # Thư mục lưu binary cục bộ
        self.bin_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "bin")
        os.makedirs(self.bin_dir, exist_ok=True)
        
        self.bin_path = self._get_or_download_binary()

    def _get_or_download_binary(self):
        """
        Xác định hoặc tự động tải binary cloudflared phù hợp với kiến trúc máy.
        """
        system = sys.platform.lower()
        arch = platform.machine().lower()
        
        bin_name = "cloudflared.exe" if "win" in system else "cloudflared"
        target_path = os.path.join(self.bin_dir, bin_name)

        # 1. Nếu đã tải sẵn trong bin/ -> Dùng luôn
        if os.path.exists(target_path):
            return target_path

        # 2. Nếu đã có trong PATH hệ thống -> Dùng luôn
        sys_which = shutil.which("cloudflared")
        if sys_which:
            return sys_which

        # 3. Tự động tải binary chính thức từ GitHub Cloudflare Releases
        print("[CLOUDFLARE] Dang tu dong tai binary Cloudflare phu hop voi he thong...")
        if "aarch64" in arch or "arm64" in arch:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64"
        elif "arm" in arch:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm"
        elif "win" in system:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"
        else:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64"

        try:
            r = requests.get(download_url, timeout=30)
            if r.status_code == 200:
                with open(target_path, "wb") as f:
                    f.write(r.content)
                if "win" not in system:
                    os.chmod(target_path, 0o755)
                print(f"[CLOUDFLARE] Da tai thanh cong binary tai: {target_path}")
                return target_path
        except Exception as e:
            print(f"[CLOUDFLARE WARN] Khong the tai binary truc tiep ({e}). Thu dung pycloudflared...")

        # 4. Fallback qua pycloudflared nếu có
        try:
            import pycloudflared.util
            exe = str(pycloudflared.util.download())
            return exe
        except Exception:
            pass

        return "cloudflared"

    def start(self):
        if self.running:
            return
        self.running = True
        self.public_url = None
        self.thread = threading.Thread(target=self._run_tunnel, daemon=True)
        self.thread.start()

    def _run_tunnel(self):
        print(f"[CLOUDFLARE] Dang mo Cloudflare Quick Tunnel cho port {self.port}...")
        
        # 1. Thử qua pycloudflared trước nếu có
        try:
            from pycloudflared import try_cloudflare
            tunnel = try_cloudflare(port=self.port, verbose=False)
            if tunnel and hasattr(tunnel, "tunnel_url") and tunnel.tunnel_url:
                self.public_url = tunnel.tunnel_url
                self._notify_ready()
                return
            elif tunnel and hasattr(tunnel, "tunnel") and tunnel.tunnel:
                self.public_url = tunnel.tunnel
                self._notify_ready()
                return
        except Exception:
            pass

        # 2. Chạy trực tiếp binary với cờ --no-autoupdate
        exe = self.bin_path or "cloudflared"
        cmd = [exe, "tunnel", "--no-autoupdate", "--url", f"http://127.0.0.1:{self.port}"]

        try:
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
                universal_newlines=True
            )

            url_pattern = re.compile(r'https://[a-zA-Z0-9-]+\.trycloudflare\.com')

            start_time = time.time()
            # Đọc stderr trong 30 giây để bắt link trycloudflare
            while self.running and (time.time() - start_time < 30):
                line = self.process.stderr.readline()
                if not line and self.process.poll() is not None:
                    break
                match = url_pattern.search(line)
                if match:
                    self.public_url = match.group(0)
                    self._notify_ready()
                    break

            # Duy trì đọc nền
            while self.running and self.process and self.process.poll() is None:
                time.sleep(1)

        except Exception as e:
            print(f"[CLOUDFLARE ERROR] Loi khoi chay tunnel: {e}")

    def _notify_ready(self):
        print("=" * 65)
        print(f"  CLOUDFLARE PUBLIC URL: {self.public_url}")
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
