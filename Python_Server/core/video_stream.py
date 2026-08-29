import cv2
import time
import threading
import numpy as np

class VideoStream:
    """
    Quản lý luồng Camera non-blocking chạy trên background thread.
    Tương thích cả Webcam USB (0, 1) lẫn IP Camera (RTSP / HTTP Stream).
    """
    def __init__(self, source=0, width=640, height=480, fps=15):
        self.source = source
        self.width = width
        self.height = height
        self.fps = fps
        
        self.cap = None
        self.frame = None
        self.running = False
        self.lock = threading.Lock()
        self.thread = None
        
        self.actual_fps = 0.0
        self.last_frame_time = time.time()
        self.frame_count = 0
        
        self._init_camera()

    def _init_camera(self):
        try:
            src = int(self.source) if str(self.source).isdigit() else str(self.source)
            self.cap = cv2.VideoCapture(src)
            if self.cap.isOpened():
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
                self.cap.set(cv2.CAP_PROP_FPS, self.fps)
                print(f"[CAMERA] Khoi tao thanh cong nguon: {self.source}")
            else:
                print(f"[CAMERA WARN] Chua the mo camera: {self.source}. Se tao frame gia lap.")
        except Exception as e:
            print(f"[CAMERA ERROR] Loi mo camera: {e}")

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
        cv2.putText(img, "DANG KET NOI NGUON CAMERA...", (40, 100), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (100, 100, 100), 1)
        
        # Vẽ một vài gợn sóng nước demo
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
            if self.cap and self.cap.isOpened():
                ret, raw_frame = self.cap.read()
                if ret and raw_frame is not None:
                    if raw_frame.shape[1] != self.width or raw_frame.shape[0] != self.height:
                        frame = cv2.resize(raw_frame, (self.width, self.height))
                    else:
                        frame = raw_frame
                else:
                    # Mat ket noi -> thu reconnect dinh ky moi 5s
                    time.sleep(1.0)
                    self._init_camera()
            
            if frame is None:
                frame = self._create_placeholder_frame()
                time.sleep(0.05)
                
            # Tinh toan FPS
            now = time.time()
            self.frame_count += 1
            if now - self.last_frame_time >= 1.0:
                self.actual_fps = round(self.frame_count / (now - self.last_frame_time), 1)
                self.frame_count = 0
                self.last_frame_time = now

            with self.lock:
                self.frame = frame

            time.sleep(1.0 / self.fps if self.fps > 0 else 0.03)

    def get_frame(self):
        with self.lock:
            if self.frame is not None:
                return self.frame.copy()
            return self._create_placeholder_frame()

    def get_jpeg_bytes(self, overlay_info=None):
        frame = self.get_frame()
        
        # Ve overlay thong so AI neu co
        if overlay_info:
            fps_text = f"FPS: {self.actual_fps}"
            cv2.putText(frame, fps_text, (15, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 200, 0), 2)
            
            fish_text = f"Ca: {overlay_info.get('total_fish', 0)} | Chet: {overlay_info.get('dead_fish', 0)}"
            cv2.putText(frame, fish_text, (15, 55), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 200, 255), 2)
            
            turb_text = f"Do duc: {overlay_info.get('water_turbidity', 0)}%"
            cv2.putText(frame, turb_text, (15, 85), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 150, 0), 2)

        ret, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
        if ret:
            return jpeg.tobytes()
        return None

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=2.0)
        if self.cap:
            self.cap.release()
        print("[CAMERA] Da dung luong camera.")
