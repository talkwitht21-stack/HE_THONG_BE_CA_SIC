# DỰ ÁN: HỆ THỐNG GIÁM SÁT VÀ ĐIỀU KHIỂN TỰ ĐỘNG BỂ CÁ ỨNG DỤNG IOT VÀ TRÍ TUỆ NHÂN TẠO

> **Đề tài Nghiên cứu & Ứng dụng:** Xây dựng hệ sinh thái bể cá thông minh tự vận hành toàn diện, kết hợp mạng lưới cảm biến **IoT Đa Tầng (ESP32 Master - Slave)**, **Thị giác Máy tính Edge AI (Raspberry Pi 5)**, **Giao diện Web mDNS Local** và **Nền tảng Đám mây ThingsBoard**.

---

## 📑 MỤC LỤC
1. [Giới thiệu và Mục tiêu nghiên cứu](#1-giới-thiệu-và-mục-tiêu-nghiên-cứu)
2. [Điểm mới và Tính đột phá của Đề tài](#2-điểm-mới-và-tính-đột-phá-của-đề-tài)
3. [Kiến trúc Tổng thể Hệ thống](#3-kiến-trúc-tổng-thể-hệ-thống)
4. [Các Tính Năng Cốt Lõi Chi Tiết](#4-các-tính-năng-cốt-lõi-chi-tiết)
5. [Sơ Đồ Kết Nối Phần Cứng & Pinout](#5-sơ-đồ-kết-nối-phần-cứng--pinout)
6. [Giao Thức Giao Tiếp Master - Slave (UART)](#6-giao-thức-giao-tiếp-master---slave-uart)
7. [Bảng Mã Điều Khiển Remote Hồng Ngoại IR](#7-bảng-mã-điều-khiển-remote-hồng-ngoại-ir)
8. [Giao Diện Web Dashboard & mDNS Local](#8-giao-diện-web-dashboard--mdns-local)
9. [Hướng Dẫn Cài Đặt & Nạp Firmware](#9-hướng-dẫn-cài-đặt--nạp-firmware)
10. [Hướng Dẫn Tạo Dashboard Trên ThingsBoard Cloud](#10-hướng-dẫn-tạo-dashboard-trên-thingsboard-cloud)
11. [Hướng Dẫn Cài Đặt & Vận Hành Python Server (Edge AI & IoT Gateway)](#11-hướng-dẫn-cài-đặt--vận-hành-python-server-edge-ai--iot-gateway)

---

## 1. GIỚI THIỆU VÀ MỤC TIÊU NGHIÊN CỨU

Hệ thống được thiết kế nhằm giải quyết triệt để các rủi ro trong việc nuôi và bảo tồn sinh vật thủy sinh:
- **Tự động hóa hoàn toàn các chu trình sống:** Kiểm soát nhiệt độ nước chính xác, tự động bơm bù nước chống cạn/tràn, tuần hoàn lọc nước song song, sục khí oxy theo chu kỳ và cho ăn tự động đúng giờ với định lượng chính xác.
- **Bảo vệ an toàn tuyệt đối (Multi-level Failsafe):** Tự động cắt sưởi khi quá nhiệt ($\ge 35^\circ\text{C}$), ngắt bơm rút và sưởi khi cạn nước, cơ chế Heartbeat giám sát liên tục giữa các vi điều khiển để ngắt điện khẩn cấp khi xảy ra sự cố.
- **Giám sát trực tiếp sinh vật bằng AI tại Biên (Edge AI):** Phân tích hình ảnh thời gian thực từ Camera thông qua **Raspberry Pi 5** để nhận diện độ vẩn đục của nước, phát hiện cá mắc bệnh, bơi lờ đờ hoặc tử vong nhằm đưa ra cảnh báo sớm qua Email/Cloud.
- **Vận hành độc lập Offline & Điều khiển đa kênh:** Cho phép điều khiển tức thời qua **Remote Hồng ngoại IR**, mạng nội bộ **Local Web Server (`http://beca.local`)** ngay cả khi mất Internet, và đồng bộ dữ liệu giám sát từ xa qua **ThingsBoard Cloud**.

---

## 2. ĐIỂM MỚI VÀ TÍNH ĐỘT PHÁ CỦA ĐỀ TÀI

| Tiêu chí | Bể cá truyền thống trên thị trường | Hệ thống của Đề tài |
|---|---|---|
| **Đối tượng giám sát** | Chỉ đo môi trường vật lý (nhiệt độ) | **Giám sát kép:** Môi trường vật lý + Trạng thái thực thể sống (AI Vision) |
| **Phát hiện cá chết / bệnh** | Không có (chỉ biết khi nước bị ô nhiễm nặng) | **Edge AI trên Raspberry Pi 5** nhận diện hành vi, đếm số lượng, phát hiện cá lật bụng |
| **Kiến trúc phần cứng** | 1 vi điều khiển duy nhất (dễ treo khi lỗi) | **Phân lớp Master - Slave:** Tách biệt Gateway xử lý mạng/logic và Node điều khiển cảm biến/relay |
| **Độ trễ điều khiển** | Phụ thuộc hoàn toàn vào Cloud (1-3 giây) | **Phản hồi tức thì < 50ms:** Remote IR 0ms (non-blocking), Web LAN `beca.local` |
| **An toàn điện & phần cứng** | Ngắt rơ-le đơn giản | **Failsafe đa tầng:** Cắt cưỡng bức khi cạn/quá nhiệt, Heartbeat PING/PONG 15s tự ngắt khi mất kết nối |

---

## 3. KIẾN TRÚC TỔNG THỂ HỆ THỐNG

```mermaid
graph TD
    %% Khối Cảm biến & Cơ cấu chấp hành
    subgraph HardwareLayer ["LỚP THIẾT BỊ NGOẠI VI (Hardware / Actuators)"]
        DS18B20["Cảm biến Nhiệt Nước (DS18B20)"]
        DHT22["Cảm biến Nhiệt/Ẩm Không Khí (DHT22)"]
        HCSR04["Cảm biến Siêu Âm Đo Nước (HC-SR04)"]
        IR_REC["Mắt Thu Hồng Ngoại (IR 38kHz)"]
        
        RELAYS["6 Kênh Relay:<br/>- Sưởi (220V/12V)<br/>- Quạt Làm Mát<br/>- Bơm Bù Nước<br/>- Bơm Rút Nước<br/>- Sục Oxy<br/>- Đèn LED Chiếu Sáng"]
        SERVO["Servo Cho Ăn (SG90/MG90S)<br/>Góc quay tùy chỉnh 10° - 180°"]
    end

    %% Khối Vi Điều Khiển
    subgraph ControllerLayer ["LỚP ĐIỀU KHIỂN & GATEWAY (Dual ESP32 Architecture)"]
        ESPSlave["ESP32-Slave (Sensor & Relay Node)<br/>- Đọc cảm biến Non-blocking 0ms<br/>- Giải mã Remote IR tức thì<br/>- Đóng ngắt Relay & Kéo Servo<br/>- Heartbeat PING/PONG 15s"]
        ESPMaster["ESP32-Master (IoT Gateway & Server)<br/>- Web Server SPA + mDNS: http://beca.local<br/>- Logic Failsafe & Thermostat độc lập<br/>- Quản lý Chu kỳ Lọc nước & Sục Oxy<br/>- Lưu trữ cấu hình Flash NVS (Preferences)"]
    end

    %% Khối AI & Đám mây
    subgraph CloudAndAI ["LỚP TRÍ TUỆ NHÂN TẠO & ĐÁM MÂY (Edge AI & Cloud)"]
        IPCam["IP Camera / Smartphone Stream"]
        RPi5["Raspberry Pi 5 (Edge AI)<br/>- Phân tích độ đục nước<br/>- Nhận diện cá bệnh / cá chết<br/>- Cơ chế Dự phòng Failover"]
        ThingsBoard["ThingsBoard Cloud (MQTT IoT Platform)<br/>- Telemetry 5s/lần<br/>- Điều khiển RPC từ xa<br/>- Cảnh báo Email khẩn cấp"]
    end

    %% Kết nối vật lý và truyền thông
    DS18B20 -->|"Đọc nhiệt độ nước"| ESPSlave
    DHT22 -->|"Đọc nhiệt/ẩm không khí"| ESPSlave
    HCSR04 -->|"Đo khoảng cách mực nước"| ESPSlave
    IR_REC -->|"Nhận tín hiệu phím bấm"| ESPSlave
    
    ESPSlave -->|"Điều khiển điện"| RELAYS
    ESPSlave -->|"Kéo góc thức ăn"| SERVO

    ESPSlave <==>|"Giao tiếp UART JSON (9600 baud)<br/>GPIO 16/17"| ESPMaster
    IPCam -->|"Luồng RTSP/HTTP Video"| RPi5
    RPi5 -.->|"Đồng bộ kết quả AI"| ESPMaster

    ESPMaster <==>|"WiFi / mDNS Local"| WebClient["Người dùng (Web Browser / Smartphone)"]
    ESPMaster <==>|"MQTT Protocol (Port 1883)"| ThingsBoard
    RPi5 -.->|"Failover Backup"| ThingsBoard
```

---

## 4. CÁC TÍNH NĂNG CỐT LÕI CHI TIẾT

### 4.1. Điều Khiển Nhiệt Độ Thông Minh (Thermostat)
- **Tự động Bật/Tắt Sưởi:** Bật sưởi khi nhiệt độ nước $< 18.0^\circ\text{C}$, Tắt sưởi khi nước $\ge 20.0^\circ\text{C}$ (các ngưỡng nhiệt độ tùy chỉnh linh hoạt từ $10^\circ\text{C} - 40^\circ\text{C}$).
- **Tự động Bật/Tắt Quạt Làm Mát:** Bật quạt khi nước $> 30.0^\circ\text{C}$, Tắt quạt khi nước $\le 28.0^\circ\text{C}$.
- **Độc lập 100% theo cảm biến nước DS18B20:** Loại bỏ hoàn toàn sự phụ thuộc sai lệch vào cảm biến môi trường không khí.
- **Failsafe Quá Nhiệt Tuyệt Đối:** Bất kể thiết bị được bật bằng tay, hẹn giờ hay tự động, khi nhiệt độ nước $\ge 35.0^\circ\text{C}$ $\rightarrow$ Hệ thống lập tức ngắt sưởi cưỡng bức để bảo vệ cá.

---

### 4.2. Quản Lý Mực Nước & Failsafe Cạn Nước (HC-SR04)
- **Tự động Bơm Bù Nước:** Khi mực nước tụt xuống ngưỡng thấp (`th_water_low`), hệ thống tự động kích hoạt Bơm Bù Nước và ngắt khi nước đầy (`th_water_full`) để chống tràn.
- **Failsafe Cạn Nước Nguy Hiểm (`is_empty`):** Khi mực nước vượt ngưỡng cạn (`th_water_empty`), Master lập tức ngắt khẩn cấp **Máy Sưởi**, **Bơm Rút Nước** và **Chế Độ Lọc Nước** để chống cháy thanh sưởi và cháy động cơ bơm.

---

### 4.3. Chế Độ Lọc Nước Song Song (Water Filter) & Chu Kỳ Tự Động
- **Lọc Nước Tuần Hoàn Đồng Thời:** Kích hoạt đồng thời cả 2 bơm (**Bơm Rút** và **Bơm Bù**) giúp luân chuyển nước qua hệ thống lọc liên tục.
- **Hẹn Giờ Đếm Lùi (Countdown Timer):** Hẹn giờ chạy lọc theo số Giờ - Phút - Giây (mặc định 15 phút), hết giờ tự động ngắt cả 2 bơm.
- **Chế Độ Chu Kỳ Lọc Nước Tự Động (Filter Cycle Mode):**
  - Tự động luân phiên **Chạy $X$ phút** (mặc định 15p) / **Nghỉ $Y$ phút** (mặc định 45p) liên tục 24/7.
  - Tùy chỉnh số phút Chạy và Nghỉ linh hoạt trên Web, lưu vĩnh viễn vào bộ nhớ Flash NVS.
  - Bấm phím **8** trên Remote IR hoặc bấm nút trên Web để chuyển đổi tức thì giữa chế độ thủ công và tự động.

---

### 4.4. Hệ Thống Sục Khí Oxy (Air Pump)
- **Chế độ Bật Liên Tục (Continuous):** Sục khí 24/7.
- **Chế độ Bật Theo Chu Kỳ (Cycle):** Luân phiên **5 phút BẬT / 15 phút TẮT** (tùy chỉnh số phút linh hoạt trên Web).
- **Phản hồi Remote IR thông minh:** Bấm phím **4** 1 lần $\rightarrow$ Bật/Tắt chu kỳ; Bấm phím **4** 3 lần liên tiếp trong 3s $\rightarrow$ Bật sủi liên tục.

---

### 4.5. Hệ Thống Cho Ăn Tự Động Với Góc Quay Tùy Chỉnh (Servo Feeder)
- **Hẹn giờ 3 mốc/ngày:** Định dạng chính xác `HH:MM:SS` (ví dụ: `08:00:00`, `12:00:00`, `18:00:00`).
- **Tùy Chỉnh Góc Quay Rotor Cho Ăn ($10^\circ - 180^\circ$):** Cho phép người dùng chỉnh góc mở Servo trên Web để định lượng thức ăn rơi ra chính xác cho từng loại cá.
- Khi kích hoạt (theo lịch, bấm Web, hoặc bấm phím **7** Remote): Servo quay đúng góc đã cài đặt, giữ 2 giây rồi tự động quay về $0^\circ$.

---

### 4.6. Chiếu Sáng Đèn LED & Hẹn Giờ Khung Giờ
- Bật/Tắt thủ công, hẹn giờ đếm lùi tự tắt, hoặc chạy tự động theo khung giờ trong ngày (ví dụ: Tự bật lúc `07:00` và tự tắt lúc `21:00`).

---

### 4.7. Công Tắc Tổng Khẩn Cấp (Kill Switch)
- Nút **`[HE THONG: BAT / TAT]`** trên Web Dashboard và phím **0** trên Remote IR.
- Khi Tắt Hệ Thống: **Lập tức ngắt 100% toàn bộ 7 Relay, Sục Oxy, Chế độ Lọc nước, Chu kỳ Lọc nước và khóa các timer**.

---

## 5. SƠ ĐỒ KẾT NỐI PHẦN CỨNG & PINOUT

### 5.1. ESP32-Master (Nút Gateway & Web Server)
| Chân ESP32-Master | Chức năng | Kết nối tới |
|---|---|---|
| **GPIO 16 (RX2)** | Nhận dữ liệu UART | Chân **TX2 (GPIO 17)** của ESP32-Slave |
| **GPIO 17 (TX2)** | Truyền dữ liệu UART | Chân **RX2 (GPIO 16)** của ESP32-Slave |
| **GPIO 0 (BOOT)** | Nút bấm vật lý | Giữ 3 giây để **Factory Reset Flash NVS** |
| **GPIO 2** | Đèn LED tích hợp | Báo trạng thái hoạt động |
| **GND** | Nối đất chung | Nối chung với GND của Slave và nguồn 5V |

---

### 5.2. ESP32-Slave (Nút Cảm Biến & Rơ-le Ngoại Vi)
| Chân ESP32-Slave | Thiết bị ngoại vi | Loại tín hiệu | Ghi chú |
|---|---|---|---|
| **GPIO 23** | Relay 1: Máy Sưởi | Output Digital (Active HIGH) | Đóng cắt nguồn điện sưởi |
| **GPIO 22** | Relay 2: Quạt Tản Nhiệt | Output Digital (Active HIGH) | Quạt làm mát bề mặt |
| **GPIO 19** | Relay 3: Bơm Bù Nước | Output Digital (Active HIGH) | Bơm nước sạch vào bể |
| **GPIO 21** | Relay 4: Máy Sục Oxy | Output Digital (Active HIGH) | Bơm khí oxy |
| **GPIO 26** | Relay 5: Bơm Rút Nước | Output Digital (Active HIGH) | Bơm hút xả nước cũ |
| **GPIO 27** | Relay 6: Đèn LED | Output Digital (Active HIGH) | Đèn chiếu sáng bể |
| **GPIO 13** | Động cơ Servo Cho Ăn | Output PWM | SG90 / MG90S (Góc $10^\circ - 180^\circ$) |
| **GPIO 4** | Mắt Thu Hồng Ngoại (IR) | Input Digital (38kHz) | MH-R38 (Giao thức NEC) |
| **GPIO 18** | Cảm biến Nhiệt Nước DS18B20 | Input 1-Wire Digital | Trở kéo 4.7kΩ lên 3.3V/5V (Non-blocking) |
| **GPIO 25** | Cảm biến Nhiệt Ẩm DHT22 | Input Single-bus Digital | Đo nhiệt độ & độ ẩm không khí |
| **GPIO 5** | Cảm biến Siêu Âm HC-SR04 (Trig) | Output Digital | Phát xung siêu âm 10µs |
| **GPIO 34** | Cảm biến Siêu Âm HC-SR04 (Echo) | Input Digital (Chỉ đọc) | Nhận sóng phản xạ đo mực nước |
| **GPIO 16 (RX2)** | Nhận lệnh UART | Input UART | Nối vào **TX2 (GPIO 17)** của Master |
| **GPIO 17 (TX2)** | Gửi Telemetry UART | Output UART | Nối vào **RX2 (GPIO 16)** của Master |

---

## 6. GIAO THỨC GIAO TIẾP MASTER - SLAVE (UART)

Tốc độ baud: **`9600 baud`**, định dạng dữ liệu: **JSON UTF-8**, kết thúc bằng ký tự xuống dòng `\n`.

### 6.1. Bản tin Dữ liệu Cảm biến & Trạng thái (Slave $\rightarrow$ Master)
Định kỳ gửi mỗi **2 giây/lần** (hoặc gửi ngay lập tức khi có sự kiện Remote IR):
```json
{
  "water_temp": 25.4,
  "air_temp": 28.2,
  "air_hum": 70.5,
  "water_cm": 15.3,
  "heater": 0,
  "fan": 0,
  "pump": 1,
  "oxy": 1,
  "drain": 1,
  "led": 0,
  "oxy_mode": 0,
  "last_ir": "15"
}
```

### 6.2. Bản tin Điều Khiển Relay & Thiết Bị (Master $\rightarrow$ Slave)
```json
{
  "cmd": "relay",
  "heater": false,
  "fan": false,
  "pump": true,
  "oxy": true,
  "drain": true,
  "led": false,
  "oxy_mode": false,
  "oo": 5,
  "of": 15,
  "fa": 180,
  "feed": false
}
```

### 6.3. Bản tin Đồng Bộ Bảng Mã Phím IR (Master $\rightarrow$ Slave)
```json
{
  "cmd": "ir_map",
  "ir1": "45",
  "ir2": "46",
  "ir3": "47",
  "ir4": "44",
  "ir5": "40",
  "ir6": "43",
  "ir7": "07",
  "ir8": "15",
  "ir0": "16"
}
```

### 6.4. Cơ Chế Heartbeat Giám Sát An Toàn (PING / PONG)
- **Slave gửi mỗi 15 giây:** `{"cmd":"ping"}`
- **Master phản hồi ngay lập tức:** `{"cmd":"pong"}`
- Nếu sau 15 giây Slave không nhận được phản hồi từ Master $\rightarrow$ Slave kích hoạt chế độ **`emergencyOff()`**, lập tức cắt nguồn toàn bộ Relay để đảm bảo an toàn tuyệt đối.

---

## 7. BẢNG MÃ ĐIỀU KHIỂN REMOTE HỒNG NGOẠI IR

Hệ thống sử dụng chuẩn giao thức **NEC (20 phím tiêu chuẩn)**. Người dùng có thể xem mã vừa bấm và gán lại mã phím tùy ý trong tab **CÀI ĐẶT** trên Web:

| Nút bấm Remote | Mã Hex mặc định | Chức năng điều khiển | Hành vi chi tiết |
|:---:|:---:|---|---|
| **1** | `0x45` | **Máy Sưởi** | Bật / Tắt máy sưởi thủ công |
| **2** | `0x46` | **Quạt Làm Mát** | Bật / Tắt quạt tản nhiệt thủ công |
| **3** | `0x47` | **Bơm Bù Nước** | Bật / Tắt bơm cấp nước sạch |
| **4** | `0x44` | **Sục Khí Oxy** | - **Bấm 1 lần:** Bật/Tắt chu kỳ (5p Bật / 15p Tắt)<br/>- **Bấm 3 lần liên tiếp:** Bật sục Oxy liên tục 24/7 |
| **5** | `0x40` | **Bơm Rút Nước** | Bật / Tắt bơm hút xả nước |
| **6** | `0x43` | **Đèn LED** | Bật / Tắt đèn chiếu sáng bể |
| **7** | `0x07` | **Cho Ăn (Servo)** | Kích hoạt quay Servo đến góc đã cài đặt ($10^\circ - 180^\circ$) trong 2 giây |
| **8** | `0x15` | **Chế Độ Lọc Nước** | Bật / Tắt đồng thời cả Bơm Rút & Bơm Bù để tuần hoàn lọc nước |
| **0** | `0x16` | **TẮT TẤT CẢ (Emergency)** | Ngắt toàn bộ 6 Relay và hủy tất cả chế độ ngay lập tức |

---

---

## 8. GIAO DIỆN WEB DASHBOARD & mDNS LOCAL

Hệ thống tích hợp máy chủ Web trực tiếp trên ESP32-Master với công nghệ Single Page Application (SPA), phong cách **Theme Trắng & Xanh Aqua (Aqua Ocean)** hiện đại, không cần kết nối Internet vẫn hoạt động đầy đủ 100%:

- **Tên miền truy cập cố định (mDNS):**
  - **`http://beca.local`** (hoặc `http://beca/` trên Windows).
  - Hoặc truy cập qua địa chỉ IP cục bộ được cấp phát bởi Router WiFi.

### Các Phân Hệ Trên Giao Diện Web:
1. **Tab BẢNG ĐIỀU KHIỂN (Dashboard):**
   - Giám sát các chỉ số cảm biến: Nhiệt độ nước, Nhiệt độ & Độ ẩm không khí, Mực nước (cm), Cường độ sóng WiFi (RSSI dBm), Đồng hồ thời gian thực NTP.
   - Nút bật/tắt thủ công cho từng thiết bị có đèn báo trạng thái `ON`/`OFF`.
   - Cụm Hẹn giờ đếm lùi độc lập (Giờ - Phút - Giây) cho Sưởi, Quạt, Bơm Rút, Lọc Nước và Đèn LED.
   - Nút kích hoạt **Chu Kỳ Lọc Nước Tự Động** (`[CHU KY: BAT / TAT]`).
   - Nút **Cho Ăn Tức Thì** và **Công tắc Tổng Hệ Thống (Kill Switch)**.
2. **Tab CÀI ĐẶT (Settings):**
   - Thiết lập ngưỡng nhiệt độ sưởi và quạt làm mát (tự động chuẩn hóa).
   - Thiết lập các mốc khoảng cách mực nước siêu âm (Đầy, Thấp, Cạn).
   - Cài đặt số phút Bật/Nghỉ của **Chu Kỳ Sục Oxy** và **Chu Kỳ Lọc Nước**.
   - Cài đặt khung giờ bật/tắt đèn LED tự động.
   - Cài đặt 3 mốc thời gian cho ăn tự động (`HH:MM:SS`) và **Góc Quay Rotor Cho Ăn ($10^\circ - 180^\circ$)**.
   - Cấu hình thông tin WiFi và Access Token kết nối **ThingsBoard MQTT**.
   - Bảng học mã và gán mã phím Remote IR linh hoạt.

---

## 9. HƯỚNG DẪN CÀI ĐẶT & NẠP FIRMWARE

### 9.1. Chuẩn bị Môi trường Lập trình
- Cài đặt **Arduino IDE (bản 1.8.19 hoặc 2.x)** hoặc **VS Code với PlatformIO**.
- Cài đặt ESP32 Board Package (bản `2.0.x` hoặc `3.x`).
- **Các thư viện cần thiết:**
  - `ArduinoJson` (v6.x)
  - `PubSubClient` (kết nối MQTT ThingsBoard)
  - `OneWire` & `DallasTemperature` (đo cảm biến DS18B20)
  - `DHT sensor library` (đo cảm biến DHT22)
  - `IRremote` (v4.x)
  - `ESP32Servo` (điều khiển Servo MG90S/SG90)
  - `ESPmDNS` & `Preferences` (có sẵn trong ESP32 Core)

### 9.2. Quy trình nạp Firmware
1. **Nạp Firmware cho ESP32-Master:**
   - Mở file [`Firmware/ESP32_Master/ESP32_Master.ino`](file:///g:/BECA/HE_THONG_BE_CA_SIC/Firmware/ESP32_Master/ESP32_Master.ino).
   - Chọn Board: `DOIT ESP32 DEVKIT V1` (hoặc `ESP32 Dev Module`).
   - Bấm **Upload**.
2. **Nạp Firmware cho ESP32-Slave:**
   - Mở file [`Firmware/ESP32_Slave/ESP32_Slave.ino`](file:///g:/BECA/HE_THONG_BE_CA_SIC/Firmware/ESP32_Slave/ESP32_Slave.ino).
   - Chọn Board: `DOIT ESP32 DEVKIT V1`.
   - Bấm **Upload**.
3. **Kết nối và Trải nghiệm:**
   - Bật nguồn cho cả 2 ESP32 và mở trình duyệt truy cập vào **`http://beca.local`** để bắt đầu sử dụng!

---

## 10. HƯỚNG DẪN TẠO DASHBOARD TRÊN THINGSBOARD CLOUD

Hệ thống hỗ trợ giám sát và điều khiển 2 chiều thời gian thực thông qua nền tảng **ThingsBoard Cloud** (hoặc ThingsBoard Community Server tự host). Dưới đây là quy trình từng bước thiết lập Dashboard hoàn chỉnh:

```mermaid
graph LR
    ESP[ESP32-Master] <==>|"MQTT Port 1883<br/>Telemetry & 2-Way RPC"| TB[ThingsBoard Cloud]
    TB <==>|"Web Dashboard / Mobile App"| User[Người Dùng]
```

### Bước 1: Tạo Thiết Bị (Device) Trên ThingsBoard
1. Đăng nhập vào [ThingsBoard](https://demo.thingsboard.io) (hoặc Server ThingsBoard của bạn).
2. Vào menu **Device Center** $\rightarrow$ **Devices** $\rightarrow$ bấm nút `+` (**Add Device**).
3. Đặt tên thiết bị: `ESP32_Master_BeCa` (Device profile: `default`).
4. Bấm **Add**. Sau khi tạo xong, mở chi tiết thiết bị $\rightarrow$ tab **Credentials** $\rightarrow$ Copy chuỗi **Access Token** (ví dụ: `A1b2C3d4E5f6G7h8`).

---

### Bước 2: Cấu Hình Kết Nối Trên ESP32-Master
1. Kết nối điện thoại hoặc máy tính vào cùng mạng WiFi với ESP32.
2. Mở trình duyệt truy cập vào **`http://beca.local`** (hoặc địa chỉ IP của ESP32).
3. Chuyển sang tab **CÀI ĐẶT HỆ THỐNG** $\rightarrow$ cuộn xuống mục **Kết Nối ThingsBoard Cloud (MQTT)**:
   - **Kích hoạt MQTT:** Chọn `BẬT`
   - **MQTT Server:** Điền `demo.thingsboard.io` (hoặc tên miền / IP server ThingsBoard của bạn)
   - **Access Token:** Dán chuỗi Token đã copy ở Bước 1.
4. Bấm **💾 LƯU TOÀN BỘ CÀI ĐẶT**.
5. Sau 2-3 giây, trạng thái sẽ báo **ONLINE** màu xanh lá. Vào tab **Latest Telemetry** của thiết bị trên ThingsBoard để thấy các dữ liệu cảm biến nhảy liên tục!

---

### Bước 3: Tạo Dashboard & Thiết Lập Entity Alias
1. Vào menu **Dashboard Center** $\rightarrow$ **Dashboards** $\rightarrow$ bấm nút `+` (**Create new dashboard**).
2. Đặt tiêu đề: `GIÁM SÁT & ĐIỀU KHIỂN BỂ CÁ THÔNG MINH`.
3. Mở Dashboard vừa tạo $\rightarrow$ bấm nút **Edit Mode** (biểu tượng cây bút góc dưới bên phải).
4. Bấm biểu tượng **Entity Aliases** (trên thanh công cụ trên cùng) $\rightarrow$ **Add Alias**:
   - **Alias name:** `BeCa`
   - **Filter type:** `Single entity`
   - **Type:** `Device`
   - **Device:** Chọn `ESP32_Master_BeCa`
5. Bấm **Add** và **Save**.

---

### Bước 4: Tạo Các Widget Giám Sát (Latest Telemetry)

Bấm **Add new widget** $\rightarrow$ chọn **Datasource:** Alias `BeCa`:

| Nhóm Widget (Bundle) | Tên Widget Khuyên Dùng | Keys Telemetry | Nhãn & Đơn Vị Hiển Thị |
|---|---|---|---|
| **Cards** | *Simple Card* hoặc *Value Card* | `water_temp` | Nhiệt Độ Nước (°C) |
| **Cards** | *Simple Card* | `air_temp`, `air_hum` | Nhiệt Độ (°C) & Độ Ẩm Không Khí (%) |
| **Cards** | *Simple Card* | `water_cm` | Mực Nước (cm cách nắp bể) |
| **Cards** | *Simple Card* | `rssi` | Cường Độ Sóng WiFi (dBm) |
| **Analogue gauges** | *Radial gauge* hoặc *Digital gauge* | `water_temp` (Min: 10, Max: 40) | Đồng hồ nhiệt độ nước |
| **Analogue gauges** | *Radial gauge* | `water_cm` (Min: 0, Max: 50) | Thước đo khoảng cách mực nước |
| **Charts** | *Timeseries chart* | `water_temp`, `air_temp`, `air_hum`, `water_cm` | Biểu đồ lịch sử biến thiên 24h |

---

### Bước 5: Tạo Các Widget Điều Khiển 2 Chiều (Control RPC)

Vào thư viện **Control widgets** $\rightarrow$ chọn các widget công tắc / nút bấm:

| Tên Thiết Bị | Loại Widget | Method RPC (`method`) | Key Trạng Thái (Target Key) | Mô Tả Chức Năng |
|---|---|---|---|---|
| **Máy Sưởi** | Switch control / Round switch | `setHeater` | `heater` | Bật / Tắt thanh sưởi |
| **Quạt Làm Mát** | Switch control | `setFan` | `fan` | Bật / Tắt quạt tản nhiệt |
| **Bơm Bù Nước** | Switch control | `setPump` | `pump` | Bật / Tắt bơm cấp nước sạch |
| **Sục Khí Oxy** | Switch control | `setOxy` | `oxy` | Bật / Tắt máy sục bọt khí oxy |
| **Bơm Rút Nước** | Switch control | `setDrain` | `drain` | Bật / Tắt bơm xả nước cũ |
| **Lọc Nước Song Song**| Switch control | `setFilter` | `filter` | Bật / Tắt tuần hoàn lọc 2 bơm |
| **Chu Kỳ Lọc Tự Động**| Switch control | `setFilterCycle` | `filter_cycle` | Bật / Tắt chu kỳ chạy/nghỉ tự động |
| **Đèn LED Bể Cá** | Switch control / Light switch | `setLed` | `led` | Bật / Tắt đèn chiếu sáng |
| **Nút Cho Ăn Tức Thì**| Action Button / Push button | `setFeed` (params: `true`)| N/A | Kích hoạt quay Servo rót thức ăn |
| **Công Tắc Tổng Khẩn Cấp**| Switch control / Power switch | `setSystem` | `system` | Khóa / Tắt khẩn cấp toàn bộ hệ thống |

> 💡 **Lưu ý quan trọng về RPC:** Firmware ESP32-Master đã được tích hợp sẵn cơ chế **2-Way RPC Response** (`v1/devices/me/rpc/response/{requestId}`). Khi bạn gạt công tắc hoặc bấm nút trên ThingsBoard, ESP32 sẽ lập tức gửi xác nhận thành công về Cloud, giúp giao diện widget cập nhật tức thì trong vòng $< 50\text{ms}$ mà không bao giờ bị báo lỗi timeout!

---

### Bước 6: Lưu Và Sử Dụng
1. Bấm nút **Apply changes** (dấu tích màu cam) để lưu giao diện Dashboard.
2. Bây giờ bạn có thể mở ThingsBoard trên trình duyệt máy tính hoặc tải ứng dụng **ThingsBoard Mobile App** (iOS / Android) đăng nhập vào để theo dõi và chăm sóc bể cá của mình từ bất kỳ đâu trên thế giới! 🌐🐟

---

## 11. HƯỚNG DẪN CÀI ĐẶT & VẬN HÀNH PYTHON SERVER (EDGE AI & IOT GATEWAY)

Python Server được thiết kế để chạy trên **Raspberry Pi 5** (hoặc máy tính mini / PC Windows / Linux) với các tính năng:
- **Trí tuệ nhân tạo Đa phương thức Gemini Flash Vision:** Phân tích ảnh từ camera theo thời gian thực (đếm số cá sống/chết, phát hiện cá lờ đờ, đo độ đục nước).
- **Tự động mở Cloudflare Tunnel:** Sinh đường dẫn Public URL (`https://*.trycloudflare.com`) truy cập Web từ xa và tự động gửi link vào Telegram.
- **Giao diện Web Hợp nhất 2 Chế Độ:**
  - *Chế độ 1 (LAN `beca.local`):* Gọi trực tiếp REST API của ESP32 qua mạng nội bộ để đọc cảm biến và điều khiển 6 Relay siêu nhanh $< 5\text{ms}$.
  - *Chế độ 2 (Cloud ThingsBoard):* Nhúng Dashboard ThingsBoard để giám sát từ xa qua Internet.
- **Telegram Chatbot 2 Chiều:** Tự động gửi ảnh snapshot cảnh báo khẩn cấp khi có sự cố và tương tác lệnh (`/status`, `/snapshot`, `/data`, `/feed`, `/url`).
- **Đẩy Telemetry AI lên Device riêng trên ThingsBoard:** Tách biệt hoàn toàn với ESP32.

---

### 11.1. Logic Chụp Ảnh & Cơ Chế Xác Thực Cá Chết 5 Lần Liên Tiếp (5-Sample Anti-False-Alarm Filter)

```mermaid
graph TD
    CAM["Camera (Webcam USB / IP Camera RTSP)"] -->|"Thread đọc non-blocking 15 FPS"| BUF["Bộ đệm khung hình (Video Frame Buffer)"]
    
    BUF -->|"1. Chu kỳ bình thường: Mỗi 2 phút chụp 1 ảnh"| GEMINI["Mô hình Gemini Flash Vision VLM"]
    
    GEMINI -->|"Phát hiện nghi ngờ dead_fish > 0"| VERIFY["Kích Hoạt Chu Kỳ Xác Thực (Mỗi 10s chụp 1 lần)"]
    VERIFY -->|"Bộ đếm Streak: 1..5 lần"| CHECK{"Cả 5 lần liên tiếp<br/>đều có cá chết?"}
    
    CHECK -->|"CÓ (Đạt 5/5 lần)"| CONFIRM["XÁC NHẬN CÁ CHẾT 100%:<br/>- Trừ số cá sống: Sống = Tổng - Chết<br/>- Gửi ảnh bằng chứng + Cảnh báo Telegram<br/>- Đẩy Telemetry lên ThingsBoard AI"]
    CHECK -->|"KHÔNG (Cá bơi lại bình thường)"| RESET["HỦY BÁO ĐỘNG GIẢ:<br/>- Reset Streak về 0/5<br/>- Quay lại chu kỳ bình thường 2 phút/lần"]
    
    BUF -->|"2. Người dùng bấm nút trên Web hoặc gõ /snapshot"| INSTANT["Chụp & Phân Tích AI Tức Thì (< 2s)"]
    BUF -->|"3. Luồng liên tục 15 FPS"| MJPEG["MJPEG Video Stream trên Web<br/>http://<ip-pi5>:5000/video_feed"]
```

#### Chi tiết các luồng xử lý:
1. **Chu kỳ lấy mẫu bình thường (Mỗi 2 phút / 120s):**
   - Chụp 1 snapshot nén JPEG $640 \times 480$ gửi sang Gemini Flash Vision.
2. **Cơ chế xác thực 5 lần liên tiếp chống báo động giả (5 Consecutive Positives):**
   - Khi có nghi ngờ cá chết, hệ thống **không báo động ngay** mà chuyển sang chế độ xác thực **chụp liên tục mỗi 10 giây**.
   - Nếu **cả 5 lần chụp liên tiếp** đều phát hiện cá chết $\rightarrow$ Xác nhận chính xác cá chết, cập nhật số cá sống: $\text{Số Cá Sống} = \text{Tổng Cá Thả} - \text{Số Cá Chết}$, đồng thời gửi ảnh snapshot bằng chứng và cảnh báo khẩn cấp qua Telegram!
   - Nếu trong 5 lần đó cá bơi lại bình thường $\rightarrow$ Reset bộ đếm xác thực về 0, quay lại chu kỳ 2 phút/lần (tránh báo động giả).
3. **Nút chụp tức thì trên Web & Lệnh Telegram:**
   - Người dùng bấm nút **📸 CHỤP ẢNH & PHÂN TÍCH AI NGAY** trên Web Dashboard hoặc gõ `/status` trên Telegram để chụp và nhận kết quả tức thì trong $< 2\text{s}$.
4. **Luồng phát Video trực tiếp MJPEG:**
   - Duy trì liên tục tại endpoint `/video_feed` cho Web Dashboard.

---

### 11.2. Cài đặt Môi trường & Thư viện
```bash
cd Python_Server
pip install -r requirements.txt
```

### 11.3. Cấu hình hệ thống (`config.yaml`)
Mở file [`Python_Server/config.yaml`](file:///g:/BECA/HE_THONG_BE_CA_SIC/Python_Server/config.yaml) và điền thông tin:
```yaml
camera:
  source: 0 # 0 là Webcam USB, hoặc chuỗi RTSP IP Camera: "rtsp://admin:123456@192.168.1.100:554/stream1"
  fps: 15
  analyze_interval_sec: 10

aquarium:
  total_fish: 10 # Tổng số cá thả trong bể (có thể chỉnh trực tiếp trên Web)

gemini:
  enabled: true
  api_key: "YOUR_GEMINI_API_KEY" # Lấy API Key miễn phí tại: https://aistudio.google.com/
  model: "gemini-2.0-flash"

telegram:
  enabled: true
  bot_token: "YOUR_TELEGRAM_BOT_TOKEN" # Lấy token từ @BotFather
  chat_id: "YOUR_TELEGRAM_CHAT_ID"     # Lấy ID từ @userinfobot (có thể sửa trên Web)
  cooldown_minutes: 10

thingsboard:
  enabled: true
  host: "demo.thingsboard.io"
  port: 1883
  access_token: "YOUR_RPI5_AI_DEVICE_TOKEN" # Token của Device AI riêng trên ThingsBoard

esp32_lan:
  enabled: true
  base_url: "http://beca.local"

web:
  host: "0.0.0.0"
  port: 5000
```

### 11.4. Khởi chạy Server
```bash
python run_server.py
```
* **Tự động mở Cloudflare Tunnel:** Ngay khi khởi động, hệ thống tự động sinh Public URL (`https://*.trycloudflare.com`) và gửi thẳng vào Telegram của bạn!
* **Truy cập Web Dashboard:** Mở trình duyệt vào **`http://localhost:5000`** (hoặc đường link Cloudflare).
* **Trải nghiệm Telegram Bot:** Gõ lệnh `/status`, `/snapshot`, `/data`, `/feed`, `/url` để nhận báo cáo và ảnh chụp tức thì.



