#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobotDFPlayerMini.h>
// --- THƯ VIỆN CHỐNG SỤT ÁP ---
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ==================== CẤU HÌNH WIFI & SERVER ====================
const char* ssid = "notB";       // Đổi lại nếu dùng Hotspot điện thoại
const char* password = "22222222"; 
// Nhớ đổi IP nếu máy tính đổi IP
const char* server_url = "http://10.113.168.50:5000/detect"; 

// ==================== DANH SÁCH 113 VẬT THỂ ====================
const char* OBJECT_NAMES[] = {
  "airplane", "ambulance", "apple", "banana", "battery", "bed", "bicycle", "bird", "biscuit", "boat", 
  "bowl", "box", "bread", "broccoli", "broom", "bucket", "butter", "butterfly", "can", "candle", 
  "carrot", "cat", "cauliflower", "chair", "chili", "clock", "cloud", "coin", "comb", "compass", 
  "computer mouse", "cookie", "corn", "cow", "crab", "cucumber", "dog", "dustpan", "egg", "elephant", 
  "eraser", "fan", "fire hydrant", "fish", "fork", "frying pan", "garlic", "glasses", "globe", "gloves", 
  "guitar", "hammer", "hat", "helmet", "ice cream", "key", "keyboard", "lamp", "laptop", "lettuce", 
  "lighter", "lobster", "mango", "microwave", "monkey", "mop", "mouse", "notebook", "onion", "orange", 
  "paintbrush", "pants", "pen", "pencil", "penguin", "phone", "piano", "pillow", "pliers", "plug", 
  "pumpkin", "refrigerator", "ring", "rose", "ruler", "scissors", "shoes", "shrimp", "slipper", "socks", 
  "sofa", "spoon", "strawberry", "sunflower", "table", "tape", "television", "thermometer", "tie", "tiger", 
  "tire", "toilet", "tomato", "toothbrush", "towel", "traffic light", "turtle", "umbrella", "vase", "wallet", 
  "watch", "watermelon", "window"
};
const int TOTAL_CLASSES = 113;

// ==================== PHẦN CỨNG ====================
LiquidCrystal_I2C lcd(0x3F, 16, 2); 

#define DFPLAYER_RX_PIN 13 
#define DFPLAYER_TX_PIN 12 
DFRobotDFPlayerMini dfPlayer;

String current_objects = "";
String last_objects = ""; 
String last_display = "";
bool wifi_ok = false;
bool cam_ok = false;

// ==================== CAMERA PINS ====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==================== SETUP ====================
void setup() {
  // --- QUAN TRỌNG NHẤT: TẮT BROWNOUT DETECTOR ---
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  // ----------------------------------------------

  Serial.begin(115200);
  Serial.println("\n=== ESP32-CAM ANTI-BROWNOUT ===");

  // 1. LCD
  Wire.begin(15, 14); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("System Booting");

  // 2. DFPlayer
  Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  if (dfPlayer.begin(Serial2)) {
    Serial.println("DFPlayer OK!");
    dfPlayer.volume(25); 
  }

  // 3. WiFi
  lcd.setCursor(0, 1); lcd.print("WiFi Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200); Serial.print(".");
  }
  Serial.println("\nWiFi OK!");
  wifi_ok = true;
  lcd.setCursor(0, 1); lcd.print("WiFi OK!      ");

  // 4. Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Giữ nguyên cấu hình tốc độ cao
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 15; 
  config.fb_count = 1; // Nếu vẫn reset thì đổi số này thành 1
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&config) == ESP_OK) {
    Serial.println("Camera OK");
    cam_ok = true;
  } else {
    lcd.setCursor(0, 1); lcd.print("Cam Error!");
    while(1);
  }
  
  delay(1000);
}

// ==================== CÁC HÀM KHÁC (GIỮ NGUYÊN) ====================
void speak() {
  if (current_objects == "scanning" || current_objects == "none" || current_objects == "") return;
  if (current_objects == last_objects) return; 

  last_objects = current_objects;
  Serial.println(">>> CHOT KET QUA: " + current_objects);

  int fileIndex = -1;
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    if (current_objects.indexOf(OBJECT_NAMES[i]) != -1) {
      fileIndex = i + 1;
      break;
    }
  }
  
  if (fileIndex != -1) {
    dfPlayer.play(fileIndex);
    delay(2000); 
  }
}

void updateLCD() {
  if (current_objects == "scanning") {
     if (last_display != "scanning") {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Processing...");
        lcd.setCursor(0, 1); lcd.print("Scanning...");
        last_display = "scanning";
     }
     return;
  }

  if (current_objects == last_display) return;
  last_display = current_objects;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Result:");
  lcd.setCursor(0, 1);
  if (current_objects == "none" || current_objects == "") {
    lcd.print("No Object");
  } else {
    String show = current_objects;
    if (show.length() > 16) show = show.substring(0, 16);
    lcd.print(show);
  }
}

void sendFrame() {
  if (WiFi.status() != WL_CONNECTED) return;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;

  HTTPClient http;
  http.setReuse(true); 
  http.begin(server_url);
  http.addHeader("Content-Type", "image/jpeg"); 

  int httpCode = http.POST(fb->buf, fb->len);

  if (httpCode == 200) {
    String payload = http.getString();
    payload.trim();
    if (payload.length() > 0) {
      current_objects = payload;
      if (payload == "scanning") Serial.print("."); 
      else Serial.println("\nDONE: " + payload);
    }
  }
  http.end();
  esp_camera_fb_return(fb); 
}

void loop() {
  if (wifi_ok && cam_ok) {
    sendFrame();   
    updateLCD();   
    speak();       
  }
  delay(100); 
}