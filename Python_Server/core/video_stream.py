import cv2
import time
import threading
import numpy as np
import urllib.request
from urllib.parse import urlparse

class VideoStream:
    """
    Quản lý luồng Camera non-blocking chạy trên background thread.
    Tích hợp kiến trúc tối ưu từ Nam19ti/IOT_ (Zero-Crash & Dual Engine):
    - Tự động chuẩn hóa URL (http, https, rtsp, /shot.jpg).
    - Chế độ 1 (HTTP Fast Snapshot): Đọc ảnh tĩnh trực tiếp qua HTTP GET (siêu nhẹ, < 30ms, không chiếm RAM buffer).
    - Chế độ 2 (RTSP / VideoCapture): Đọc luồng video RTSP/MJPEG/USB Webcam với buffer size = 1 chống trễ hình.
    - Tự động chuyển đổi mượt mà và tự phục hồi kết nối.
    """
    def __init__(self, source=0, width=640, height=480, fps=15):
        self.raw_source = source
        self.source = self._normalize_source(source)
        self.width = width
        self.height = height
        self.fps = fps
        
        self.cap = None
        self.frame = None
        self.running = False
        self.lock = threading.Lock()
        self.thread = None
        self.is_http_snapshot = False
        
        self.actual_fps = 0.0
        self.last_frame_time = time.time()
        self.frame_count = 0
        
        self._init_camera()

    def _normalize_source(self, src):
        """
        Tự động chuẩn hóa nguồn Camera (URL hoặc index USB Webcam).
        Học hỏi từ CameraClient của Nam19ti/IOT_.
        """
        if src is None:
            return 0
        src_str = str(src).strip()
        if src_str.isdigit():
            return int(src_str)
        
        # Nếu là URL
        if not (src_str.startswith("http://") or src_str.startswith("https://") or src_str.startswith("rtsp://")):
            src_str = "http://" + src_str

        parsed = urlparse(src_str)
        # Nếu người dùng chỉ gõ IP:Port hoặc kết thúc bằng / (VD: 192.168.1.50:8080) -> Tự thêm /shot.jpg cho IP Webcam
        if src_str.startswith("http") and (not parsed.path or parsed.path == "/"):
            src_str = src_str.rstrip("/") + "/shot.jpg"

        return src_str

    def update_source(self, new_source):
        """
        Đổi nguồn Camera/IP Camera (RTSP/HTTP/USB) trong runtime mà không cần tắt server.
        """
        with self.lock:
            self.raw_source = new_source
            self.source = self._normalize_source(new_source)
            if self.cap:
                try:
                    self.cap.release()
                except Exception:
                    pass
                self.cap = None
            self._init_camera()
            print(f"[CAMERA] Da doi va chuan hoa nguon Camera sang: {self.source}")
            return True

    def _init_camera(self):
        """
        Khởi tạo chế độ đọc phù hợp (HTTP Snapshot hoặc VideoCapture).
        """
        if isinstance(self.source, str) and (self.source.endswith("/shot.jpg") or self.source.endswith("/capture") or "/photo" in self.source):
            self.is_http_snapshot = True
            print(f"[CAMERA ENGINE] Kich hoat HTTP Fast Snapshot cho: {self.source}")
            return

        self.is_http_snapshot = False
        try:
            self.cap = cv2.VideoCapture(self.source)
            if self.cap.isOpened():
                self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1) # Giảm buffer xuống 1 để triệt tiêu độ trễ
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
                self.cap.set(cv2.CAP_PROP_FPS, self.fps)
                print(f"[CAMERA ENGINE] Khoi tao thanh cong VideoCapture cho: {self.source}")
            else:
                print(f"[CAMERA WARN] Chua the mo VideoCapture: {self.source}. Se thu che do HTTP hoac gia lap.")
        except Exception as e:
            print(f"[CAMERA ERROR] Loi mo VideoCapture: {e}")

    def _fetch_http_frame(self):
        """
        Lấy frame trực tiếp qua HTTP GET từ IP Webcam (Nam19ti/IOT_ method).
        """
        try:
            req = urllib.request.urlopen(self.source, timeout=1.8)
            arr = np.asarray(bytearray(req.read()), dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
            if img is not None and img.size > 0:
                if img.shape[1] != self.width or img.shape[0] != self.height:
                    img = cv2.resize(img, (self.width, self.height))
                return img
        except Exception:
            pass
        return None

    def start(self):
        if self.running:
            return
        self.running = True
        self.thread = threading.Thread(target=self._update_loop, daemon=True)
        self.thread.start()
        print("[CAMERA] Background thread doc frame da khoi chay.")

    def _create_placeholder_frame(self):
        # Tạo frame giả lập đẹp mắt phong cách Xanh Nước Biển nếu camera chưa cắm
        img = np.zeros((self.height, self.width, 3), dtype=np.uint8)
        img[:] = (245, 230, 215) # Light water blue background (BGR)
        
        cv2.rectangle(img, (20, 20), (self.width - 20, self.height - 20), (199, 132, 2), 2)
        cv2.putText(img, "HE THONG BE CA SIC - AI CAMERA", (40, 60), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (161, 105, 3), 2)
        cv2.putText(img, f"NGUON: {self.source}", (40, 100), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (100, 100, 100), 1)
        
        # Vẽ gợn sóng nước demo
        t = time.time()
        for x in range(30, self.width - 30, 10):
            y = int(self.height / 2 + 15 * np.sin(x * 0.03 + t * 2))
            cv2.circle(img, (x, y), 2, (233, 165, 14), -1)
            
        time_str = time.strftime("%Y-%m-%d %H:%M:%S")
        cv2.putText(img, f"Time: {time_str}", (40, self.height - 40), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (120, 120, 120), 1)
        return img

    def _update_loop(self):
        while self.running:
            frame = None

            # 1. Thử HTTP Fast Snapshot nếu là luồng HTTP
            if self.is_http_snapshot or (isinstance(self.source, str) and self.source.startswith("http")):
                frame = self._fetch_http_frame()

            # 2. Thử VideoCapture nếu chưa có frame
            if frame is None and self.cap and self.cap.isOpened():
                ret, raw_frame = self.cap.read()
                if ret and raw_frame is not None:
                    if raw_frame.shape[1] != self.width or raw_frame.shape[0] != self.height:
                        frame = cv2.resize(raw_frame, (self.width, self.height))
                    else:
                        frame = raw_frame
                else:
                    time.sleep(0.5)
                    self._init_camera()

            # 3. Tạo placeholder nếu mất tín hiệu
            if frame is None:
                frame = self._create_placeholder_frame()
                time.sleep(0.05)
                
            # Tính toán FPS
            now = time.time()
            self.frame_count += 1
            if now - self.last_frame_time >= 1.0:
                self.actual_fps = round(self.frame_count / (now - self.last_frame_time), 1)
                self.frame_count = 0
                self.last_frame_time = now

            with self.lock:
                self.frame = frame

            time.sleep(1.0 / self.fps)

    def get_frame(self):
        with self.lock:
            if self.frame is not None:
                return self.frame.copy()
        return None

    def get_jpeg_bytes(self, overlay_info=None):
        frame = self.get_frame()
        if frame is None:
            frame = self._create_placeholder_frame()

        if overlay_info:
            text = f"Dead: {overlay_info.get('dead_fish', 0)} | Turbidity: {overlay_info.get('water_turbidity', 0)}%"
            cv2.putText(frame, text, (15, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        ret, jpeg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
        if ret:
            return jpeg.tobytes()
        return None

    def stop(self):
        self.running = False
        if self.cap:
            self.cap.release()
