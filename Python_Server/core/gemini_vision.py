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
Hãy phân tích bức ảnh chụp bể cá này và trả về ĐÚNG ĐỊNH DẠNG JSON sau (KHÔNG thêm bất kỳ giải thích nào ngoài JSON):

{
  "total_fish": <số lượng cá thể nhận diện được, số nguyên>,
  "dead_fish": <số cá thể bị tử vong, lật ngửa, nổi bất động trên mặt nước hoặc nằm im dưới đáy, số nguyên>,
  "abnormal_fish": <số cá thể bơi lờ đờ, tróc vảy, nấm trắng, bơi nghiêng bất thường, số nguyên>,
  "water_turbidity": <ước tính độ vẩn đục của nước từ 0 (trong vắt) đến 100 (rất đục/ô nhiễm), số nguyên>,
  "is_alert": <true nếu dead_fish > 0 hoặc water_turbidity >= 40, ngược lại false>,
  "summary": "<Nhận xét tóm tắt ngắn gọn tình trạng sức khỏe của cá và chất lượng nước bể cá trong 1-2 câu tiếng Việt>"
}
"""

class GeminiVisionAnalyzer:
    """
    Module phân tích hình ảnh chuyên sâu bằng Gemini Multimodal Vision API.
    """
    def __init__(self, api_key, model_name="gemini-2.0-flash", temperature=0.2):
        self.api_key = api_key
        self.model_name = model_name
        self.temperature = temperature
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
        Gửi frame ảnh sang Gemini Vision API để nhận phân tích JSON có cấu trúc.
        """
        if not self.enabled or self.model is None or frame is None:
            return self._fallback_cv_analysis(frame)

        try:
            # Resize và nén JPEG tối ưu băng thông
            h, w = frame.shape[:2]
            if w > 800:
                frame = cv2.resize(frame, (640, int(h * 640 / w)))
                
            ret, buf = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
            if not ret:
                return self._fallback_cv_analysis(frame)
                
            image_parts = [{"mime_type": "image/jpeg", "data": buf.tobytes()}]
            
            response = self.model.generate_content([PROMPT_ANALYZE_FISH, image_parts[0]])
            text = response.text.strip()
            
            # Trích xuất JSON
            match = re.search(r'\{.*\}', text, re.DOTALL)
            if match:
                data = json.loads(match.group(0))
                return {
                    "total_fish": int(data.get("total_fish", 0)),
                    "dead_fish": int(data.get("dead_fish", 0)),
                    "abnormal_fish": int(data.get("abnormal_fish", 0)),
                    "water_turbidity": int(data.get("water_turbidity", 10)),
                    "is_alert": bool(data.get("is_alert", False)),
                    "summary": str(data.get("summary", "Bể cá bình thường.")),
                    "ai_engine": self.model_name
                }
            else:
                return json.loads(text)
                
        except Exception as e:
            print(f"[GEMINI INFERENCE ERROR] Loi phan tich Gemini: {e}")
            return self._fallback_cv_analysis(frame)

    def _fallback_cv_analysis(self, frame):
        """
        Thuật toán xử lý ảnh cổ điển (OpenCV) dự phòng khi chưa có API Key hoặc mất Internet.
        """
        if frame is None:
            return {
                "total_fish": 0, "dead_fish": 0, "abnormal_fish": 0,
                "water_turbidity": 10, "is_alert": False,
                "summary": "Khong co tin hieu camera.", "ai_engine": "Fallback-CV"
            }
            
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        # Tinh toan do bien thien do tuong phan Laplacian (uoc tinh do duc)
        laplacian_var = cv2.Laplacian(gray, cv2.CV_64F).var()
        # Chuyen sang thang diem do duc 0 - 100%
        turbidity = int(max(5, min(95, 100 - (laplacian_var / 5.0))))
        
        return {
            "total_fish": 5, # Gia lap uoc tinh
            "dead_fish": 0,
            "abnormal_fish": 0,
            "water_turbidity": turbidity,
            "is_alert": (turbidity >= 40),
            "summary": f"Che do Du phong: Nuoc co do duc xap xi {turbidity}%.",
            "ai_engine": "Fallback-CV"
        }
