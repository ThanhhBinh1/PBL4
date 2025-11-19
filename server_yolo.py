from flask import Flask, request
from ultralytics import YOLO
import cv2
import numpy as np
import os
import datetime
import random
from collections import Counter
import io

app = Flask(__name__)

# --- CẤU HÌNH ---
try:
    model = YOLO("best.pt")
    print("--> TAI MO HINH THANH CONG!")
except Exception as e:
    print(f"--> LOI: Khong tim thay file 'best.pt'. {e}")
    exit()

SAVE_DIR = "detected_images"
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)

# --- BIẾN BỘ NHỚ ĐỆM ---
BATCH_SIZE = 10
image_buffer = []
current_count = 0

@app.route("/detect", methods=["POST"])
def detect():
    global image_buffer, current_count
    try:
        img_bytes = request.data
        if not img_bytes:
            print("--> Loi: Nhan duoc request nhung khong co du lieu anh")
            return "EMPTY_DATA", 400

        # Chuyển đổi bytes thành ảnh OpenCV
        nparr = np.frombuffer(img_bytes, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if frame is None:
            print("--> Loi: Khong giai ma duoc anh (Decode Failed)")
            return "DECODE_ERROR", 400
            
    except Exception as e:
        print(f"--> Loi Server: {e}")
        return "SERVER_ERROR", 500
    # ================================

    # Chạy AI
    results = model.predict(frame, conf=0.5, verbose=False)
    
    detected_labels = []
    if results[0].boxes is not None:
        for box in results[0].boxes:
            cls_id = int(box.cls[0])
            detected_labels.append(model.names[cls_id])
    
    detected_labels = list(set(detected_labels))

    # Lưu vào bộ đệm
    image_buffer.append({
        "image": frame,
        "labels": detected_labels
    })
    current_count += 1

    # In ra terminal
    labels_str = ", ".join(detected_labels) if detected_labels else "None"
    print(f"--> [Anh {current_count}/{BATCH_SIZE}] Phat hien: {labels_str}")

    # Kiểm tra đủ 10 ảnh chưa
    if current_count < BATCH_SIZE:
        return "scanning", 200
    else:
        print("---------------------------------------------")
        print("--> DA DU 10 ANH. DANG PHAN TICH...")

        all_labels = []
        for item in image_buffer:
            all_labels.extend(item["labels"])

        final_result = "none"

        if len(all_labels) > 0:
            # Tìm cái xuất hiện nhiều nhất
            most_common = Counter(all_labels).most_common(1)
            winner_label = most_common[0][0]
            frequency = most_common[0][1]
            
            print(f"--> CHOT KET QUA: '{winner_label}' (Tan suat {frequency}/{BATCH_SIZE})")
            
            # Lưu 1 ảnh ngẫu nhiên
            candidates = [item["image"] for item in image_buffer if winner_label in item["labels"]]
            if len(candidates) > 0:
                lucky_frame = random.choice(candidates)
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"{winner_label}_{timestamp}.jpg"
                save_path = os.path.join(SAVE_DIR, filename)
                cv2.imwrite(save_path, lucky_frame)
                print(f"--> DA LUU ANH: {filename}")
            
            final_result = winner_label
        else:
            print("--> CHOT KET QUA: Khong co vat the.")

        # Reset
        image_buffer = []
        current_count = 0
        print("=============================================")
        
        return final_result, 200

if __name__ == "__main__":
    print("SERVER DA SUA LOI HTTP 400 - SAN SANG!")
    # Nhớ thay IP này bằng IP máy bạn nếu nó thay đổi
    app.run(host="0.0.0.0", port=5000, threaded=True, debug=False)