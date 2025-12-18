from flask import Flask, request
import os
import datetime
import cv2
import numpy as np

app = Flask(__name__)

# --- CẤU HÌNH TÊN VẬT THỂ CẦN CHỤP ---
# Ví dụ: Muốn chụp quả táo thì đổi thành "apple", chụp cái bút thì đổi thành "pen"
CLASS_NAME = "apple"  # Thay đổi tên vật thể ở đây

# Tạo thư mục lưu ảnh
SAVE_DIR = f"dataset/{CLASS_NAME}"
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)

print(f"--> DANG CHE DO THU THAP DU LIEU: {CLASS_NAME}")
print(f"--> Anh se duoc luu vao: {SAVE_DIR}")

@app.route("/upload", methods=["POST"])
def upload_image():
    try:
        img_bytes = request.data
        if not img_bytes:
            return "empty", 400

        # Giải mã ảnh
        nparr = np.frombuffer(img_bytes, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        if frame is not None:
            # Tạo tên file không trùng: ten_vat_the_ngay_gio_giay.jpg
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
            filename = f"{CLASS_NAME}_{timestamp}.jpg"
            filepath = os.path.join(SAVE_DIR, filename)
            
            # Lưu ảnh
            cv2.imwrite(filepath, frame)
            print(f"Da luu: {filename}")
            return "ok", 200
        else:
            return "error", 500
    except Exception as e:
        print(f"Loi: {e}")
        return "error", 500

if __name__ == "__main__":
    # Chạy trên tất cả các IP của máy
    app.run(host="0.0.0.0", port=5000, debug=False)