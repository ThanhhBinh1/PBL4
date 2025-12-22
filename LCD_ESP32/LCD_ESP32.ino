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
// SỬA IP NÀY CHO ĐÚNG VỚI MÁY TÍNH CỦA BẠN (LỆNH ipconfig)
const char* server_url = "http://10.71.89.100:5000/detect"; 

LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- PIN CAM ---
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

HTTPClient http;
String last_display = "";

void updateLCD(String text) {
  text.trim();
  if (text == last_display) return;
  
  lcd.clear();
  lcd.setCursor(0,0);
  
  if (text == "scanning") {
    lcd.print("Dang quet...");
    lcd.setCursor(0,1); lcd.print("Cho 7 anh...");
  } else if (text == "error") {
    lcd.print("Loi Server");
  } else {
    // ĐÂY LÀ LÚC NHẬN ĐƯỢC WINNER
    lcd.print("Ket qua:");
    lcd.setCursor(0,1);
    lcd.print(text); // In tên tiếng Anh (Winner)
  }
  last_display = text;
}

void setup() {
  setCpuFrequencyMhz(240);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  
  Wire.begin(15, 14); // SDA, SCL cho LCD
  lcd.init(); lcd.backlight();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  lcd.print("WiFi OK!"); delay(1000);

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
  config.frame_size = FRAMESIZE_CIF;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST; 
  
  esp_camera_init(&config);
  http.setReuse(true);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) { esp_camera_fb_return(fb); return; }

    http.begin(server_url);
    http.addHeader("Content-Type", "image/jpeg");
    int httpCode = http.POST(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (httpCode == 200) {
      updateLCD(http.getString()); // Hiển thị kết quả trả về từ Server
    } else {
      updateLCD("error");
    }
  }
  delay(50);
}