#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --- CẤU HÌNH ---
const char* ssid = "notB";
const char* password = "22222222";
const char* server_url = "http://10.113.168.50:5000/result"; // API Lấy kết quả

// --- DANH SÁCH VẬT THỂ (Copy y nguyên list cũ vào đây) ---
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

// --- PHẦN CỨNG ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // Hoặc 0x27
SoftwareSerial dfSerial(14, 12); // RX=D5, TX=D6 (ESP8266 Pins)
DFRobotDFPlayerMini dfPlayer;

int last_batch_id = 0;
String last_display = "";

void setup() {
  Serial.begin(115200);
  
  // LCD
  Wire.begin(4, 5); // SDA=D2, SCL=D1
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("System Init...");

  // DFPlayer
  dfSerial.begin(9600);
  if(dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer OK");
    dfPlayer.volume(25);
  } else {
    lcd.setCursor(0,1); lcd.print("DFPlayer Error");
  }

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  lcd.setCursor(0,1); lcd.print("WiFi Connected  ");
}

void speak(String objectName) {
  int fileIndex = -1;
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    if (objectName == String(OBJECT_NAMES[i])) {
      fileIndex = i + 1;
      break;
    }
  }
  if (fileIndex != -1) {
    dfPlayer.play(fileIndex);
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, server_url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      JSONVar myObject = JSON.parse(payload);

      if (JSON.typeof(myObject) != "undefined") {
        String status = (const char*) myObject["status"];
        int batch_id = (int) myObject["batch_id"];
        String obj = (const char*) myObject["object"];

        // 1. Xử lý hiển thị
        if (status == "scanning") {
           if (last_display != "scanning") {
             lcd.clear();
             lcd.setCursor(0,0); lcd.print("Scanning...");
             last_display = "scanning";
           }
        } else if (status == "done") {
           // Chỉ cập nhật và phát loa khi có Batch ID mới
           if (batch_id != last_batch_id) {
             last_batch_id = batch_id;
             
             lcd.clear();
             lcd.setCursor(0,0); lcd.print("Result:");
             lcd.setCursor(0,1); lcd.print(obj);
             last_display = obj;
             
             if (obj != "none") {
               speak(obj);
             }
           }
        }
      }
    }
    http.end();
  }
  delay(200); // Hỏi server 5 lần/giây
}