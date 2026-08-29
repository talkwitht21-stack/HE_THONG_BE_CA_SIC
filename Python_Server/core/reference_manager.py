import os
import cv2
import time
import base64

class ReferenceManager:
    """
    Quản lý 5 ảnh mẫu tham chiếu bể cá ở trạng thái bình thường (Baseline Calibration).
    Giúp Gemini Multimodal AI đối chiếu để loại bỏ 100% việc nhận diện nhầm vật thể tĩnh (lũa, đá, hang hốc) thành cá chết.
    """
    def __init__(self, ref_dir="reference_images", max_slots=5):
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        if not os.path.isabs(ref_dir):
            self.ref_dir = os.path.join(base_dir, ref_dir)
        else:
            self.ref_dir = ref_dir
            
        self.max_slots = max_slots
        os.makedirs(self.ref_dir, exist_ok=True)

    def get_slot_path(self, slot_id):
        slot_id = int(slot_id)
        if 1 <= slot_id <= self.max_slots:
            return os.path.join(self.ref_dir, f"ref_{slot_id}.jpg")
        return None

    def save_frame_to_slot(self, slot_id, frame):
        """
        Lưu một frame OpenCV vào slot_id (1..5)
        """
        path = self.get_slot_path(slot_id)
        if path and frame is not None:
            try:
                # Resize chuẩn hóa 640x480 và nén JPEG chất lượng 85
                h, w = frame.shape[:2]
                if w > 800:
                    frame = cv2.resize(frame, (640, int(h * 640 / w)))
                cv2.imwrite(path, frame, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
                print(f"[REF MANAGER] Da luu anh mau vao Slot {slot_id}: {path}")
                return True
            except Exception as e:
                print(f"[REF MANAGER ERROR] Loi luu anh slot {slot_id}: {e}")
        return False

    def save_bytes_to_slot(self, slot_id, image_bytes):
        """
        Lưu file ảnh tải lên từ Web dạng bytes vào slot_id (1..5)
        """
        path = self.get_slot_path(slot_id)
        if path and image_bytes:
            try:
                with open(path, "wb") as f:
                    f.write(image_bytes)
                print(f"[REF MANAGER] Da tai len va luu anh mau vao Slot {slot_id}: {path}")
                return True
            except Exception as e:
                print(f"[REF MANAGER ERROR] Loi luu bytes slot {slot_id}: {e}")
        return False

    def delete_slot(self, slot_id):
        path = self.get_slot_path(slot_id)
        if path and os.path.exists(path):
            try:
                os.remove(path)
                print(f"[REF MANAGER] Da xoa anh mau Slot {slot_id}")
                return True
            except Exception as e:
                print(f"[REF MANAGER ERROR] Loi xoa slot {slot_id}: {e}")
        return False

    def get_slot_status(self):
        """
        Trả về trạng thái của cả 5 slot: [{slot: 1, exists: True, mtime: '19:30 29/08'}, ...]
        """
        status_list = []
        for i in range(1, self.max_slots + 1):
            path = self.get_slot_path(i)
            exists = os.path.exists(path) if path else False
            mtime_str = ""
            if exists:
                mtime = os.path.getmtime(path)
                mtime_str = time.strftime("%H:%M:%S %d/%m/%Y", time.localtime(mtime))
            status_list.append({
                "slot": i,
                "exists": exists,
                "mtime": mtime_str
            })
        return status_list

    def get_all_reference_images_for_gemini(self):
        """
        Nạp tất cả các ảnh mẫu hiện có và đóng gói định dạng phù hợp cho Gemini Vision API.
        """
        gemini_parts = []
        for i in range(1, self.max_slots + 1):
            path = self.get_slot_path(i)
            if path and os.path.exists(path):
                try:
                    with open(path, "rb") as f:
                        data = f.read()
                    gemini_parts.append({
                        "mime_type": "image/jpeg",
                        "data": data
                    })
                except Exception as e:
                    print(f"[REF MANAGER WARN] Khong the doc ref_{i}.jpg: {e}")
        return gemini_parts
