from flask import Flask, request, jsonify
from ultralytics import YOLO
import cv2
import numpy as np
import os
import datetime
import random
from collections import Counter

app = Flask(__name__)

# --- CẤU HÌNH ---
try:
    model = YOLO("best.pt")
    print("--> TAI MO HINH THANH CONG!")
except:
    print("--> LOI: Khong tim thay best.pt")
    exit()

SAVE_DIR = "detected_images"
if not os.path.exists(SAVE_DIR): os.makedirs(SAVE_DIR)

# --- BIẾN BỘ NHỚ ---
BATCH_SIZE = 5
image_buffer = []
current_count = 0

# Biến dành riêng cho ESP8266 (Loa)
audio_state = {
    "batch_id": 0,      # ID để biết là kết quả mới
    "object": "none"    # Tên vật thể để phát loa
}
batch_counter = 0

# ================= API 1: CHO ESP32 (GỬI ẢNH - NHẬN TEXT LCD) =================
@app.route("/detect", methods=["POST"])
def detect():
    global image_buffer, current_count, audio_state, batch_counter

    try:
        img_bytes = request.data
        if not img_bytes: return "scanning", 200 # Trả về scanning để hiện LCD
        nparr = np.frombuffer(img_bytes, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        if frame is None: return "error", 200
    except:
        return "error", 200

    # Chạy AI
    results = model.predict(frame, conf=0.4, verbose=False)
    
    detected = []
    if results[0].boxes is not None:
        for box in results[0].boxes:
            cls_id = int(box.cls[0])
            detected.append(model.names[cls_id])
    detected = list(set(detected))

    image_buffer.append({"image": frame, "labels": detected})
    current_count += 1
    
    print(f"--> [Batch {batch_counter}] Anh {current_count}/{BATCH_SIZE}: {detected}")

    # === XỬ LÝ KHI ĐỦ 5 ẢNH ===
    if current_count >= BATCH_SIZE:
        all_labels = []
        for item in image_buffer: all_labels.extend(item["labels"])

        winner = "none"
        if len(all_labels) > 0:
            winner = Counter(all_labels).most_common(1)[0][0]
            
            # Lưu ảnh
            candidates = [x["image"] for x in image_buffer if winner in x["labels"]]
            if candidates:
                lucky_frame = random.choice(candidates)
                fname = f"{winner}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
                cv2.imwrite(os.path.join(SAVE_DIR, fname), lucky_frame)
                print(f"--> DA LUU: {fname}")

        # CẬP NHẬT CHO ESP8266 (LOA)
        batch_counter += 1
        audio_state = {
            "batch_id": batch_counter,
            "object": winner
        }
        
        print(f"--> CHOT KET QUA: {winner}")
        
        # Reset
        image_buffer = []
        current_count = 0
        
        # TRẢ VỀ CHO ESP32 ĐỂ HIỆN LCD NGAY
        return winner, 200

    # Nếu chưa đủ 5 ảnh, trả về scanning
    return "scanning", 200

# ================= API 2: CHO ESP8266 (HỎI ĐỂ PHÁT LOA) =================
@app.route("/audio", methods=["GET"])
def get_audio():
    return jsonify(audio_state), 200

if __name__ == "__main__":
    # Thay IP bằng IP máy bạn
    app.run(host="0.0.0.0", port=5000, threaded=True, debug=False)

//chuoi