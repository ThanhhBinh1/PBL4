#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- CẤU HÌNH ---
const char* ssid = "notB";
const char* password = "22222222";
const char* server_url = "http://172.24.66.50:5000/detect"; // Thay IP mới

// --- LCD (Địa chỉ 0x3F hoặc 0x27) ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- CAMERA PINS ---
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

String last_display = "";

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Tắt Brownout
  Serial.begin(115200);

  // 1. LCD Init
  Wire.begin(15, 14); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Khoi dong...");

  // 2. WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  lcd.setCursor(0,1); lcd.print("WiFi OK!");
  delay(1000);

  // 3. Camera Config (Tốc độ cao)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 320x240
  config.jpeg_quality = 12;
  config.fb_count = 2; // Buffer đôi
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_camera_init(&config);
}

// --- HÀM CẬP NHẬT MÀN HÌNH (Đã Việt Hóa) ---
void updateLCD(String text) {
  // Nếu nội dung không đổi thì không vẽ lại (đỡ nháy màn hình)
  if (text == last_display) return;
  
  lcd.clear(); // Xóa sạch màn hình trước khi in

  if (text == "scanning") {
      // Trường hợp 1: Đang chụp liên tục để gom đủ 5 ảnh
      lcd.setCursor(0,0); lcd.print("Dang quet...");
      lcd.setCursor(0,1); lcd.print("Vui long cho");
  } 
  else if (text == "none" || text == "") {
      // Trường hợp 2: Đã phân tích xong nhưng không thấy gì
      lcd.setCursor(0,0); lcd.print("Khong tim thay");
      lcd.setCursor(0,1); lcd.print("vat the nao");
  } 
  else if (text == "error") {
      // Trường hợp 3: Lỗi
      lcd.setCursor(0,0); lcd.print("Loi Server");
  }
  else {
      // Trường hợp 4: Đã tìm thấy (text là tên vật thể, ví dụ 'apple')
      lcd.setCursor(0,0); lcd.print("Phat hien:");
      lcd.setCursor(0,1); 
      if (text.length() > 16) text = text.substring(0, 16);
      lcd.print(text); // In tên tiếng Anh (hoặc bạn có thể mapping sang tiếng Việt nếu muốn)
  }
  
  last_display = text;
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      HTTPClient http;
      http.setReuse(true); // Giữ kết nối để gửi nhanh
      http.begin(server_url);
      http.addHeader("Content-Type", "image/jpeg");
      
      int code = http.POST(fb->buf, fb->len);
      if (code == 200) {
        String response = http.getString();
        response.trim();
        if (response.length() > 0) {
           updateLCD(response); // Hiển thị nội dung Server trả về
        }
      }
      http.end();
      esp_camera_fb_return(fb);
    }
  }
  delay(100); // Tốc độ chụp nhanh
}