import cv2
import time
import json
import random
import paho.mqtt.client as mqtt
import smtplib
from email.message import EmailMessage

# --- Cấu hình ThingsBoard ---
THINGSBOARD_HOST = 'demo.thingsboard.io'
ACCESS_TOKEN = 'YOUR_PI5_ACCESS_TOKEN'

# --- Cấu hình Email Cảnh báo ---
EMAIL_SENDER = "your_email@gmail.com"
EMAIL_PASSWORD = "your_app_password"
EMAIL_RECEIVER = "admin_email@gmail.com"

# --- Khởi tạo MQTT Client ---
client = mqtt.Client()
client.username_pw_set(ACCESS_TOKEN)

def connect_mqtt():
    try:
        client.connect(THINGSBOARD_HOST, 1883, 60)
        client.loop_start()
        print("Đã kết nối ThingsBoard (MQTT)")
    except Exception as e:
        print(f"Lỗi kết nối MQTT: {e}")

def send_alert_email(subject, body):
    try:
        msg = EmailMessage()
        msg.set_content(body)
        msg['Subject'] = subject
        msg['From'] = EMAIL_SENDER
        msg['To'] = EMAIL_RECEIVER

        server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        server.login(EMAIL_SENDER, EMAIL_PASSWORD)
        server.send_message(msg)
        server.quit()
        print(f" Đã gửi Email Cảnh báo: {subject}")
    except Exception as e:
        print(f" Lỗi gửi email: {e}")

def analyze_frame(frame):
    # Mock hàm AI YOLO - Trong thực tế sẽ chạy inference YOLO ở đây
    # Trả về: (số lượng cá, số lượng bơi lờ đờ, số lượng cá chết, độ đục của nước)
    
    # Random demo dữ liệu AI
    total_fish = 10
    dead_fish = 1 if random.random() > 0.95 else 0 # 5% rủi ro có cá chết
    abnormal_fish = random.randint(0, 2)
    turbidity = random.uniform(10.0, 50.0) # NTU
    
    return total_fish, dead_fish, abnormal_fish, turbidity

def main():
    connect_mqtt()
    
    # Khởi động Camera (IP Cam hoặc USB WebCam)
    # cap = cv2.VideoCapture("rtsp://admin:123456@192.168.1.100:554/stream1") 
    cap = cv2.VideoCapture(0) # Dùng Webcam làm demo
    
    last_process_time = time.time()
    
    while True:
        ret, frame = cap.read()
        if not ret:
            print("Không thể đọc từ Camera")
            time.sleep(5)
            continue
            
        current_time = time.time()
        
        # Phân tích mỗi 5 giây (để tối ưu hiệu năng Edge AI)
        if current_time - last_process_time >= 5.0:
            last_process_time = current_time
            
            total, dead, abnormal, turbid = analyze_frame(frame)
            
            # 1. Đẩy Telemetry lên ThingsBoard
            telemetry = {
                "ai_total_fish": total,
                "ai_dead_fish": dead,
                "ai_abnormal_fish": abnormal,
                "ai_water_turbidity": round(turbid, 2)
            }
            client.publish('v1/devices/me/telemetry', json.dumps(telemetry), 1)
            print(f" AI Data Sent: {telemetry}")
            
            # 2. Xử lý Cảnh báo Khẩn cấp
            if dead > 0:
                print(" PHÁT HIỆN CÁ CHẾT! Kích hoạt gửi Email.")
                # TODO: Lưu ảnh frame ra file để đính kèm email
                send_alert_email(
                    "CẢNH BÁO TỪ BỂ CÁ",
                    f"Hệ thống AI phát hiện có {dead} cá thể lật ngửa/tử vong.\nĐộ đục: {round(turbid,2)} NTU.\nVui lòng kiểm tra bể ngay!"
                )
            elif turbid > 40.0:
                print(" NƯỚC RẤT ĐỤC! Kích hoạt gửi Email.")
                send_alert_email(
                    "CẢNH BÁO TỪ BỂ CÁ",
                    f"Nước trong bể đang rất đục (Độ đục: {round(turbid,2)} NTU).\nCó thể hệ thống lọc đang gặp sự cố."
                )

        # Hiển thị (Debug)
        cv2.putText(frame, f"Fish: {total} | Dead: {dead}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2)
        cv2.imshow("Edge AI Monitoring", frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    client.loop_stop()

if __name__ == "__main__":
    print("Khởi động Raspberry Pi 5 AI Edge Node...")
    main()
