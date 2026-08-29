import requests
import json
import time

class ESP32LANClient:
    """
    Cầu nối gọi trực tiếp REST API của ESP32 qua mạng nội bộ LAN (http://beca.local).
    Tốc độ phản hồi cực nhanh (< 5ms), không phụ thuộc vào Internet.
    """
    def __init__(self, base_url="http://beca.local", timeout=2.0):
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.last_cached_data = {}
        self.last_fetch_time = 0

    def get_data(self):
        """
        Lấy toàn bộ dữ liệu cảm biến và trạng thái relay từ ESP32 qua GET /api/data
        """
        url = f"{self.base_url}/api/data"
        try:
            r = requests.get(url, timeout=self.timeout)
            if r.status_code == 200:
                data = r.json()
                self.last_cached_data = data
                self.last_fetch_time = time.time()
                return data
        except Exception:
            pass
        return self.last_cached_data

    def control_device(self, dev_name):
        """
        Gửi lệnh điều khiển bật/tắt thiết bị qua POST /api/ctrl
        dev_name: heater, fan, pump, oxy, drain, filter, filter_cycle, feed, system
        """
        url = f"{self.base_url}/api/ctrl"
        try:
            payload = {"d": dev_name}
            r = requests.post(url, json=payload, timeout=self.timeout)
            return r.status_code == 200
        except Exception as e:
            print(f"[ESP32 LAN ERROR] Loi goi POST /api/ctrl cho {dev_name}: {e}")
            return False

    def start_timer(self, dev_name, seconds):
        """
        Gửi lệnh hẹn giờ qua POST /api/timer
        """
        url = f"{self.base_url}/api/timer"
        try:
            payload = {"d": dev_name, "sec": seconds}
            r = requests.post(url, json=payload, timeout=self.timeout)
            return r.status_code == 200
        except Exception as e:
            print(f"[ESP32 LAN ERROR] Loi goi POST /api/timer: {e}")
            return False
