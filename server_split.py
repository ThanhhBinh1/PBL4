from flask import Flask, request, jsonify, send_file, Response
from ultralytics import YOLO
import cv2
import numpy as np
import os
import datetime
import random
import time
from collections import Counter

app = Flask(__name__)

# --- CẤU HÌNH ---
try:
    model = YOLO("best.pt")
    print("--> TAI MO HINH THANH CONG!")
except:
    print("--> LOI: Khong tim thay best.pt")
    # exit() 

SAVE_DIR = "detected_images"
if not os.path.exists(SAVE_DIR): os.makedirs(SAVE_DIR)

# --- BIẾN BỘ NHỚ ---
BATCH_SIZE = 5
image_buffer = []
current_count = 0
batch_counter = 0

# Biến dùng chung
global_frame = None  # Dùng cho Stream
audio_state = {"batch_id": 0, "object": "none"}
mobile_state = {"batch_id": 0, "object": "none", "image_path": ""}

@app.route("/detect", methods=["POST"])
def detect():
    global image_buffer, current_count, audio_state, batch_counter, mobile_state, global_frame

    try:
        img_bytes = request.data
        if not img_bytes: return "scanning", 200 
        
        # 1. Cập nhật cho Stream
        global_frame = img_bytes 

        # 2. Decode cho AI
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
    
    # XỬ LÝ KHI ĐỦ BATCH
    if current_count >= BATCH_SIZE:
        all_labels = []
        for item in image_buffer: all_labels.extend(item["labels"])

        winner = "none"
        if len(all_labels) > 0:
            winner = Counter(all_labels).most_common(1)[0][0]
            
            # Lưu ảnh winner
            candidates = [x["image"] for x in image_buffer if winner in x["labels"]]
            if candidates:
                lucky_frame = random.choice(candidates)
                fname = f"{winner}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
                full_path = os.path.join(SAVE_DIR, fname)
                cv2.imwrite(full_path, lucky_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 60]) # Nén nhẹ
                
                mobile_state = {
                    "batch_id": batch_counter + 1,
                    "object": winner,
                    "image_path": full_path
                }

        batch_counter += 1
        audio_state = {"batch_id": batch_counter, "object": winner}
        image_buffer = []
        current_count = 0
        return winner, 200

    return "scanning", 200

# --- API STREAM MJPEG ---
def generate_frames():
    while True:
        if global_frame:
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + global_frame + b'\r\n')
        time.sleep(0.04)

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

# --- API MOBILE ---
@app.route("/mobile/info", methods=["GET"])
def get_mobile_info():
    response_data = {
        "batch_id": mobile_state["batch_id"],
        "object": mobile_state["object"],
        "has_image": True if mobile_state["image_path"] else False
    }
    return jsonify(response_data), 200

@app.route("/mobile/image", methods=["GET"])
def get_mobile_image():
    try:
        if mobile_state["image_path"] and os.path.exists(mobile_state["image_path"]):
            return send_file(mobile_state["image_path"], mimetype='image/jpeg')
        else: return "No image yet", 404
    except: return "Error", 500

@app.route("/audio", methods=["GET"])
def get_audio():
    return jsonify(audio_state), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, threaded=True, debug=False)