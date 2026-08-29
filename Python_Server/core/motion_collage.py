import cv2
import numpy as np
import time

def resize_to_20percent_pixels(frame, target_pixel_ratio=0.20):
    """
    Nén kích thước ảnh sao cho tổng số pixel chỉ còn ~20% so với ảnh gốc.
    Tỷ lệ thu nhỏ cạnh s = sqrt(0.20) ~= 0.447 (~45%).
    """
    if frame is None:
        return None
    h, w = frame.shape[:2]
    scale = float(np.sqrt(target_pixel_ratio))
    new_w = max(64, int(w * scale))
    new_h = max(48, int(h * scale))
    return cv2.resize(frame, (new_w, new_h), interpolation=cv2.INTER_AREA)

def create_5frame_motion_collage(frames_list, intervals_sec=1.0):
    """
    Nhận vào danh sách 5 frames [f1, f2, f3, f4, f5].
    1. Nén mỗi frame xuống còn ~20% số pixel (scale ~45%).
    2. Vẽ viền phân cách dày 4px màu vàng/xanh neon tương phản cao.
    3. Vẽ Banner dán nhãn Frame 1..5 kèm mốc thời gian (+0.0s, +1.0s, ...).
    4. Ghép 5 frame thành 1 ảnh Collage duy nhất có Header hướng dẫn trực quan.
    
    Returns:
        collage_img: Ảnh OpenCV BGR hoàn chỉnh
        collage_jpeg_bytes: Dữ liệu JPEG của ảnh collage
        individual_jpegs: Danh sách 5 ảnh JPEG bytes riêng lẻ để hiển thị trên Web
    """
    if not frames_list or len(frames_list) == 0:
        return None, None, []

    processed_list = list(frames_list)
    while len(processed_list) < 5:
        processed_list.append(processed_list[-1].copy() if processed_list else np.zeros((240, 320, 3), dtype=np.uint8))
    processed_list = processed_list[:5]

    individual_jpegs = []
    annotated_frames = []

    BORDER_COLOR = (255, 200, 0) # Vàng cam BGR
    HEADER_BG_COLOR = (20, 30, 45) # Xanh đen

    for i, raw_frame in enumerate(processed_list):
        # 1. Resize xuống 20% pixel
        small_frame = resize_to_20percent_pixels(raw_frame, target_pixel_ratio=0.20)
        h, w = small_frame.shape[:2]

        # 2. Tạo viền phân cách 4px và thanh nhãn 24px
        bordered = cv2.copyMakeBorder(
            small_frame, 
            top=24, bottom=4, left=4, right=4, 
            borderType=cv2.BORDER_CONSTANT, 
            value=BORDER_COLOR
        )
        bh, bw = bordered.shape[:2]

        # 3. Vẽ nhãn Frame và mốc thời gian
        t_offset = float(i) * float(intervals_sec)
        label_text = f"FRAME {i+1}/5 (+{t_offset:.1f}s)"
        
        cv2.rectangle(bordered, (0, 0), (bw, 22), (30, 41, 59), -1)
        cv2.putText(
            bordered, label_text, (6, 16), 
            cv2.FONT_HERSHEY_SIMPLEX, 0.42, (255, 255, 255), 1, cv2.LINE_AA
        )

        annotated_frames.append(bordered)

        # Lưu ảnh JPEG riêng lẻ để Web hiển thị
        ret, buf = cv2.imencode('.jpg', bordered, [cv2.IMWRITE_JPEG_QUALITY, 85])
        if ret:
            individual_jpegs.append(buf.tobytes())

    # 4. Ghép 5 frame thành dải ngang
    combined_strip = np.hstack(annotated_frames)
    strip_h, strip_w = combined_strip.shape[:2]

    # 5. Tạo Banner tiêu đề phía trên cho toàn bộ ảnh Collage (Hint cho AI)
    header_h = 42
    header_canvas = np.zeros((header_h, strip_w, 3), dtype=np.uint8)
    header_canvas[:] = HEADER_BG_COLOR

    title_text = "CHUOI 5 KHUNG HINH THEO DONG THOI GIAN (TEMPORAL MOTION ANALYSIS) - BE CA SIC"
    sub_text = "AI HINT: So sanh vi tri/vay cua tung con ca giua Frame 1->5: Co chuyen dong = SONG | Bat dong 100% ca 5 frame = CHET"

    cv2.putText(header_canvas, title_text, (10, 18), cv2.FONT_HERSHEY_SIMPLEX, 0.40, (56, 189, 248), 1, cv2.LINE_AA)
    cv2.putText(header_canvas, sub_text, (10, 34), cv2.FONT_HERSHEY_SIMPLEX, 0.30, (148, 163, 184), 1, cv2.LINE_AA)

    final_collage = np.vstack([header_canvas, combined_strip])

    ret, collage_buf = cv2.imencode('.jpg', final_collage, [cv2.IMWRITE_JPEG_QUALITY, 85])
    collage_jpeg_bytes = collage_buf.tobytes() if ret else None

    return final_collage, collage_jpeg_bytes, individual_jpegs
