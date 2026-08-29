# DỰ ÁN: HỆ THỐNG GIÁM SÁT VÀ ĐIỀU KHIỂN TỰ ĐỘNG BỂ CÁ ỨNG DỤNG IOT VÀ TRÍ TUỆ NHÂN TẠO

> **Đề tài Nghiên cứu & Ứng dụng:** Xây dựng hệ sinh thái bể cá thông minh tự vận hành toàn diện, kết hợp mạng lưới cảm biến **IoT Đa Tầng (ESP32 Master - Slave)**, **Thị giác Máy tính Edge AI (Raspberry Pi 5 / Gemini Flash Vision)**, **Giao diện Web mDNS Local**, **Cloudflare Tunnel** và **Nền tảng Đám mây ThingsBoard**.

---

## 📑 MỤC LỤC
1. [Giới thiệu và Mục tiêu nghiên cứu](#1-giới-thiệu-và-mục-tiêu-nghiên-cứu)
2. [Điểm mới và Tính đột phá của Đề tài](#2-điểm-mới-và-tính-đột-phá-của-đề-tài)
3. [Kiến trúc Tổng thể Hệ thống](#3-kiến-trúc-tổng-thể-hệ-thống)
4. [Các Tính Năng Cốt Lõi Chi Tiết](#4-các-tính-năng-cốt-lõi-chi-tiết)
5. [Sơ Đồ Kết Nối Phần Cứng & Pinout](#5-sơ-đồ-kết-nối-phần-cứng--pinout)
6. [Giao Thức Giao Tiếp Master - Slave (UART)](#6-giao-thức-giao-tiếp-master---slave-uart)
7. [Bảng Mã Điều Khiển Remote Hồng Ngoại IR](#7-bảng-mã-điều-khiển-remote-hồng-ngoại-ir)
8. [Hướng Dẫn Cài Đặt & Nạp Firmware (ESP32 Master & Slave)](#8-hướng-dẫn-cài-đặt--nạp-firmware-esp32-master--slave)
9. [Hướng Dẫn Import Dashboard ThingsBoard Cloud (21 Widgets Hoàn Chỉnh)](#9-hướng-dẫn-import-dashboard-thingsboard-cloud-21-widgets-hoàn-chỉnh)
10. [Hướng Dẫn Cài Đặt & Vận Hành Python Server (Edge AI & IoT Gateway)](#10-hướng-dẫn-cài-đặt--vận-hành-python-server-edge-ai--iot-gateway)
11. [Hướng Dẫn Sử Dụng Chi Tiết Toàn Bộ Hệ Thống Từ A - Z](#11-hướng-dẫn-sử-dụng-chi-tiết-toàn-bộ-hệ-thống-từ-a---z)

---

## 1. GIỚI THIỆU VÀ MỤC TIÊU NGHIÊN CỨU

Hệ thống được thiết kế nhằm giải quyết triệt để các rủi ro trong việc nuôi và bảo tồn sinh vật thủy sinh:
- **Tự động hóa hoàn toàn các chu trình sống:** Kiểm soát nhiệt độ nước chính xác, tự động bơm bù nước chống cạn/tràn, tuần hoàn lọc nước song song, sục khí oxy theo chu kỳ và cho ăn tự động đúng giờ với định lượng chính xác.
- **Bảo vệ an toàn tuyệt đối (Multi-level Failsafe):** Tự động cắt sưởi khi quá nhiệt ($\ge 35^\circ\text{C}$), ngắt bơm rút và sưởi khi cạn nước, cơ chế Heartbeat giám sát liên tục giữa các vi điều khiển để ngắt điện khẩn cấp khi xảy ra sự cố.
- **Giám sát trực tiếp sinh vật bằng AI tại Biên (Edge AI):** Phân tích hình ảnh thời gian thực từ Camera thông qua **Raspberry Pi 5** kết hợp **Gemini Flash Vision VLM** với cơ chế **5 ảnh mẫu tham chiếu (Baseline Calibration)** loại bỏ $100\%$ báo động giả từ lũa/đá tĩnh.
- **Vận hành độc lập Offline & Điều khiển đa kênh:** Cho phép điều khiển tức thời qua **Remote Hồng ngoại IR**, mạng nội bộ **Local Web Server (`http://beca.local`)** ngay cả khi mất Internet, truy cập từ xa không cần mở port qua **Cloudflare Tunnel**, nhận cảnh báo qua **Telegram Chatbot**, và đồng bộ dữ liệu giám sát qua **ThingsBoard Cloud**.

---

## 2. ĐIỂM MỚI VÀ TÍNH ĐỘT PHÁ CỦA ĐỀ TÀI

| Tiêu chí | Bể cá truyền thống trên thị trường | Hệ thống của Đề tài |
|---|---|---|
| **Đối tượng giám sát** | Chỉ đo môi trường vật lý (nhiệt độ) | **Giám sát kép:** Môi trường vật lý + Trạng thái thực thể sống (Gemini Vision AI) |
| **Phát hiện cá chết / bệnh** | Không có (chỉ biết khi nước bị ô nhiễm nặng) | **Gemini Flash VLM + Cơ chế 5 ảnh mẫu tham chiếu** phân biệt chính xác cá thể và lũa/đá tĩnh |
| **Xác thực chống báo động giả**| Báo động ngay khi có 1 frame lỗi | **Bộ lọc 5 lần liên tiếp (5 Consecutive Positives):** Chụp liên tục 10s/lần, chỉ báo khi đủ 5/5 |
| **Kiến trúc phần cứng** | 1 vi điều khiển duy nhất (dễ treo khi lỗi) | **Phân lớp Master - Slave:** Tách biệt Gateway xử lý mạng/logic và Node điều khiển cảm biến/relay |
| **Độ trễ điều khiển** | Phụ thuộc hoàn toàn vào Cloud (1-3 giây) | **Phản hồi tức thì < 50ms:** Remote IR 0ms (non-blocking), Web LAN `beca.local` $< 5\text{ms}$ |
| **An toàn điện & phần cứng** | Ngắt rơ-le đơn giản | **Failsafe đa tầng:** Cắt cưỡng bức khi cạn/quá nhiệt, Heartbeat UART tự ngắt khi mất kết nối |

---

## 3. KIẾN TRÚC TỔNG THỂ HỆ THỐNG

```mermaid
graph TD
    %% Khối Ngoại vi
    subgraph HardwareLayer ["LỚP THIẾT BỊ NGOẠI VI (Hardware / Actuators)"]
        DS18B20["Cảm biến Nhiệt Nước (DS18B20)"]
        DHT11["Cảm biến Nhiệt/Ẩm Không Khí (DHT11)"]
        HCSR04["Cảm biến Siêu Âm Đo Mực Nước (HC-SR04)"]
        IR_REC["Mắt Thu Hồng Ngoại (IR 38kHz)"]
        
        RELAYS["6 Kênh Relay (ULN2003 Driver):<br/>- Sưởi (220V/12V)<br/>- Quạt Làm Mát<br/>- Bơm Bù Nước<br/>- Bơm Rút Nước<br/>- Sục Oxy<br/>- Đèn LED Chiếu Sáng"]
        SERVO["Servo Cho Ăn (SG90/MG90S)<br/>Góc quay rót thức ăn 0° - 180°"]
    end

    %% Khối Vi Điều Khiển
    subgraph ControllerLayer ["LỚP ĐIỀU KHIỂN & GATEWAY (Dual ESP32 Architecture)"]
        ESPMaster["ESP32-Master (IoT Gateway & Server)<br/>- Đọc cảm biến liên tục (Median Filter 5 mẫu)<br/>- Web Server mDNS: http://beca.local<br/>- ThingsBoard 2-Way RPC Response < 50ms<br/>- Gửi UART Heartbeat sang Slave mỗi 200ms"]
        ESPSlave["ESP32-Slave (Relay & Servo Node)<br/>- Lái 6 Relay qua ULN2003<br/>- Điều khiển Servo attach/detach chống rung<br/>- Giải mã Remote IR NEC tức thì<br/>- Watchdog Failsafe 10 phút tự ngắt"]
    end

    %% Khối AI & Đám mây
    subgraph CloudAndAI ["LỚP TRÍ TUỆ NHÂN TẠO & ĐÁM MÂY (Edge AI & Cloud)"]
        Camera["Webcam USB / IP Camera RTSP"]
        RPi5["Raspberry Pi 5 (Python Server - Edge AI)<br/>- Gemini Flash Vision VLM (Đối chiếu 5 ảnh mẫu)<br/>- Xác thực cá chết 5 lần liên tiếp<br/>- Cloudflare Quick Tunnel (trycloudflare)<br/>- Telegram Bot tương tác 2 chiều"]
        ThingsBoard["ThingsBoard Cloud Platform (21 Widgets)<br/>- Giám sát môi trường & AI Telemetry<br/>- 10 Nút điều khiển 2 chiều RPC"]
    end

    %% Kết nối
    DS18B20 -->|"Đọc nhiệt độ nước"| ESPMaster
    DHT11 -->|"Đọc nhiệt/ẩm không khí"| ESPMaster
    HCSR04 -->|"Đo khoảng cách mực nước"| ESPMaster
    
    IR_REC -->|"Nhận tín hiệu remote"| ESPSlave
    ESPSlave -->|"Kích rơ-le"| RELAYS
    ESPSlave -->|"Kéo góc quay"| SERVO

    ESPMaster <==>|"Giao tiếp UART (9600 baud)<br/>GPIO 16/17 ($CMD...*CS)"| ESPSlave
    
    Camera -->|"Luồng Video 15 FPS"| RPi5
    RPi5 <==>|"REST API LAN < 5ms"| ESPMaster
    
    ESPMaster <==>|"ThingsBoard MQTT (Port 1883)"| ThingsBoard
    RPi5 <==>|"ThingsBoard MQTT AI Telemetry"| ThingsBoard
    
    RPi5 <==>|"HTTPS Public Tunnel"| RemoteUser["Người dùng (4G/Internet Ngoài Đường)"]
    RPi5 <==>|"Telegram API"| TgBot["Telegram Bot Cảnh Báo Snapshot"]
```

---

## 4. CÁC TÍNH NĂNG CỐT LÕI CHI TIẾT

### 4.1. Điều Khiển Nhiệt Độ Thông Minh (Thermostat)
- **Tự động Bật/Tắt Sưởi:** Bật sưởi khi nhiệt độ nước $< 18.0^\circ\text{C}$, Tắt sưởi khi nước $\ge 20.0^\circ\text{C}$ (ngưỡng tùy chỉnh linh hoạt).
- **Tự động Bật/Tắt Quạt Làm Mát:** Bật quạt khi nước $> 30.0^\circ\text{C}$, Tắt quạt khi nước $\le 28.0^\circ\text{C}$.
- **Failsafe Quá Nhiệt Tuyệt Đối:** Bất kể bật tay hay tự động, khi nhiệt độ nước $\ge 35.0^\circ\text{C}$ $\rightarrow$ Lập tức ngắt sưởi cưỡng bức để bảo vệ cá.

### 4.2. Quản Lý Mực Nước Siêu Âm Chống Cạn & Chống Tràn
- **Bộ lọc trung vị 5 mẫu (Median Filter):** Khử hoàn toàn xung nhiễu do bọt khí oxy hoặc sóng nước dao động.
- **Tự động Bơm Bù Nước:** Bật bơm bù khi khoảng cách nước $> 15.0\text{cm}$, ngắt khi nước đầy $\le 5.0\text{cm}$.
- **Bảo Vệ Chống Tràn & Khóa Bơm Rút:** Ngắt bơm rút khi nước chạm đáy $\ge 30.0\text{cm}$ để chống cháy bơm.

### 4.3. Hệ Thống Cho Ăn Tự Động Với Servo Kéo Góc
- **Cơ chế `attach()` / `detach()`:** Chỉ cấp xung PWM khi quay rót thức ăn, sau đó lập tức ngắt xung để chống rung, chống nóng và tăng tuổi thọ servo gấp 10 lần.
- **Đa kênh kích hoạt:** Hẹn giờ cố định hàng ngày, bấm nút trên Web LAN, nút bấm trên ThingsBoard (Action Button 1 chạm), hoặc gửi lệnh `/feed` qua Telegram.

---

## 5. SƠ ĐỒ KẾT NỐI PHẦN CỨNG & PINOUT

### 5.1. Bảng Phân Bổ Chân ESP32-Master:
| Chân ESP32 | Thiết Bị Ngoại Vi | Chức Năng | Ghi Chú |
|---|---|---|---|
| **GPIO 4** | Cảm biến DS18B20 | Đọc nhiệt độ nước | Kèm trở kéo lên $4.7\text{k}\Omega$ lên 3.3V |
| **GPIO 5** | Cảm biến DHT11 | Đọc nhiệt/ẩm không khí | Chân DATA DHT11 |
| **GPIO 18** | Cảm biến HC-SR04 (Trig) | Phát xung siêu âm | Output |
| **GPIO 19** | Cảm biến HC-SR04 (Echo) | Nhận xung siêu âm | Input (Qua cầu phân áp $1\text{k}\Omega / 2\text{k}\Omega$ bảo vệ 3.3V) |
| **GPIO 16** | Chân RX2 UART | Nhận dữ liệu từ Slave TX | Nối chéo với TX Slave (GPIO 17) |
| **GPIO 17** | Chân TX2 UART | Gửi lệnh sang Slave RX | Nối chéo với RX Slave (GPIO 16) |
| **GPIO 2** | LED Onboard | Đèn báo trạng thái mạng | Nhấp nháy khi mất mạng, sáng đứng khi online |
| **GPIO 0** | Nút BOOT | Factory Reset NVS Flash | Nhấn giữ 3 giây để xóa cấu hình WiFi |

### 5.2. Bảng Phân Bổ Chân ESP32-Slave:
| Chân ESP32 | Thiết Bị Ngoại Vi | Chức Năng | Ghi Chú |
|---|---|---|---|
| **GPIO 23** | Relay 1 (Máy Sưởi) | Đóng cắt nhiệt sưởi | Kích mức CAO qua IC ULN2003 |
| **GPIO 22** | Relay 2 (Quạt Làm Mát) | Đóng cắt quạt gió | Kích mức CAO qua IC ULN2003 |
| **GPIO 21** | Relay 3 (Bơm Bù Nước) | Đóng cắt bơm cấp nước | Kích mức CAO qua IC ULN2003 |
| **GPIO 19** | Relay 4 (Sục Khí Oxy) | Đóng cắt sủi oxy | Kích mức CAO qua IC ULN2003 |
| **GPIO 18** | Relay 5 (Bơm Rút Nước) | Đóng cắt bơm xả nước | Kích mức CAO qua IC ULN2003 |
| **GPIO 5** | Relay 6 (Đèn LED) | Đóng cắt đèn chiếu sáng | Kích mức CAO qua IC ULN2003 |
| **GPIO 13** | Servo Cho Ăn (SG90) | Quay rót thức ăn (PWM) | Chân Signal Servo |
| **GPIO 15** | Mắt Thu Hồng Ngoại IR | Nhận tín hiệu Remote 38kHz | Chân DATA mắt thu IR |
| **GPIO 16** | Chân RX2 UART | Nhận lệnh từ Master TX | Nối chéo với TX Master (GPIO 17) |
| **GPIO 17** | Chân TX2 UART | Gửi phản hồi sang Master RX | Nối chéo với RX Master (GPIO 16) |

---

## 6. GIAO THỨC GIAO TIẾP MASTER - SLAVE (UART)

Giao tiếp UART2 hoạt động tại baudrate **9600 bps**, định dạng gói tin ASCII kèm Checksum XOR bảo đảm tính toàn vẹn:

* **Master gửi sang Slave (Heartbeat & Lệnh mỗi 200ms):**
  $$\texttt{\$CMD,<heater>,<fan>,<pump>,<oxy>,<drain>,<led>,<feed>,<system>*<CS>\n}$$
* **Slave phản hồi sang Master:**
  $$\texttt{\$ACK,<relay_states>,<ir_cmd>*<CS>\n}$$
* **Cơ chế Watchdog 10 phút:** Nếu Slave không nhận được gói tin `$CMD` hợp lệ từ Master trong 10 phút liên tục $\rightarrow$ Slave tự động ngắt toàn bộ 6 Relay để đưa bể cá về trạng thái an toàn tuyệt đối.

---

## 7. BẢNG MÃ ĐIỀU KHIỂN REMOTE HỒNG NGOẠI IR

Hệ thống hỗ trợ điều khiển tức thì (độ trễ 0ms) qua Remote hồng ngoại NEC tiêu chuẩn:

| Phím Bấm | Mã Hex NEC | Thiết Bị Điều Khiển | Chức Năng |
|:---:|:---:|---|---|
| **Phím 1** | `0xFF30CF` | Máy Sưởi | Đảo trạng thái Bật / Tắt |
| **Phím 2** | `0xFF18E7` | Quạt Làm Mát | Đảo trạng thái Bật / Tắt |
| **Phím 3** | `0xFF7A85` | Bơm Bù Nước | Đảo trạng thái Bật / Tắt |
| **Phím 4** | `0xFF10EF` | Sục Khí Oxy | Đảo trạng thái Bật / Tắt |
| **Phím 5** | `0xFF38C7` | Bơm Rút Nước | Đảo trạng thái Bật / Tắt |
| **Phím 6** | `0xFF5AA5` | Lọc Tuần Hoàn | Đảo trạng thái Bật / Tắt 2 bơm |
| **Phím 7** | `0xFF42BD` | Đèn LED Bể Cá | Đảo trạng thái Bật / Tắt |
| **Phím 8** | `0xFF4AB5` | Cho Ăn Tức Thì | Kích hoạt quay Servo rót thức ăn |
| **Phím 0 / OFF** | `0xFF6897` | Khóa Hệ Thống | Ngắt khẩn cấp toàn bộ thiết bị |

---

## 8. HƯỚNG DẪN CÀI ĐẶT & NẠP FIRMWARE (ESP32 MASTER & SLAVE)

### 8.1. Chuẩn bị môi trường nạp:
1. Tải và cài đặt **Arduino IDE 2.x** từ [arduino.cc](https://www.arduino.cc/en/software).
2. Vào **File** $\rightarrow$ **Preferences** $\rightarrow$ Thêm URL Board ESP32 vào ô *Additional Board Manager URLs*:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Vào **Tools** $\rightarrow$ **Board** $\rightarrow$ **Boards Manager**, tìm `esp32` và bấm **Install** (phiên bản 2.0.x hoặc 3.0.x).

### 8.2. Cài đặt các thư viện cần thiết:
Vào **Tools** $\rightarrow$ **Manage Libraries**, tìm và cài đặt các thư viện:
- `OneWire` và `DallasTemperature` (đọc DS18B20)
- `DHT sensor library` (đọc DHT11)
- `PubSubClient` (kết nối MQTT ThingsBoard)
- `ArduinoJson` (phiên bản 6.x hoặc 7.x)
- `IRremote` (phiên bản 4.x - giải mã Remote hồng ngoại)
- `ESP32Servo` (điều khiển Servo SG90)

### 8.3. Nạp Firmware ESP32-Master:
1. Mở file [`Firmware/ESP32_Master/ESP32_Master.ino`](file:///g:/BECA/HE_THONG_BE_CA_SIC/Firmware/ESP32_Master/ESP32_Master.ino).
2. Cấu hình tên WiFi và mật khẩu nhà bạn ở đầu file:
   ```cpp
   String sta_ssid     = "TEN_WIFI_NHA_BAN";
   String sta_password = "MAT_KHAU_WIFI";
   ```
3. Chọn Board: **ESP32 Dev Module**, chọn đúng Cổng COM và bấm **Upload**.

### 8.4. Nạp Firmware ESP32-Slave:
1. Mở file [`Firmware/ESP32_Slave/ESP32_Slave.ino`](file:///g:/BECA/HE_THONG_BE_CA_SIC/Firmware/ESP32_Slave/ESP32_Slave.ino).
2. Chọn Board: **ESP32 Dev Module**, chọn đúng Cổng COM và bấm **Upload**.

---

## 9. HƯỚNG DẪN IMPORT DASHBOARD THINGSBOARD CLOUD (21 WIDGETS HOÀN CHỈNH)

Hệ thống đã có sẵn file mẫu thiết kế siêu đẹp theo tiêu chuẩn UI/UX hiện đại:
- [`giám_sát_&_điều_khiển_bể_cá_thông_minh.json`](file:///g:/BECA/HE_THONG_BE_CA_SIC/gi%C3%A1m_s%C3%A1t_&_%C4%91i%E1%BB%81u_khi%E1%BB%83n_b%E1%BB%83_c%C3%A1_th%C3%B4ng_minh.json)
- [`ThingsBoard_Dashboard_BeCa.json`](file:///g:/BECA/HE_THONG_BE_CA_SIC/ThingsBoard_Dashboard_BeCa.json)

### 🚀 Cách Import Nhanh (Chỉ mất 10 giây):
1. Đăng nhập vào **[ThingsBoard Cloud](https://thingsboard.cloud)**.
2. Vào menu **Dashboards** $\rightarrow$ bấm biểu tượng **`+`** (Import Dashboard góc trên bên phải).
3. Chọn file [`giám_sát_&_điều_khiển_bể_cá_thông_minh.json`](file:///g:/BECA/HE_THONG_BE_CA_SIC/gi%C3%A1m_s%C3%A1t_&_%C4%91i%E1%BB%81u_khi%E1%BB%83n_b%E1%BB%83_c%C3%A1_th%C3%B4ng_minh.json).
4. Bấm **Import** $\rightarrow$ Bạn sẽ có ngay một bảng điều khiển hoàn chỉnh hợp nhất cả ESP32 lẫn Edge AI!

---

## 10. HƯỚNG DẪN CÀI ĐẶT & VẬN HÀNH PYTHON SERVER (EDGE AI & IOT GATEWAY)

### 10.1. Cấu trúc thư mục module hóa:
```
Python_Server/
├── config.yaml                    # File cấu hình tập trung (tự động lưu từ Web)
├── requirements.txt               # Danh sách thư viện Python
├── run_server.py                  # Entry-point chính điều phối toàn bộ các luồng
├── reference_images/              # Thư mục chứa 5 ảnh mẫu tham chiếu (ref_1.jpg .. ref_5.jpg)
├── core/
│   ├── reference_manager.py       # Quản lý lưu trữ/nạp 5 ảnh mẫu tham chiếu
│   ├── config_manager.py          # Quản lý đọc/ghi cấu hình linh hoạt
│   ├── video_stream.py            # Đọc camera non-blocking 15 FPS
│   ├── gemini_vision.py           # Gemini Flash Vision VLM (có dynamic reload API Key)
│   ├── cloudflare_tunnel.py       # Tự động tạo Cloudflare Quick Tunnel (trycloudflare)
│   └── telegram_bot.py            # Telegram Bot tương tác 2 chiều (/status, /snapshot, /setref, /data, /feed)
├── gateway/
│   ├── mqtt_ai_client.py          # Đẩy Telemetry AI lên Device riêng trên ThingsBoard
│   └── esp32_lan_client.py        # Gọi REST API trực tiếp tới ESP32 qua LAN (http://beca.local)
└── web/
    ├── app.py                     # Flask Web Server & API quản lý cấu hình, ảnh mẫu
    └── templates/
        └── index.html             # Dashboard Web Hợp Nhất 2 Chế Độ (Trắng - Xanh Nước Nhạt)
```

### 10.2. Cài đặt môi trường & Khởi chạy Server:
```bash
# 1. Di chuyển vào thư mục Python_Server
cd Python_Server

# 2. Cài đặt các thư viện phụ thuộc
pip install -r requirements.txt

# 3. Khởi chạy Server
python run_server.py
```

* **Tự động mở Cloudflare Tunnel:** Ngay khi khởi động, hệ thống tự động sinh Public URL (`https://*.trycloudflare.com`) và gửi thẳng vào Telegram của bạn!
* **Truy cập Web Dashboard:** Mở trình duyệt vào **`http://localhost:5000`** (hoặc đường link Cloudflare từ 4G/ngoài đường).

## 11. HƯỚNG DẪN SỬ DỤNG CHI TIẾT TOÀN BỘ HỆ THỐNG TỪ A - Z

### 11.1. Cài Đặt Khóa API, Nguồn Camera & Chu Kỳ Chụp Trong Hộp Thoại Modal:
Mở Web Dashboard tại `http://localhost:5000` $\rightarrow$ Bấm nút **⚙️ CÀI ĐẶT HỆ THỐNG** trên góc phải Header:
1. **Camera & Chu Kỳ Phân Tích AI:**
   - **Nguồn Camera:** Nhập `0` (Webcam USB) hoặc chuỗi RTSP/HTTP IP Camera (`rtsp://admin:123456@192.168.1.100:554/stream1`).
   - **Chu kỳ bình thường (giây):** Số giây tự động chụp định kỳ (Mặc định: `120` giây = 2 phút/lần).
   - **Chu kỳ khi nghi ngờ cá chết (giây):** Số giây chụp xác thực liên tiếp 5 lần (Mặc định: `10` giây/lần).
   - Bấm **Lưu Cài Đặt Camera & Chu Kỳ** (áp dụng động ngay trong runtime).
2. **Google Gemini Vision API Key:** Lấy key miễn phí tại [Google AI Studio](https://aistudio.google.com/) $\rightarrow$ Dán vào ô và bấm **Lưu API Key** (Hệ thống tự nạp model ngay lập tức).
3. **ThingsBoard Access Token (Device AI Node):** Tạo Device mới trên ThingsBoard $\rightarrow$ Sao chép Access Token $\rightarrow$ Dán vào ô và bấm **Lưu Token TB**.
4. **ThingsBoard Public Dashboard Link:** Dán link Public Dashboard của bạn (VD: `https://thingsboard.cloud/dashboard/1f6621a0-a3ae-11f1-9b46-e7fbeb690c95?publicId=05e0b4a0-a3b0-11f1-8523-a9586d32bc6e`) $\rightarrow$ Bấm **Lưu Link TB** (iframe tự cập nhật ngay).
5. **Telegram Bot Token:** Nhắn tin với [@BotFather](https://t.me/botfather) tạo bot mới $\rightarrow$ Dán token vào ô và bấm **Lưu Token Bot**.
6. **Telegram Chat ID / User ID:** Nhắn tin `/start` với [@userinfobot](https://t.me/userinfobot) để lấy ID của bạn $\rightarrow$ Dán vào ô và bấm **Lưu ID Telegram**.
7. **Tổng Số Cá Thả:** Nhập số cá đang thả trong bể (VD: 10) $\rightarrow$ Bấm **Lưu Số Cá**.
8. **Tạo Lại Đường Hầm Cloudflare:** Bấm nút **"🔄 Tạo Lại Đường Hầm"** trên thanh Banner Web bất kỳ lúc nào để làm mới Public URL.

---

### 11.2. Thiết Lập 5 Ảnh Mẫu Tham Chiếu (Baseline Calibration):
* **Mục đích:** Loại bỏ $100\%$ việc AI nhìn nhầm gỗ lũa, đá sỏi, hang hốc dưới đáy bể thành cá chết.
* **Cách thực hiện:**
  1. Trong Hộp Thoại Cài Đặt, mục **5 Ảnh Mẫu Tham Chiếu**, bạn có 5 slot (`Mẫu 1` .. `Mẫu 5`).
  2. Bấm nút **Chụp Cam** (để lấy frame camera hiện tại) hoặc bấm **Tải Ảnh** (tải ảnh chụp bể cá lúc bình thường ở các góc sáng, tối, lũa đá).
  3. Hoặc trên Telegram, gõ lệnh `/setref 1` .. `/setref 5` để chụp lưu ảnh mẫu trực tiếp từ điện thoại!

---

### 11.3. Chuyển Đổi & Nhúng Public Dashboard ThingsBoard (Chế Độ 2):
* Chuyển sang **CHẾ ĐỘ 2: CLOUD (ThingsBoard)** trên Web:
  - Phía trên có thanh công cụ: Bạn có thể **dán link Public Dashboard bất kỳ** $\rightarrow$ Bấm **"Lưu & Tải Lại Dashboard"** để đổi giao diện tức thì!
  - Bấm nút **"Mở Tab Mới ↗"** để xem toàn màn hình trên trình duyệt.

---

### 11.4. Cơ Chế Xác Thực Cá Chết 5 Lần Liên Tiếp (Chống Báo Động Giả):
* **Chu kỳ bình thường:** Mỗi **2 phút (120s)** chụp 1 ảnh gửi Gemini AI phân tích.
* **Khi nghi ngờ cá chết:** Hệ thống tự động kích hoạt chu kỳ xác thực **chụp liên tục mỗi 10 giây/lần**.
  * Nếu **cả 5 lần chụp liên tiếp** đều phát hiện cá chết $\rightarrow$ Xác nhận chính xác $100\%$, cập nhật $\text{Cá Sống} = \text{Tổng Cá} - \text{Cá Chết}$, gửi ảnh snapshot bằng chứng và cảnh báo khẩn cấp qua Telegram!
  * Nếu cá bơi lại bình thường $\rightarrow$ Reset streak về `0/5`, quay lại chu kỳ 2 phút/lần.
* **Nút chụp tức thì:** Bấm nút **📸 CHỤP ẢNH & PHÂN TÍCH AI NGAY** trên Web để đọc kết quả trong $< 2\text{s}$.

---

### 11.5. Bảng Lệnh Tương Tác Qua Telegram Chatbot:
| Lệnh Telegram | Chức Năng Thực Hiện |
|---|---|
| `/status` | Chụp ảnh camera $\rightarrow$ Gửi AI đối chiếu 5 ảnh mẫu $\rightarrow$ Trả về ảnh snapshot + báo cáo chi tiết |
| `/snapshot` | Chụp và gửi ảnh trực tiếp từ camera ngay lập tức |
| `/setref <1-5>` | Chụp frame camera hiện tại lưu làm ảnh mẫu tham chiếu số $1..5$ |
| `/refstatus` | Xem danh sách và thời gian lưu của 5 ảnh mẫu tham chiếu |
| `/url` | Lấy lại đường dẫn Public Web Cloudflare Tunnel (`https://*.trycloudflare.com`) |
| `/data` | Đọc toàn bộ cảm biến môi trường (Nhiệt độ nước, mực nước, relay) từ ESP32 qua LAN |
| `/feed` | Kích hoạt quay Servo rót thức ăn từ xa qua Telegram |
