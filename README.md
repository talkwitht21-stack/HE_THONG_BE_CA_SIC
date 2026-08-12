# HỆ THỐNG GIÁM SÁT VÀ ĐIỀU KHIỂN TỰ ĐỘNG BỂ CÁ ỨNG DỤNG IOT VÀ TRÍ TUỆ NHÂN TẠO

Dự án nghiên cứu và ứng dụng nền tảng Vạn vật kết nối (IoT) kết hợp với Trí tuệ nhân tạo (AI) nhằm xây dựng một hệ thống giám sát và điều khiển tự động toàn diện cho môi trường thủy sinh. Hệ thống được thiết kế để duy trì môi trường sống ổn định cho sinh vật, đồng thời tích hợp thị giác máy tính (Computer Vision) để phân tích hành vi và trạng thái sức khỏe của sinh vật theo thời gian thực.

---

## MỤC LỤC
1. [Mục tiêu nghiên cứu](#1-mục-tiêu-nghiên-cứu)
2. [Đặt vấn đề và Đối tượng nghiên cứu](#2-đặt-vấn-đề-và-đối-tượng-nghiên-cứu)
3. [Khảo sát hiện trạng và Điểm mới của đề tài](#3-khảo-sát-hiện-trạng-và-điểm-mới-của-đề-tài)
4. [Các tính năng cốt lõi](#4-các-tính-năng-cốt-lõi)
5. [Quy trình hoạt động và Kiến trúc hệ thống](#5-quy-trình-hoạt-động-và-kiến-trúc-hệ-thống)
6. [Cấu trúc phần cứng và phần mềm](#6-cấu-trúc-phần-cứng-và-phần-mềm)

---

## 1. MỤC TIÊU NGHIÊN CỨU
Mục tiêu chính của dự án là xây dựng một hệ thống quản lý bể cá thông minh dựa trên IoT và AI, nhằm:
- Đảm bảo và duy trì các thông số của môi trường nước ở trạng thái cân bằng và lý tưởng nhất.
- Giảm thiểu rủi ro sinh vật chết do thiếu sự giám sát thường xuyên.
- Cung cấp nền tảng quản trị từ xa toàn diện, cho phép người dùng giám sát và điều khiển hệ sinh thái của bể cá thông qua giao diện Web/Ứng dụng quản lý.

## 2. ĐẶT VẤN ĐỀ VÀ ĐỐI TƯỢNG NGHIÊN CỨU

### 2.1. Đối tượng nghiên cứu và ứng dụng
- Cá nhân nuôi thủy sinh, cá cảnh tại nhà.
- Cơ sở kinh doanh cá cảnh quy mô vừa và nhỏ.
- Cá nhân có lịch trình bận rộn, thường xuyên công tác, hạn chế thời gian chăm sóc thường xuyên.

### 2.2. Vấn đề thực tiễn
- **Biến động môi trường:** Sinh vật thủy sinh có độ nhạy cảm cao với sự thay đổi của môi trường. Sự biến thiên đột ngột về nhiệt độ hoặc mực nước mà không được phát hiện kịp thời sẽ dẫn đến rủi ro hao hụt sinh vật.
- **Rủi ro trong quá trình chăm sóc:** Hoạt động cung cấp thức ăn thiếu tính chu kỳ sẽ gây ảnh hưởng tiêu cực đến sức khỏe của sinh vật.
- **Hạn chế về mặt giám sát:** Sự vắng mặt của người quản lý tạo ra khoảng trống trong việc nắm bắt thông tin và tình trạng của hệ sinh thái.

### 2.3. Giải pháp đề xuất
Hệ thống triển khai cơ chế tự động hóa toàn diện bao gồm: thu thập dữ liệu môi trường, phân tích hình ảnh, phát tín hiệu cảnh báo và thực thi các chuỗi hành động khắc phục (thay nước, kích hoạt sục khí oxy, cung cấp thức ăn) mà không đòi hỏi sự can thiệp liên tục từ người sử dụng.

## 3. KHẢO SÁT HIỆN TRẠNG VÀ ĐIỂM MỚI CỦA ĐỀ TÀI

### 3.1. Hiện trạng các sản phẩm trên thị trường
Thông qua khảo sát các sản phẩm bể cá thông minh tiêu biểu trên thị trường như: Xiaomi Mijia 20L MYG100, DINGSMART Mini Wi-Fi Tank, Xiaomi Mijia 10L MYG200, Hygger Smart Aquarium Kit, và Smart Aquarium 5 Gallon Glass Tank, có thể ghi nhận các hệ thống này đã đáp ứng được yêu cầu kiểm soát môi trường với các chức năng cơ bản:
- **Điều khiển từ xa:** Thiết lập thông qua kết nối Wi-Fi/Bluetooth.
- **Kiểm soát ánh sáng:** Điều chỉnh cường độ, màu sắc và chu kỳ chiếu sáng.
- **Ổn định nhiệt độ:** Tích hợp bộ sưởi tự động để điều tiết nhiệt độ nước.
- **Lọc nước tuần hoàn:** Hệ thống lọc nhiều lớp giúp duy trì chất lượng nước.
- **Cung cấp thức ăn tự động:** Định lượng và thiết lập lịch trình cho ăn.

### 3.2. Điểm đóng góp và Tính đột phá
Hạn chế lớn nhất của các hệ thống hiện hữu là chỉ dừng lại ở việc giám sát môi trường nước, thiếu đi cơ chế giám sát trực tiếp thực thể (sinh vật). 
Dự án này tích hợp công nghệ **Thị giác máy tính (Computer Vision)** để khắc phục hạn chế nêu trên:
- **Giám sát thời gian thực:** Camera thực hiện theo dõi liên tục hoạt động của sinh vật.
- **Phân tích hành vi:** Triển khai mô hình học sâu (Deep Learning) nhằm phân tích hình ảnh, phát hiện các cá thể có biểu hiện bơi bất thường, mang mầm bệnh hoặc đã chết.
- **Cảnh báo khẩn cấp:** Hệ thống Web Server tự động kích hoạt tiến trình gửi thư điện tử (Email) cảnh báo tới người quản lý, đính kèm dữ liệu hình ảnh/video để hỗ trợ ra quyết định kịp thời.

## 4. CÁC TÍNH NĂNG CỐT LÕI
1. **Theo dõi thông số môi trường 24/7:** Đo đạc nhiệt độ, mực nước (có khả năng mở rộng tích hợp cảm biến pH, TDS).
2. **AI Camera phân tích hành vi và môi trường:** Đánh giá mức độ vẩn đục của nước, nhận diện tình trạng sinh vật (chết, mắc bệnh, bơi lờ đờ).
3. **Tự động hóa chuỗi hành động:**
   - Kích hoạt quạt tản nhiệt hoặc máy sưởi dựa trên mức chênh lệch nhiệt độ.
   - Điều khiển bơm/thay nước khi mực nước suy giảm hoặc AI phân tích hình ảnh phát hiện nước bị đục.
   - Kích hoạt máy sủi oxy theo điều kiện thiết lập.
   - Cung cấp thức ăn theo định mức và lịch trình.
4. **Nền tảng Quản trị (Hỗ trợ Offline):** Cho phép quản lý từ xa, xem video trực tiếp, và hiển thị trạng thái hoạt động thực của toàn bộ thiết bị (bơm, sưởi, quạt, đèn). Hệ thống hỗ trợ khả năng điều khiển thiết bị thủ công thông qua Mạng cục bộ (LAN), đảm bảo duy trì hoạt động ngay cả khi gián đoạn kết nối Internet.

## 5. QUY TRÌNH HOẠT ĐỘNG VÀ KIẾN TRÚC HỆ THỐNG

### 5.1. Thu thập và Xử lý dữ liệu (Kiến trúc Master - Slave)
- **ESP32-Slave (Cụm Cảm biến):** Chịu trách nhiệm trực tiếp thu thập dữ liệu từ cảm biến nhiệt độ nước, cảm biến nhiệt đới - độ ẩm không khí và truyền tín hiệu trạng thái về nút trung tâm.
- **ESP32-Master (Nút Gateway Trung tâm):** Đóng vai trò là bộ vi điều khiển chính. Nút này tiếp nhận toàn bộ dữ liệu từ ESP32-Slave qua giao thức UART, đồng thời nhận tín hiệu và dữ liệu đếm số lượng sinh vật từ Raspberry Pi 5. Nút Gateway có nhiệm vụ tổng hợp và truyền tải dữ liệu lên nền tảng ThingsBoard.
- **Thuật toán kích hoạt sưởi/quạt (Kiểm tra chéo - Cross-validation):** ESP32-Master chỉ phát lệnh chuyển đổi trạng thái của máy sưởi hoặc quạt tản nhiệt khi **đồng thời cả hai cảm biến** (nhiệt độ nước và nhiệt độ/độ ẩm môi trường) ghi nhận sự biến thiên tương quan. Thuật toán này ngăn chặn các lỗi kích hoạt giả (false positive) phát sinh do sự cố phần cứng của một cảm biến đơn lẻ.
- **Cơ chế dự phòng (Failover) với Pi 5:** Trong tình huống phát sinh lỗi phần cứng làm gián đoạn liên kết UART giữa ESP32-Slave và ESP32-Master, Raspberry Pi 5 sẽ tự động đảm nhận vai trò dự phòng (Backup Gateway). Thiết bị này sẽ kích hoạt giao diện Wi-Fi để tiếp tục truyền tải toàn bộ dữ liệu lên nền tảng ThingsBoard, đảm bảo tính sẵn sàng cao (High Availability) cho hệ thống.

### 5.2. Xử lý ảnh bằng Trí tuệ Nhân tạo (Computer Vision)
- Thiết bị thu hình (IP Camera/Smartphone) được bố trí tại vị trí quan sát bể, đảm nhiệm việc thu nhận và truyền luồng video về thiết bị điện toán biên **Raspberry Pi 5**.
- **Vai trò chuyên biệt của Pi 5:** Pi 5 chuyên trách xử lý mô hình Trí tuệ Nhân tạo (AI) (tiến hành chụp ảnh chu kỳ 2 phút/lần để phân tích mức độ vẩn đục và trạng thái sinh vật). Kết quả phân tích sẽ được luân chuyển về **ESP32-Master**.
- **Quy trình xác minh:** Khi mô hình phân loại phát hiện trạng thái bất thường (sinh vật lật ngửa), hệ thống tự động chuyển đổi sang cơ chế chụp và phân tích liên tục tần số cao nhằm xác thực tình trạng tử vong.
- Sau quá trình xác thực, hệ thống tiến hành tính toán lại số lượng cá thể, truyền dữ liệu về ESP32-Master để đồng bộ hóa với cơ sở dữ liệu lưu trữ.

### 5.3. Quản trị và Đánh giá cảnh báo (AI trên Web Server)
- **Giám sát và Điều khiển (Offline Support):** Web Server thu nhận và lưu trữ chính xác trạng thái logic (Bật/Tắt) của từng thiết bị trong hệ thống. Quản trị viên có đặc quyền can thiệp thủ công thông qua giao diện Web. Việc triển khai Web Server cho phép người dùng thao tác thông qua mạng nội bộ (LAN), đảm bảo tính toàn vẹn của việc điều khiển ngoại tuyến khi xảy ra sự cố mạng diện rộng.
- **Mô hình AI Đánh giá Tổng quát:** Dữ liệu môi trường (từ ESP32) và dữ liệu thị giác máy tính (từ Raspberry Pi 5) được tổng hợp tại Web Server. Web Server triển khai một mô hình AI phân tích để đánh giá toàn diện, phân loại xem trạng thái hệ thống có đạt ngưỡng **"Cảnh báo Khẩn cấp" (Critical Alarm)** hay không.
- Nếu thuật toán đánh giá phân loại trạng thái ở mức rủi ro cao (ví dụ: nhiệt độ vượt ngưỡng an toàn kết hợp với độ đục cao và sinh vật lờ đờ), hệ thống sẽ lập tức khởi tạo tiến trình gửi Email khẩn cấp đến quản trị viên.

### 5.4. Sơ đồ khối quy trình (Workflow Diagram)

```mermaid
graph TD
    %% Khối Cảm biến & ESP32
    subgraph Thu thập & Điều khiển (Hardware)
        S1["Cảm biến nhiệt độ nước"] -->|"Dữ liệu"| ESPSlave{"ESP32-Slave"}
        S2["Cảm biến nhiệt/độ ẩm MT"] -->|"Dữ liệu"| ESPSlave
        ESPSlave -->|"Truyền tín hiệu"| ESPMaster{"ESP32-Master<br/>(Gateway)"}
        
        ESPMaster -->|"Đồng thời biến thiên"| Check{"Kiểm định chéo (Cross-check)"}
        Check -->|"Biến thiên Lạnh"| H_ON["Kích hoạt máy sưởi"]
        Check -->|"Biến thiên Nóng"| F_ON["Kích hoạt quạt tản nhiệt"]
    end

    %% Khối AI Camera & Pi 5
    subgraph Xử lý Hình ảnh & Điện toán Biên (Edge AI)
        IPCam["Thiết bị thu hình (IP Camera)"] -->|"Chụp 2 phút/lần"| Pi5{"Raspberry Pi 5 (AI Edge)"}
        Pi5 -->|"Phân loại Độ đục / Tử vong"| Verify{"Phân tích & Thống kê sinh vật"}
        Verify -->|"Truyền tín hiệu & Dữ liệu"| ESPMaster
    end

    %% Khối Cảnh báo & Lưu trữ
    ESPMaster -->|"Đồng bộ dữ liệu"| Cloud[("Nền tảng ThingsBoard (Cloud)")]
    Pi5 -.->|"Kích hoạt Failover (Lỗi UART)"| Cloud
    
    Cloud --> WebServer{"Web Server (Phân tích Tổng quát)"}
    WebServer -->|"Đánh giá đa biến"| AI_Check{"Trạng thái Khẩn cấp?"}
    AI_Check -->|"Đạt ngưỡng Cảnh báo"| Alert(("Web Server gửi Email khẩn cấp"))
```

## 6. CẤU TRÚC PHẦN CỨNG VÀ PHẦN MỀM
- **Phần cứng viễn thông & Điều khiển (Hardware/IoT):** Hệ thống triển khai 02 module ESP32 (cấu trúc Master-Slave) / Arduino và 01 thiết bị Raspberry Pi 5 (đảm nhiệm Edge AI).
- **Thiết bị Cảm biến & Cơ cấu chấp hành:** Cảm biến đo nhiệt độ dung dịch (DS18B20), Cảm biến siêu âm/Phao từ (HC-SR04), Cảm biến nhiệt đới - độ ẩm môi trường (DHT11/DHT22), Động cơ Servo (Cấp thức ăn).
- **Hệ thống Thị giác máy tính (AI Camera):** IP Camera / ESP32-CAM / Webcam tích hợp kết nối cùng Raspberry Pi.
- **Kiến trúc Dữ liệu và Đám mây (Backend & Cloud):** Nền tảng ThingsBoard (đóng vai trò lưu trữ cơ sở dữ liệu và MQTT Broker) kết hợp cùng Web Server (phát triển trên nền tảng Node.js/Python có tích hợp mô hình AI) để phục vụ tác vụ phân tích và gửi thông báo.
- **Giao diện Người dùng (Frontend):** Bảng điều khiển (Dashboard) của nền tảng ThingsBoard, hỗ trợ mở rộng bằng Ứng dụng di động tùy chỉnh (React Native / Flutter).
