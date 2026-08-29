import cv2
import json
import time
import re

try:
    import google.generativeai as genai
    HAS_GENAI = True
except ImportError:
    genai = None
    HAS_GENAI = False

PROMPT_ANALYZE_FISH = """
Bạn là Chuyên gia Trí tuệ Nhân tạo Giám sát Thủy sinh và Bể cá Thông minh.
Dưới đây là:
1. Các bức ảnh mẫu chụp bể cá này khi ở trạng thái HOÀN TOÀN BÌNH THƯỜNG và KHÔNG CÓ CÁ CHẾT (các vật thể tĩnh dưới đáy như gỗ lũa, đá sỏi, hang hốc, rêu thủy sinh là bối cảnh tự nhiên).
2. Bức ảnh cuối cùng là ẢNH HIỆN TẠI CỦA BỂ CÁ CẦN PHÂN TÍCH.

NHIỆM VỤ CỦA BẠN:
Hãy so sánh ảnh hiện tại với các ảnh mẫu bình thường để LOẠI TRỪ 100% các vật thể tĩnh trong bể, và chỉ đánh giá cá thể thực sự.
Trả về ĐÚNG ĐỊNH DẠNG JSON sau (KHÔNG thêm bất kỳ giải thích nào ngoài JSON):

{
  "dead_fish": <số cá thể thực sự bị tử vong, lật ngửa bụng, nổi bất động trên mặt nước hoặc nằm im dưới đáy bể (không phải gỗ/đá tĩnh), số nguyên>,
  "abnormal_fish": <số cá thể bơi lờ đờ, tróc vảy, nấm trắng, bơi nghiêng bất thường, số nguyên>,
  "water_turbidity": <ước tính độ vẩn đục của nước từ 0 (trong vắt) đến 100 (rất đục/ô nhiễm), số nguyên>,
  "is_alert": <true nếu dead_fish > 0 hoặc water_turbidity >= 40, ngược lại false>,
  "summary": "<Nhận xét tóm tắt ngắn gọn tình trạng sức khỏe của cá và chất lượng nước bể cá trong 1-2 câu tiếng Việt>"
}
"""

