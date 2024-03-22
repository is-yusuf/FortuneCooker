#include <WiFi.h>
#include "esp_wpa2.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <SoftwareSerial.h>
#include "Adafruit_Thermal.h"
#include <string.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()

AsyncWebServer server(80);
#define LED_PIN 21


#define RX_PIN 18 
#define TX_PIN 33 
#define potentiometer 3 
# define buttonpin  5

#define EAP_IDENTITY "eduroam"
#define EAP_USERNAME ""
# define EAP_PASSWORD ""
#define WIFI_NAME "CarletonGuests"
int previousSensorVal = LOW;

String apiKey = "";

SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Thermal printer(&mySerial);

int counter = 0;

String make_message(float voltage) {
  srand(time(NULL)); // Seed the random number generator with current time
  String message = "";
  int choice = rand() % 3; // Generates a random number between 0 and 2

  if (voltage < 0.25) {
    message += (choice == 0) ? "Whisper a timeless truth." :
               (choice == 1) ? "Craft a message of ancient wisdom." :
               "Share a proverb from a forgotten era.";
  } else if (voltage < 0.5) {
    message += (choice == 0) ? "Concoct a curious, offbeat prophecy." :
               (choice == 1) ? "Weave a peculiar prediction." :
               "Offer a forecast, wrapped in mystery.";
  } else if (voltage < 0.75) {
    message += (choice == 0) ? "Deliver a bizarre, unexpected forecast." :
               (choice == 1) ? "Imagine a future, strange and wonderful." :
               "Predict something wildly improbable.";
  } else {
    message += "Let the silliness flow! ";
    if (voltage > 0.9) {
      message += (choice == 0) ? "Now, with extra whimsy!" :
                 (choice == 1) ? "Push the bounds of silliness." :
                 "Elevate absurdity to an art form.";
    }
  }
  return message;
}

void connect_guest(){
  Serial.println();
  WiFi.disconnect(true);
  Serial.println(WiFi.macAddress());

    Serial.print("[WiFi] Connecting to ");
    
    Serial.println(WIFI_NAME);
    WiFi.begin(WIFI_NAME);
    int tryDelay = 500;
    int numberOfTries = 20;

    while (true) {
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            return;
            break;
          case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            Serial.println("[WiFi] WiFi is disconnected");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            return;
            break;
          default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(tryDelay);
        
        if(numberOfTries <= 0){
          Serial.print("[WiFi] Failed to connect to WiFi!");
          // Use disconnect function to force stop trying to connect
          WiFi.disconnect();
          return;
        } else {
          numberOfTries--;
        }
    }
}
void init_printer(){
  mySerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, RX_PIN, TX_PIN);  // must be 8N1 mode
  printer.begin();       
  printer.setFont('A');
  printer.setSize('s');
  printer.boldOn();
  printer.flush();
  printer.setTimes(1,10);
  WebSerial.println("Printer initialized");
}

void print(String message){
  printer.print(message);
  printer.feed(3);
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void connect_wifi(){
  
  Serial.println(WiFi.macAddress());
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(EAP_IDENTITY, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter>=60){ 
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: "); 
  Serial.println(WiFi.localIP()); 
  
}

String ask_gpt(String userMessage) {
  WebSerial.println(userMessage);
  if (WiFi.status() == WL_CONNECTED) { 
    HTTPClient http;
    String postData = "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"system\", \"content\": \"You are a furtune cookie teller. The maximum length allowed is 30 words.\"}]},{\"role\": \"user\", \"content\": \"" + userMessage + "\"}]}";
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + apiKey);

    int httpCode = http.POST(postData);
    String payload = http.getString();

  WebSerial.println("decoding");
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + 60;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    // Serial.print(F("deserializeJson() failed: "));
    WebSerial.print(F("deserializeJson() failed: "));
    WebSerial.println(error.f_str());
    // Serial.println(error.f_str());
    return "";
  }
  const char* content = doc["choices"][0]["message"]["content"];
  http.end();
  // Serial.println(content);
  WebSerial.print(error.f_str());
  
  
  WebSerial.print(content);
  print(content);
  return content;

  }
  else {
    // Serial.println("WiFi not connected");
    WebSerial.println("Wifi not connected");
  }
}

void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");

  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  ask_gpt(d);
  // WebSerial.println();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  connect_guest();
  WebSerial.begin(&server);
  
  WebSerial.msgCallback(recvMsg);
    server.begin();
    
  init_printer();
  WebSerial.println(WiFi.localIP());

  pinMode(buttonpin, INPUT_PULLUP);
  
}


void loop() {  
  int sensorVal = digitalRead(buttonpin);
  
  if (sensorVal == HIGH && previousSensorVal == LOW) { // Transition from LOW to HIGH
    int analogValue = analogRead(potentiometer);
    // Serial.print("analog value: ");
    WebSerial.print("analog value: ");
    WebSerial.println(analogValue);
    // Serial.println(analogValue );
    float voltage = floatMap(analogValue, 0, 8191, 0, 1);
    // Serial.print("Voltage: ");
    // Serial.println(voltage);
    WebSerial.print("voltage: ");
    WebSerial.println(voltage);
    String message = make_message(voltage);
    // Serial.print("Message: ");
    WebSerial.print("Message: ");
    WebSerial.print(message);
    // Serial.println(message );
    String printtext = ask_gpt(message);
   }
previousSensorVal = sensorVal;
  }
