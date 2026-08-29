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
    Tối ưu hóa:
    - 100% NON-BLOCKING: Quá trình tải binary và chạy tunnel diễn ra hoàn toàn trong background thread.
      Server Flask, Camera và Web Dashboard khởi động ngay lập tức trong 0.01s!
    - Tải binary dạng Stream với thông báo tiến trình %, không bị đơ.
    - Lưu cố định vào Python_Server/bin/ (chỉ tải 1 lần duy nhất, các lần sau mở tức thì < 1s).
    - Hỗ trợ đa nền tảng: Windows x64, Raspberry Pi 5 (ARM64 / aarch64), Linux x64.
    """
    def __init__(self, port=5000, on_url_ready=None):
        self.port = port
        self.on_url_ready = on_url_ready
        self.public_url = None
        self.status_msg = "Đang khởi tạo..."
        self.process = None
        self.running = False
        self.thread = None
        
        # Thư mục lưu binary cục bộ
        self.bin_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "bin")
        os.makedirs(self.bin_dir, exist_ok=True)
        self.bin_path = None

    def start(self):
        if self.running:
            return
        self.running = True
        self.public_url = None
        self.thread = threading.Thread(target=self._run_tunnel, daemon=True)
        self.thread.start()

    def _ensure_binary(self):
        """
        Kiểm tra hoặc tải binary trong background thread mà không làm nghẽn server.
        """
        system = sys.platform.lower()
        arch = platform.machine().lower()
        
        bin_name = "cloudflared.exe" if "win" in system else "cloudflared"
        target_path = os.path.join(self.bin_dir, bin_name)

        # 1. Đã có sẵn trong bin/ -> Dùng luôn
        if os.path.exists(target_path) and os.path.getsize(target_path) > 1000000:
            self.bin_path = target_path
            return self.bin_path

        # 2. Đã có trong PATH hệ thống -> Dùng luôn
        sys_which = shutil.which("cloudflared")
        if sys_which:
            self.bin_path = sys_which
            return self.bin_path

        # 3. Tải từ GitHub Releases dạng Streaming
        if "aarch64" in arch or "arm64" in arch:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64"
        elif "arm" in arch:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm"
        elif "win" in system:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"
        else:
            download_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64"

        print(f"[CLOUDFLARE] Dang tai binary cloudflared ({download_url})...")
        try:
            r = requests.get(download_url, stream=True, timeout=60)
            if r.status_code == 200:
                total_size = int(r.headers.get('content-length', 0))
                downloaded = 0
                temp_path = target_path + ".tmp"
                
                with open(temp_path, "wb") as f:
                    for chunk in r.iter_content(chunk_size=1024 * 1024): # 1MB chunk
                        if not self.running:
                            return None
                        if chunk:
                            f.write(chunk)
                            downloaded += len(chunk)
                            if total_size > 0:
                                percent = int((downloaded / total_size) * 100)
                                print(f"[CLOUDFLARE] Tien trinh tai binary: {percent}% ({downloaded // (1024*1024)}MB / {total_size // (1024*1024)}MB)...")

                if os.path.exists(target_path):
                    try:
                        os.remove(target_path)
                    except Exception:
                        pass
                os.rename(temp_path, target_path)

                if "win" not in system:
                    os.chmod(target_path, 0o755)
                    
                print(f"[CLOUDFLARE] DA TAI XONG BINARY VA LUU TAI: {target_path}")
                self.bin_path = target_path
                return self.bin_path
        except Exception as e:
            print(f"[CLOUDFLARE WARN] Loi tai truc tiep ({e}), thu dung pycloudflared...")

        # 4. Fallback pycloudflared
        try:
            import pycloudflared.util
            exe = str(pycloudflared.util.download())
            self.bin_path = exe
            return self.bin_path
        except Exception:
            pass

        self.bin_path = "cloudflared"
        return self.bin_path

    def _run_tunnel(self):
        # 1. Chuan bi binary (chay ngam trong thread nay)
        exe = self._ensure_binary()
        if not exe or not self.running:
            return

        print(f"[CLOUDFLARE] Khoi chay tunnel cho port {self.port} voi binary: {exe}...")
        
        # 2. Chay truc tiep binary voi co --no-autoupdate
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
            while self.running and (time.time() - start_time < 35):
                line = self.process.stderr.readline()
                if not line and self.process.poll() is not None:
                    break
                match = url_pattern.search(line)
                if match:
                    self.public_url = match.group(0)
                    self._notify_ready()
                    break

            while self.running and self.process and self.process.poll() is None:
                time.sleep(1)

        except Exception as e:
            print(f"[CLOUDFLARE ERROR] Loi khoi chay tunnel: {e}")

    def _notify_ready(self):
        print("=" * 65)
        print(f"  CLOUDFLARE PUBLIC URL SAN SANG: {self.public_url}")
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
