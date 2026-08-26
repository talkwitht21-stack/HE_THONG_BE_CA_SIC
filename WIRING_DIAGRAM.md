# SƠ ĐỒ ĐẤU NỐI (WIRING DIAGRAM)

Tài liệu hướng dẫn kết nối phần cứng cho hệ thống Bể Cá Thông Minh. Hệ thống sử dụng 2 vi điều khiển ESP32 giao tiếp với nhau qua UART.

---

## 1. ESP32-Slave (Module Cảm Biến & Điều Khiển Thực Thi)
Bảng GPIO này sử dụng **13 chân** trên ESP32. Đảm bảo nguồn cấp đủ công suất (khuyên dùng nguồn 5V/3A) do có nhiều Relay.

### Nguồn cấp (Power)
| ESP32 Pin | Kết nối tới | Ghi chú |
|:---:|:---|:---|
| `VIN` / `5V` | Nguồn 5V DC chung | Cấp nguồn nuôi ESP32 và module |
| `GND` | GND chung | Tất cả GND của module cảm biến, relay phải nối chung về đây |

### Cảm biến (Sensors)
| ESP32 Pin | Tên Cảm Biến | Kết nối chân | Ghi chú |
|:---:|:---|:---|:---|
| `GPIO 4` | **DS18B20** (Nước) | Data (DQ) | Cần trở kéo **4.7kΩ** nối giữa Data và 3.3V |
| `GPIO 5` | **DHT22** (Không khí) | Data | |
| `GPIO 33` | **Phao từ** (Mực nước) | Tín hiệu | Chân còn lại của phao từ nối xuống GND (INPUT_PULLUP) |
| `GPIO 14` | **HC-SR04** (Siêu âm) | Trig | Phát sóng siêu âm đo khoảng cách |
| `GPIO 32` | **HC-SR04** (Siêu âm) | Echo | Thu sóng phản xạ |
| `GPIO 15` | **Mắt thu IR** (MH-R38) | OUT / Y | Chân VCC nối 3.3V, GND nối GND |

### Cơ cấu chấp hành (Actuators - Relay Active HIGH)
| ESP32 Pin | Thiết bị | Module | Ghi chú |
|:---:|:---|:---|:---|
| `GPIO 25` | **Thanh sưởi** | Relay Kênh 1 | |
| `GPIO 26` | **Quạt tản nhiệt** | Relay Kênh 2 | |
| `GPIO 27` | **Bơm bù nước** | Relay Kênh 3 | Bật tự động khi phao báo cạn |
| `GPIO 12` | **Sục oxy** | Relay Kênh 4 | Bơm khí sủi bọt |
| `GPIO 23` | **Bơm thay nước** | Relay Kênh 5 | Hút nước cũ ra ngoài |
| `GPIO 19` | **Đèn LED** | Relay Kênh 6 | Chiếu sáng bể cá |
| `GPIO 13` | **Động cơ Servo** | Dây Tín hiệu (Cam) | Dây VCC (Đỏ) nối 5V, GND (Nâu) nối GND |

### Giao tiếp UART (Nối chéo sang Master)
| ESP32-Slave | ESP32-Master | Ghi chú |
|:---:|:---:|:---|
| `TX2 (GPIO 17)` | `RX2 (GPIO 16)` | Gửi dữ liệu cảm biến & trạng thái lên Master |
| `RX2 (GPIO 16)` | `TX2 (GPIO 17)` | Nhận lệnh điều khiển relay từ Master |
| `GND` | `GND` | **BẮT BUỘC** nối chung GND giữa 2 ESP32 |

---

## 2. ESP32-Master (Module Logic & Internet)
Module này chỉ đóng vai trò giao tiếp Internet, phát WiFi, tạo Web Server, và tính toán logic. Nó **không** kết nối trực tiếp với cảm biến hay relay nào (trừ UART).

| ESP32 Pin | Kết nối tới | Ghi chú |
|:---:|:---|:---|
| `VIN` / `5V` | Nguồn 5V DC chung | |
| `GND` | GND chung | |
| `TX2 (GPIO 17)` | `RX2 (GPIO 16)` của Slave | Gửi lệnh xuống Slave |
| `RX2 (GPIO 16)` | `TX2 (GPIO 17)` của Slave | Nhận dữ liệu từ Slave |

---

## 3. Remote Hồng Ngoại (Mapping)
Sử dụng Remote NEC 20/21 phím phổ biến (loại thường tặng kèm Arduino Starter Kit). Khi bấm, mắt thu MH-R38 (chân 15 trên Slave) sẽ nhận và giải mã.

| Phím trên Remote | Chức năng (Slave tự xử lý) |
|:---:|:---|
| `1` | Bật / Tắt Sưởi |
| `2` | Bật / Tắt Quạt |
| `3` | Bật / Tắt Bơm bù nước |
| `4` | Nhấn 1 lần: Sục oxy chu kỳ (5p On/15p Off). Nhấn 3 lần/3s: Liên tục |
| `5` | Bật / Tắt Bơm thay nước |
| `6` | Bật / Tắt Đèn LED |
| `7` | Cho ăn (Quay Servo) |
| `0` | Khẩn cấp: Tắt TẤT CẢ các thiết bị |

---
*Lưu ý: Mọi cấu hình logic nâng cao (Timer, ngưỡng nhiệt độ, lịch đèn) được cài đặt thông qua giao diện Web của ESP32-Master khi kết nối vào mạng WiFi `BeCa_Control`.*
