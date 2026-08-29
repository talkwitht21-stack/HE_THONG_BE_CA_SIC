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
    Tích hợp đối chiếu 5 ảnh mẫu tham chiếu bình thường (Few-Shot Baseline Calibration).
    """
    def __init__(self, api_key, model_name="gemini-2.0-flash", temperature=0.2, ref_manager=None):
        self.api_key = api_key
        self.model_name = model_name
        self.temperature = temperature
        self.ref_manager = ref_manager
        self.enabled = bool(api_key and api_key != "YOUR_GEMINI_API_KEY")
        
        self.model = None
        if self.enabled:
            if not HAS_GENAI:
                print("[GEMINI WARN] Chua cai dat thu vien 'google-generativeai'. Chay lenh: pip install google-generativeai")
                self.enabled = False
                return
            try:
                genai.configure(api_key=self.api_key)
                self.model = genai.GenerativeModel(
                    model_name=self.model_name,
                    generation_config={"temperature": self.temperature, "response_mime_type": "application/json"}
                )
                print(f"[GEMINI AI] Khoi tao thanh cong voi model: {self.model_name}")
            except Exception as e:
                print(f"[GEMINI ERROR] Khoi tao model that bai: {e}")
                self.enabled = False
        else:
            print("[GEMINI WARN] Chua nhap Gemini API Key! He thong se su dung che do Computer Vision du phong.")

    def analyze_frame(self, frame):
        """
        Gửi frame ảnh hiện tại cùng các ảnh mẫu tham chiếu sang Gemini Vision API.
        """
        if not self.enabled or self.model is None or frame is None:
            return self._fallback_cv_analysis(frame)

        try:
            h, w = frame.shape[:2]
            if w > 800:
                frame = cv2.resize(frame, (640, int(h * 640 / w)))
                
            ret, buf = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
            if not ret:
                return self._fallback_cv_analysis(frame)
                
            current_frame_part = {"mime_type": "image/jpeg", "data": buf.tobytes()}
            
            # Nap cac anh mau tham chieu
            contents = [PROMPT_ANALYZE_FISH]
            ref_parts = self.ref_manager.get_all_reference_images_for_gemini() if self.ref_manager else []
            if ref_parts:
                contents.extend(ref_parts)
            contents.append(current_frame_part)
            
            response = self.model.generate_content(contents)
            text = response.text.strip()
            
            match = re.search(r'\{.*\}', text, re.DOTALL)
            if match:
                data = json.loads(match.group(0))
                return {
                    "dead_fish": int(data.get("dead_fish", 0)),
                    "abnormal_fish": int(data.get("abnormal_fish", 0)),
                    "water_turbidity": int(data.get("water_turbidity", 10)),
                    "is_alert": bool(data.get("is_alert", False)),
                    "summary": str(data.get("summary", "Bể cá bình thường.")),
                    "ai_engine": self.model_name,
                    "ref_count_used": len(ref_parts),
                    "analyzed_at": time.strftime("%H:%M:%S %d/%m/%Y")
                }
            else:
                return json.loads(text)
                
        except Exception as e:
            print(f"[GEMINI INFERENCE ERROR] Loi phan tich Gemini: {e}")
            return self._fallback_cv_analysis(frame)

    def _fallback_cv_analysis(self, frame):
        """
        Thuật toán xử lý ảnh OpenCV dự phòng khi chưa có API Key hoặc mất Internet.
        """
        if frame is None:
            return {
                "dead_fish": 0, "abnormal_fish": 0,
                "water_turbidity": 10, "is_alert": False,
                "summary": "Khong co tin hieu camera.", "ai_engine": "Fallback-CV",
                "ref_count_used": 0,
                "analyzed_at": time.strftime("%H:%M:%S %d/%m/%Y")
            }
            
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        laplacian_var = cv2.Laplacian(gray, cv2.CV_64F).var()
        turbidity = int(max(5, min(95, 100 - (laplacian_var / 5.0))))
        
        return {
            "dead_fish": 0,
            "abnormal_fish": 0,
            "water_turbidity": turbidity,
            "is_alert": (turbidity >= 40),
            "summary": f"Che do Du phong: Do duc nuoc xap xi {turbidity}%.",
            "ai_engine": "Fallback-CV",
            "ref_count_used": 0,
            "analyzed_at": time.strftime("%H:%M:%S %d/%m/%Y")
        }
