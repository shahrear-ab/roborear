#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// Relay pin
const int relayPin = 26;

// WiFi credentials
const char* WIFI_SSID = "Shadhin WiFi";
const char* WIFI_PASS = "1234567m";

// SinricPro credentials (get from website)
#define APP_KEY    "c586b140-c06e-43ee-8de4-d2dcec960063"     // From SinricPro dashboard
#define APP_SECRET "bf333a08-063e-4201-9acc-bb4992e74ba7-81d7fa33-cf74-4599-ba50-02b59f5b4be6"  // From SinricPro dashboard
#define DEVICE_ID "68d556ea382e7b1db3a153ad"     // From SinricPro dashboard


#define RELAY_PIN 26  // GPIO 26 connected to relay


bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Device %s turned %s\r\n", deviceId.c_str(), state ? "ON" : "OFF");
  
  // Control the relay
  if (state) {
    digitalWrite(RELAY_PIN, LOW);  // Turn relay ON
    Serial.println("Light turned ON via voice");
  } else {
    digitalWrite(RELAY_PIN, HIGH); // Turn relay OFF
    Serial.println("Light turned OFF via voice");
  }
  
  return true;
}

void setup() {
  Serial.begin(115200);
  
  // Setup relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start OFF
  
  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  
  // Setup SinricPro device
  SinricProSwitch& mySwitch = SinricPro[DEVICE_ID];
  mySwitch.onPowerState(onPowerState);
  
  // Start SinricPro
  SinricPro.begin(APP_KEY, APP_SECRET);
  Serial.println("SinricPro initialization complete!");
  Serial.println("You can now use voice commands:");
  Serial.println("- 'Hey google, turn on living room's desk led'");
  Serial.println("- 'Hey google, turn off living room's desk led'");
}

void loop() {
  SinricPro.handle(); // Handle SinricPro commands
  delay(10);
}