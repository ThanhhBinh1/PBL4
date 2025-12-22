from flask import Flask, request, jsonify, send_file, Response
from ultralytics import YOLO
import cv2
import numpy as np
import os
import datetime
import random
import time
from collections import Counter
import logging

# Tắt log rác
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)

# --- CẤU HÌNH ---
print("--> DANG TAI MODEL YOLO...")
try:
    model = YOLO("best.pt") 
    print("--> TAI MO HINH THANH CONG!")
except Exception as e:
    print(f"--> LOI: {e}")

SAVE_DIR = "detected_images"
if not os.path.exists(SAVE_DIR): os.makedirs(SAVE_DIR)

# --- BIẾN LOGIC ---
BATCH_SIZE = 7  # Yêu cầu: 7 ảnh có nhãn
image_buffer = []
current_count = 0
batch_counter = 0

# Biến chia sẻ trạng thái
global_frame = None 
audio_state = {"batch_id": 0, "object": "none"}
mobile_state = {"batch_id": 0, "object": "none", "image_path": ""}

@app.route("/detect", methods=["POST"])
def detect():
    global image_buffer, current_count, audio_state, batch_counter, mobile_state, global_frame

    # 1. Nhận ảnh
    img_bytes = request.data
    if not img_bytes: return "scanning", 200 
    global_frame = img_bytes # Cho stream video

    # 2. Decode ảnh
    try:
        nparr = np.frombuffer(img_bytes, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        if frame is None: return "error", 200
    except: return "error", 200

    # 3. Chạy YOLO
    try:
        results = model.predict(frame, conf=0.4, verbose=False)
        detected = []
        if results[0].boxes is not None:
            for box in results[0].boxes:
                cls_id = int(box.cls[0])
                detected.append(model.names[cls_id])
        detected = list(set(detected)) # Lọc trùng trong 1 khung hình
    except: return "error", 200

    # --- LOGIC QUAN TRỌNG: CHỈ ĐẾM NẾU CÓ VẬT THỂ ---
    if len(detected) > 0:
        image_buffer.append({"image": frame, "labels": detected})
        current_count += 1
        print(f"--> Frame {current_count}/{BATCH_SIZE}: {detected}")
    else:
        # Nếu không thấy gì thì trả về scanning để LCD hiện "Dang quet..."
        return "scanning", 200

    # 4. Xử lý khi đủ 7 ảnh CÓ NHÃN
    if current_count >= BATCH_SIZE:
        # Gom tất cả nhãn lại
        all_labels = []
        for item in image_buffer: all_labels.extend(item["labels"])

        winner = "none"
        if len(all_labels) > 0:
            # Tìm winner (xuất hiện nhiều nhất)
            winner = Counter(all_labels).most_common(1)[0][0]
            print(f"==> KET QUA BATCH {batch_counter + 1}: {winner}")

            # Chọn ảnh ngẫu nhiên chứa winner để lưu cho App
            candidates = [x["image"] for x in image_buffer if winner in x["labels"]]
            if candidates:
                lucky_frame = random.choice(candidates)
                fname = f"{winner}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
                full_path = os.path.join(SAVE_DIR, fname)
                # Lưu ảnh
                cv2.imwrite(full_path, lucky_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
                
                # Cập nhật cho Mobile App
                mobile_state = {
                    "batch_id": batch_counter + 1,
                    "object": winner,
                    "image_path": full_path
                }
        
        # Cập nhật ID mới để ESP8266 biết đường phát loa
        batch_counter += 1
        audio_state = {"batch_id": batch_counter, "object": winner}

        # Reset bộ đếm
        image_buffer = []
        current_count = 0
        
        # TRẢ VỀ TÊN WINNER ĐỂ ESP32 HIỆN LÊN LCD
        return winner, 200

    return "scanning", 200

# --- API PHỤ ---
@app.route('/video_feed')
def video_feed():
    def generate():
        while True:
            if global_frame:
                yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + global_frame + b'\r\n')
            time.sleep(0.04)
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route("/audio", methods=["GET"])
def get_audio():
    return jsonify(audio_state), 200

@app.route("/mobile/info", methods=["GET"])
def get_mobile_info():
    return jsonify({
        "batch_id": mobile_state["batch_id"],
        "object": mobile_state["object"],
        "has_image": True if mobile_state["image_path"] else False
    }), 200

@app.route("/mobile/image", methods=["GET"])
def get_mobile_image():
    try:
        if mobile_state["image_path"] and os.path.exists(mobile_state["image_path"]):
            return send_file(mobile_state["image_path"], mimetype='image/jpeg')
    except: pass
    return "No image", 404

if __name__ == "__main__":
    # Thay port hoặc host nếu cần
    app.run(host="0.0.0.0", port=5000, threaded=True, debug=False)