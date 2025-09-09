// Sound-Activated LED System for ESP32
// Blue (soft), Green (medium), Red (loud) LED activation based on sound volume

// Pin definitions
const int SOUND_SENSOR_PIN = 34;  // Analog pin for sound sensor
const int BLUE_LED_PIN = 18;      // GPIO pin for blue LED
const int GREEN_LED_PIN = 19;      // GPIO pin for green LED
const int RED_LED_PIN = 21;        // GPIO pin for red LED
const int BUZZER_PIN = 23;

// Sound threshold values (you may need to adjust these based on testing)
const int QUIET_THRESHOLD = 1000;   // Below this value: silence
const int MEDIUM_THRESHOLD = 1500;  // Between QUIET and this: blue LED
const int LOUD_THRESHOLD = 3000;    // Between MEDIUM and this: green LED
                                  // Above this: red LED

// Variables for smoothing sound readings
const int NUM_READINGS = 10;      // Number of readings to average
int readings[NUM_READINGS];       // Array to store readings
int readIndex = 0;                // Index of current reading
int total = 0;                    // Running total
int average = 0;                  // Average value

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize LED pins as outputs
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize all LEDs to off
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize the readings array to 0
  for (int i = 0; i < NUM_READINGS; i++) {
    readings[i] = 0;
  }
  
  // Allow the sound sensor to stabilize
  delay(500);
  Serial.println("Sound-Activated LED System Ready!");
}

void loop() {
  // Read the sound sensor value
  int sensorValue = analogRead(SOUND_SENSOR_PIN);
  
  // Smooth the reading using a moving average
  total = total - readings[readIndex];
  readings[readIndex] = sensorValue;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % NUM_READINGS;
  
  // Calculate the average
  average = total / NUM_READINGS;
  
  // Print the raw and average values for debugging
  Serial.print("Raw: ");
  Serial.print(sensorValue);
  Serial.print(" | Average: ");
  Serial.print(average);
  
  // Control LEDs based on sound level
  if (average < QUIET_THRESHOLD) {
    // Quiet - turn off all LEDs
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println(" | Status: Quiet");
  } else if (average >= QUIET_THRESHOLD && average < MEDIUM_THRESHOLD) {
    // Medium-low volume - blue LED
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println(" | Status: Blue LED (Medium-low)");
  } else if (average >= MEDIUM_THRESHOLD && average < LOUD_THRESHOLD) {
    // Medium volume - green LED
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println(" | Status: Green LED (Medium)");
  } else {
    // Loud volume - red LED
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    Serial.println(" | Status: Red LED (Loud)");
  }
  
  // Small delay for stability
  delay(10);
}