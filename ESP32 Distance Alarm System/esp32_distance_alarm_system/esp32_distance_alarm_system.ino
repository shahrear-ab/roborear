// ============================================================
// ESP32 Distance Alarm - HC-SR04 Ultrasonic Sensor
// Red LED + Buzzer when too close, Blue LED when safe
// ============================================================

// Pin Definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RED_LED 19
#define BLUE_LED 21
#define BUZZER_PIN 22  // Optional - comment out if not used

// Threshold distance (cm)
const int DANGER_ZONE = 30;

void setup() {
  Serial.begin(115200);
  
  // Configure pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  #ifdef BUZZER_PIN
    pinMode(BUZZER_PIN, OUTPUT);
  #endif
  
  Serial.println("======================================");
  Serial.println("ESP32 Distance Alarm");
  Serial.println("======================================");
  Serial.print("Danger zone: < ");
  Serial.print(DANGER_ZONE);
  Serial.println(" cm");
  Serial.println("\nReady! Wave your hand in front of the sensor...\n");
}

void loop() {
  // Send trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Measure echo time (microseconds)
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance (cm)
  // Speed of sound = 343 m/s = 0.034 cm/µs
  // Distance = (Time × Speed) / 2 (round trip)
  float distance = duration * 0.034 / 2;
  
  // Print to Serial Monitor (for debugging)
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // Decision logic
  if (distance < DANGER_ZONE && distance > 2) {
    // TOO CLOSE! RED ALERT!
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
    
    #ifdef BUZZER_PIN
      // Beep pattern: 100ms on, 100ms off
      tone(BUZZER_PIN, 1000);
      delay(100);
      noTone(BUZZER_PIN);
      delay(100);
    #endif
    
    Serial.println("⚠️  DANGER! Too close!");
  } 
  else if (distance <= 400 && distance >= DANGER_ZONE) {
    // Safe distance
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
    
    #ifdef BUZZER_PIN
      noTone(BUZZER_PIN);
    #endif
  }
  else {
    // No object detected or out of range
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    
    #ifdef BUZZER_PIN
      noTone(BUZZER_PIN);
    #endif
  }
  
  delay(200);  // Small delay between readings
}