class GeminiVisionAnalyzer:
    """
    Module phân tích hình ảnh chuyên sâu bằng Gemini Multimodal Vision API.
    Cài đặt tĩnh model gemini-3.5-flash với cơ chế fallback tự động đa tầng:
    gemini-3.5-flash -> gemini-2.5-flash -> gemini-2.0-flash -> gemini-1.5-flash
    """
    FALLBACK_MODELS = [
        "gemini-3.5-flash",
        "gemini-2.5-flash",
        "gemini-2.0-flash",
        "gemini-1.5-flash"
    ]

    def __init__(self, api_key, model_name="gemini-3.5-flash", temperature=0.2, ref_manager=None):
        self.api_key = api_key
        self.model_name = "gemini-3.5-flash" # Cài đặt tĩnh theo yêu cầu
        self.temperature = temperature
        self.ref_manager = ref_manager
        self.enabled = bool(api_key and api_key != "YOUR_GEMINI_API_KEY")
        
        self.model = None
        self.active_model_name = self.model_name
        if self.enabled:
            if not HAS_GENAI:
                print("[GEMINI WARN] Chua cai dat thu vien 'google-generativeai'. Chay lenh: pip install google-generativeai")
                self.enabled = False
                return
            self._init_model()
        else:
            print("[GEMINI WARN] Chua nhap Gemini API Key! He thong se su dung che do Computer Vision du phong.")

    def _init_model(self):
        try:
            genai.configure(api_key=self.api_key)
            self.model = genai.GenerativeModel(
                model_name=self.model_name,
                generation_config={"temperature": self.temperature, "response_mime_type": "application/json"}
            )
            self.active_model_name = self.model_name
            print(f"[GEMINI AI] Khoi tao thanh cong voi model tinh: {self.model_name}")
        except Exception as e:
            print(f"[GEMINI WARN] Model {self.model_name} chua san sang, dang thu fallback: {e}")
            self._try_fallback_models()

    def _try_fallback_models(self):
        for fb_model in self.FALLBACK_MODELS:
            try:
                self.model = genai.GenerativeModel(
                    model_name=fb_model,
                    generation_config={"temperature": self.temperature, "response_mime_type": "application/json"}
                )
                self.active_model_name = fb_model
                print(f"[GEMINI AI] Fallback thanh cong sang model: {fb_model}")
                return True
            except Exception:
                pass
        return False

    def update_api_key(self, new_api_key):
        """
        Cập nhật API Key mới trực tiếp từ Web Dashboard và khởi tạo lại Model.
        """
        self.api_key = str(new_api_key).strip()
        self.enabled = bool(self.api_key and self.api_key != "YOUR_GEMINI_API_KEY")
        if self.enabled:
            if not HAS_GENAI:
                print("[GEMINI WARN] Chua cai dat thu vien 'google-generativeai'.")
                self.enabled = False
                return False
            self._init_model()
            return True
        return False

    def analyze_frame(self, frame, total_fish=10):
        """
        Phân tích hình ảnh hiện tại kết hợp đối chiếu với các ảnh mẫu tham chiếu.
        """
        if not self.enabled or self.model is None or frame is None:
            return self._fallback_cv_analysis(frame, total_fish)

        try:
            # 1. Mã hóa ảnh hiện tại sang JPEG
            ret, buf = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
            if not ret:
                return self._fallback_cv_analysis(frame, total_fish)
            
            cur_img_part = {
                "mime_type": "image/jpeg",
                "data": buf.tobytes()
            }

            # 2. Thu thập các ảnh mẫu tham chiếu chuẩn bình thường (nếu có)
            contents = [PROMPT_ANALYZE_FISH]
            if self.ref_manager and self.ref_manager.has_references():
                ref_parts = self.ref_manager.get_reference_image_parts()
                if ref_parts:
                    contents.append("--- CAC ANH MAU BE CA KHI BINH THUONG (DE LOAI TRU VAT THE TINH) ---")
                    contents.extend(ref_parts)
                    contents.append("--- ANH HIEN TAI CUA BE CA CAN PHAN TICH DUOI DAY ---")

            contents.append(cur_img_part)

            # 3. Gửi request phân tích tới Gemini Flash Model
            response = None
            try:
                response = self.model.generate_content(contents)
            except Exception as req_err:
                # Nếu model tĩnh gặp lỗi endpoint -> Thử qua chuỗi fallback
                print(f"[GEMINI WARN] Loi goi {self.active_model_name}: {req_err}. Dang thu fallback...")
                if self._try_fallback_models():
                    response = self.model.generate_content(contents)
                else:
                    raise req_err

            if not response or not response.text:
                return self._fallback_cv_analysis(frame, total_fish)

            # 4. Trích xuất JSON an toàn
            text = response.text.strip()
            # Bóc tách nếu có markdown codeblock
            match = re.search(r'\{.*\}', text, re.DOTALL)
            if match:
                clean_json = match.group(0)
            else:
                clean_json = text

            res = json.loads(clean_json)
            
            dead = int(res.get("dead_fish", 0))
            abnormal = int(res.get("abnormal_fish", 0))
            turbidity = int(res.get("water_turbidity", 15))
            summary = res.get("summary", "He thong Gemini AI hoat dong binh thuong.")
            is_alert = bool(res.get("is_alert", dead > 0 or turbidity >= 40))

            return {
                "dead_fish": dead,
                "abnormal_fish": abnormal,
                "water_turbidity": turbidity,
                "is_alert": is_alert,
                "summary": summary,
                "ai_engine": f"Gemini 3.5 Flash ({self.active_model_name})",
                "timestamp": time.time()
            }

        except Exception as e:
            print(f"[GEMINI ERROR] Loi phan tich anh: {e}")
            return self._fallback_cv_analysis(frame, total_fish)

    def _fallback_cv_analysis(self, frame, total_fish=10):
        """
        Thuật toán Computer Vision dự phòng cơ bản bằng OpenCV
        """
        if frame is None:
            return {
                "dead_fish": 0,
                "abnormal_fish": 0,
                "water_turbidity": 20,
                "is_alert": False,
                "summary": "Khong co tin hieu camera.",
                "ai_engine": "Computer Vision Offline",
                "timestamp": time.time()
            }

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        laplacian_var = cv2.Laplacian(gray, cv2.CV_64F).var()
        turbidity = max(5, min(95, int(100 - (laplacian_var / 5.0))))

        return {
            "dead_fish": 0,
            "abnormal_fish": 0,
            "water_turbidity": turbidity,
            "is_alert": turbidity >= 60,
            "summary": f"Che do CV du phong: Do duc nuoc ~{turbidity}%.",
            "ai_engine": "Computer Vision Offline",
            "timestamp": time.time()
        }
