#include <esp_now.h>
#include <WiFi.h>

// L298N Motor Control Pins (Your original pins)
#define out1 12    // Left Motor Direction 1
#define out2 14    // Left Motor Direction 2  
#define out3 27    // Right Motor Direction 1
#define out4 26    // Right Motor Direction 2

typedef struct struct_message {
  int16_t x;  // Joystick X value (0-4095)
  int16_t y;  // Joystick Y value (0-4095)
} struct_message;

struct_message incomingData;

// Function declarations
void initializeMotors();
void controlMotors(int xValue, int yValue);
void stopMotors();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();

void onReceive(const uint8_t * mac, const uint8_t *incomingDataRaw, int len) {
  memcpy(&incomingData, incomingDataRaw, sizeof(incomingData));
  Serial.printf("Received - X:%4d Y:%4d\n", incomingData.x, incomingData.y);
  
  // Use your existing motor control logic
  controlMotors(incomingData.x, incomingData.y);
}

// Your original motor control functions
void initializeMotors() {
  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  pinMode(out3, OUTPUT);
  pinMode(out4, OUTPUT);
  
  digitalWrite(out1, LOW);
  digitalWrite(out2, LOW);
  digitalWrite(out3, LOW);
  digitalWrite(out4, LOW);
}

void controlMotors(int xValue, int yValue) {
  // Convert joystick values to motor control
  // Joystick range: 0-4095, Center: ~2048

  const int center = 2700;
  const int deadZone = 500;

  // Check if in dead zone (center position)
  if ((xValue > center - deadZone) && (xValue < center + deadZone) &&
      (yValue > center - deadZone) && (yValue < center + deadZone)) {
    stopMotors();
    Serial.println("Action: STOP");
    return;
  }

  // Determine primary direction based on which axis has stronger input
  int xDiff = abs(xValue - center);
  int yDiff = abs(yValue - center);

  if (yDiff > xDiff) {
    // Y-axis dominant (Forward/Backward)
    if (yValue < center - deadZone) {
      moveForward();
      Serial.println("Action: FORWARD");
    } else if (yValue > center + deadZone) {
      moveBackward();
      Serial.println("Action: BACKWARD");
    }
  } else {
    if (xValue == 4095 && yValue == 4095) {
      // Special case: Both axes maxed - move backward
      moveBackward();
      Serial.println("Action: BACKWARD");
    } else if (xValue == 0 && yValue == 0) {
      moveBackward();
      Serial.println("Action: BACKWARD");
    } else {
      // X-axis dominant (Left/Right)
      if (xValue < center - deadZone) {
        turnLeft();
        Serial.println("Action: LEFT");
      } else if (xValue > center + deadZone) {
        turnRight();
        Serial.println("Action: RIGHT");
      }
    }
  }
}

void moveForward() {
  // Both motors forward
  digitalWrite(out1, HIGH);  // Left motor forward
  digitalWrite(out2, LOW);
  digitalWrite(out3, LOW);   // Right motor forward
  digitalWrite(out4, HIGH);
}

void moveBackward() {
  // Both motors backward
  digitalWrite(out1, LOW);
  digitalWrite(out2, HIGH);  // Left motor backward
  digitalWrite(out3, HIGH);
  digitalWrite(out4, LOW);   // Right motor backward
}

void turnLeft() {
  // Right motor forward, left motor stopped
  digitalWrite(out1, HIGH);   // Left motor stop
  digitalWrite(out2, LOW);
  digitalWrite(out3, HIGH);   // Right motor forward
  digitalWrite(out4, LOW);
}

void turnRight() {
  // Left motor forward, right motor stopped
  digitalWrite(out1, LOW);    // Left motor forward
  digitalWrite(out2, HIGH);
  digitalWrite(out3, LOW);    // Right motor stop
  digitalWrite(out4, HIGH);
}

void stopMotors() {
  digitalWrite(out1, LOW);
  digitalWrite(out2, LOW);
  digitalWrite(out3, LOW);
  digitalWrite(out4, LOW);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize motors
  initializeMotors();
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while(1);
  }

  // Register receive callback
  esp_now_register_recv_cb(onReceive);

  Serial.println("ESP-NOW Receiver Ready");
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());
  
  // Stop motors initially
  stopMotors();
}

void loop() {
  // Nothing to do here - everything handled in callback
  delay(1000); // Small delay to prevent watchdog timer issues
}