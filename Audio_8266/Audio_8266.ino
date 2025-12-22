#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

const char* ssid = "notB";
const char* password = "22222222";
// !!! SỬA IP CHO ĐÚNG !!!
const char* server_url = "http://10.71.89.100:5000/audio"; 

// --- List file nhạc trong thẻ nhớ SD phải khớp thứ tự này ---
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

SoftwareSerial dfSerial(14, 12); // D5=RX, D6=TX
DFRobotDFPlayerMini dfPlayer;

int last_batch_id = 0;

void setup() {
  Serial.begin(115200);
  dfSerial.begin(9600);
  
  if(dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer OK");
    dfPlayer.volume(30); // Âm lượng 0-30
  } else {
    Serial.println("DFPlayer Error!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected");
}

void speak(String objectName) {
  int fileIndex = -1;
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    if (objectName == String(OBJECT_NAMES[i])) {
      fileIndex = i + 1; // File 0001.mp3 ứng với index 0
      break;
    }
  }
  if (fileIndex != -1) {
    Serial.print("Playing File: "); Serial.println(fileIndex);
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
        int batch_id = (int) myObject["batch_id"];
        String obj = (const char*) myObject["object"];

        // Nếu có ID mới -> Phát loa
        if (batch_id > last_batch_id) {
           last_batch_id = batch_id;
           Serial.println("New Detection: " + obj);
           if (obj != "none") {
             speak(obj);
           }
        }
      }
    }
    http.end();
  }
  delay(300); // Check server mỗi 300ms
}