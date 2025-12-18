#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- CẤU HÌNH MẠNG ---
const char* ssid = "notB";
const char* password = "22222222";
const char* server_url = "http://10.22.168.100:5000/detect";

// --- LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- CAMERA PINS ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

HTTPClient http; // Khai báo toàn cục để dùng lại kết nối
String last_display = "";

// Hàm LCD tối ưu RAM
void updateLCD(const String &text) {
  if (text == last_display) return;
  lcd.clear();
  if (text == "scanning") {
      lcd.setCursor(0,0); lcd.print("Dang quet...");
  } else if (text == "error") {
      lcd.setCursor(0,0); lcd.print("Loi Server");
  } else {
      lcd.setCursor(0,0); lcd.print("Phat hien:");
      lcd.setCursor(0,1); 
      if (text.length() > 16) lcd.print(text.substring(0, 16));
      else lcd.print(text);
  }
  last_display = text;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  // 1. LCD
  Wire.begin(15, 14); 
  lcd.init(); lcd.backlight();
  lcd.print("No PSRAM Mode");

  // 2. WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  lcd.clear(); lcd.print("WiFi OK!");

  // 3. CẤU HÌNH CAMERA (TỐI ƯU CHO RAM YẾU)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; // 20MHz
  
  config.pixel_format = PIXFORMAT_JPEG;

  // --- THIẾT LẬP QUAN TRỌNG NHẤT ---
  // CIF (400x296) là giới hạn an toàn cao nhất cho chip không PSRAM
  config.frame_size = FRAMESIZE_CIF; 
  
  // Chất lượng 12 là rất tốt, nếu vẫn lag có thể tăng lên 15-20
  config.jpeg_quality = 12; 
  
  // BẮT BUỘC LÀ 1 (Vì không đủ RAM cho 2)
  config.fb_count = 1; 
  
  config.grab_mode = CAMERA_GRAB_LATEST; 

  if (esp_camera_init(&config) != ESP_OK) {
    lcd.clear(); lcd.print("Cam Init Fail");
    ESP.restart();
  }
  
  // Bật chế độ tái sử dụng kết nối (Keep-Alive)
  // Giúp gửi liên tục mà không cần bắt tay lại 3 bước TCP
  http.setReuse(true);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    
    // BƯỚC 1: Lấy ảnh
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Cam Fail");
      // Nếu lỗi cam, reset nhẹ buffer rồi thử lại, không cần restart chip ngay
      esp_camera_fb_return(fb); 
      delay(100); 
      return;
    }

    // BƯỚC 2: Gửi siêu tốc
    http.begin(server_url);
    http.addHeader("Content-Type", "image/jpeg");
    
    int httpCode = http.POST(fb->buf, fb->len);
    
    // BƯỚC 3: TRẢ RAM NGAY LẬP TỨC (Cực quan trọng với Buffer = 1)
    esp_camera_fb_return(fb); 
    fb = NULL;

    // BƯỚC 4: Xử lý kết quả
    if (httpCode == 200) {
      String response = http.getString();
      response.trim();
      updateLCD(response);
    } else {
      updateLCD("error");
      // Nếu lỗi mạng, đóng kết nối để làm sạch socket
      http.end(); 
    }
    
    // Mẹo: Nếu RAM tụt quá sâu, tự reset để tránh treo
    if(ESP.getFreeHeap() < 20000) {
        updateLCD("Reset RAM...");
        ESP.restart();
    }

  } else {
    WiFi.disconnect();
    WiFi.reconnect();
  }
  
  // Delay cực nhỏ: Gần như real-time
  delay(10); 
}