#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define JOY_X_PIN 34  // ADC input for joystick X
#define JOY_Y_PIN 35  // ADC input for joystick Y

// Receiver MAC Address (Your Car)
uint8_t receiverAddress[] = {0xEC, 0xE3, 0x34, 0x1A, 0xF4, 0x04};

typedef struct struct_message {
  int16_t x;  // Joystick X value
  int16_t y;  // Joystick Y value
} struct_message;

struct_message myData;

void onSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize joystick pins
  pinMode(JOY_X_PIN, INPUT);
  pinMode(JOY_Y_PIN, INPUT);
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Set channel for better reliability
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }
  
  // Register send callback
  esp_now_register_send_cb(onSent);

  // Add peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (1);
  }

  Serial.println("ESP-NOW Transmitter Ready");
  Serial.print("Target MAC: ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           receiverAddress[0], receiverAddress[1], receiverAddress[2],
           receiverAddress[3], receiverAddress[4], receiverAddress[5]);
  Serial.println(macStr);
}

void loop() {
  // Read joystick values (0-4095)
  int rawX = analogRead(JOY_X_PIN);
  int rawY = analogRead(JOY_Y_PIN);

  // Send raw values (0-4095) to maintain your existing motor control logic
  myData.x = rawX;
  myData.y = rawY;

  // Send data via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.printf("Sent - X:%4d Y:%4d\n", rawX, rawY);
  } else {
    Serial.println("Send error");
  }

  delay(50);  // Send every 50ms
}