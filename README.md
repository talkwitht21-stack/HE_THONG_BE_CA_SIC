# 🐟 Bể Cá Thông Minh IoT & AI (Smart Aquarium Monitoring System)

Dự án ứng dụng công nghệ vạn vật kết nối (IoT) và Trí tuệ nhân tạo (AI) để xây dựng một hệ thống giám sát và điều khiển tự động hoàn toàn cho bể cá. Hệ thống không chỉ duy trì môi trường sống ổn định cho thủy sinh mà còn sử dụng AI Camera để theo dõi trực tiếp tình trạng sức khỏe của cá theo thời gian thực.

---

## 📑 Mục lục
1. [Mục tiêu đề tài](#1-mục-tiêu-đề-tài)
2. [Vấn đề thực tế và Đối tượng hướng đến](#2-vấn-đề-thực-tế-và-đối-tượng-hướng-đến)
3. [Khảo sát thị trường và Tính đột phá của dự án](#3-khảo-sát-thị-trường-và-tính-đột-phá-của-dự-án)
4. [Các tính năng chính](#4-các-tính-năng-chính)
5. [Quy trình hoạt động](#5-quy-trình-hoạt-động)
6. [Kiến trúc hệ thống dự kiến](#6-kiến-trúc-hệ-thống-dự-kiến)

---

## 1. Mục tiêu đề tài
Xây dựng một hệ thống giám sát và điều khiển tự động toàn diện cho bể cá thông qua ứng dụng IoT và AI. 
**Mục tiêu cốt lõi:**
- Duy trì môi trường nước luôn ở trạng thái lý tưởng và ổn định nhất.
- Giảm thiểu tối đa rủi ro cá chết do thiếu sự giám sát của con người.
- Hỗ trợ người dùng theo dõi và điều khiển toàn bộ hệ sinh thái của bể cá từ xa thông qua ứng dụng/Dashboard.

## 2. Vấn đề thực tế và Đối tượng hướng đến

### 🎯 Đối tượng sử dụng
- Người chơi hệ thủy sinh, nuôi cá cảnh tại nhà.
- Chủ các cửa hàng kinh doanh cá cảnh quy mô nhỏ và vừa.
- Những người bận rộn, thường xuyên đi công tác xa, không có thời gian chăm sóc bể cá thường xuyên.

### ❗ Vấn đề thực tiễn đang tồn tại
- **Rủi ro môi trường:** Cá cực kì nhạy cảm và dễ chết khi các thông số như nhiệt độ, độ pH, hoặc mực nước thay đổi đột ngột mà người nuôi không kịp thời phát hiện.
- **Chăm sóc thất thường:** Việc quên cho cá ăn hoặc cho ăn không đúng cữ làm ảnh hưởng sức khỏe của cá.
- **Thiếu sự giám sát:** Khi không có mặt ở nhà, người nuôi hoàn toàn "mù thông tin" về tình trạng của bể cá.

### 💡 Giải pháp giải quyết
Hệ thống sẽ **tự động hóa hoàn toàn** các khâu: Đo đạc chỉ số môi trường, gửi cảnh báo về điện thoại và trực tiếp thực thi các hành động khắc phục (thay nước, bật sục oxy, thả thức ăn) mà không cần sự can thiệp liên tục của con người.

## 3. Khảo sát thị trường và Tính đột phá của dự án

**Thực trạng các sản phẩm trên thị trường:**
Các hệ thống bể cá thông minh hiện tại (và các nghiên cứu đi trước) đã làm rất tốt việc kiểm soát môi trường với các tính năng:
- Điều khiển từ xa.
- Điều chỉnh cường độ/thời gian chiếu sáng.
- Cân bằng nhiệt độ tự động.
- Hệ thống lọc nước tuần hoàn.
- Máy cho cá ăn tự động.

**🚀 ĐIỂM ĐỘT PHÁ CỦA DỰ ÁN (Innovation)**
Các hệ thống trên thị trường **chỉ giám sát môi trường nước, nhưng không giám sát bản thân con cá**. 
Dự án này tích hợp thêm **AI Camera (Computer Vision)** để khắc phục nhược điểm trên:
- 👁️ **Giám sát thời gian thực:** Camera liên tục theo dõi hoạt động của cá.
- 🧠 **Phân tích hành vi:** Ứng dụng mô hình AI nhận diện hình ảnh để phát hiện cá có dấu hiệu bơi lờ đờ, bất thường, cá mang mầm bệnh hoặc cá đã chết.
- ⚡ **Cảnh báo tức thời:** Gửi Push Notification lập tức đến điện thoại người nuôi kèm theo hình ảnh/video để có biện pháp xử lý kịp thời.

## 4. Các tính năng chính (Key Features)
1. **Theo dõi thông số nước 24/7:** Nhiệt độ, độ đục, mực nước (có thể mở rộng thêm pH, TDS).
2. **Camera AI phân tích hành vi:** Nhận diện cá chết, cá bệnh, trạng thái bơi bất thường.
3. **Tự động hóa hành động:**
   - Tự động bật quạt tản nhiệt hoặc máy sưởi khi nhiệt độ nước lệch chuẩn.
   - Tự động bơm/xả nước khi mực nước quá thấp hoặc nước bị đục.
   - Bật máy sủi oxy khi cần thiết.
   - Cho cá ăn theo đúng lịch trình đã cài đặt.
4. **App/Web Dashboard Dashboard:** Quản lý từ xa, xem live-stream camera và nhận thông báo cảnh báo tức thì.

## 5. Quy trình hoạt động (Workflow)

*(Đang cập nhật - Sẽ được bổ sung sau)*

## 6. Kiến trúc hệ thống dự kiến
*(Cập nhật thêm tùy thuộc vào phần cứng bạn sử dụng)*
- **Hardware/IoT:** ESP32 / Arduino / Raspberry Pi (dùng cho AI Edge).
- **Sensors:** DS18B20 (Nhiệt độ), HC-SR04/Phao từ (Mực nước), Cảm biến độ đục, Servo (Máy cho ăn).
- **AI Camera:** Camera IP / ESP32-CAM / Webcam tích hợp Raspberry Pi.
- **Backend & Cloud:** MQTT Broker, Firebase Realtime Database, Node.js/Python server.
- **Frontend:** React / Flutter / Blynk App.

---
*Báo cáo được xây dựng chi tiết để làm nền tảng triển khai và thuyết minh cho dự án Bể Cá Thông Minh IoT & AI.*
