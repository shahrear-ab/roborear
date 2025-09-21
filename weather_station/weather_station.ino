#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT Sensor Settings
#define DHTPIN 4
#define DHTTYPE DHT11  // Changed to DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ-2 Sensor Pins
#define MQ_ANALOG_PIN 34
#define MQ_DIGITAL_PIN 35

// Buzzer Pin
#define BUZZER_PIN 25

// Threshold values
#define SMOKE_THRESHOLD 600

// Weather states
enum WeatherCondition {
  SUNNY,
  CLOUDY,
  RAINY,
  UNKNOWN
};

// Sensor readings
float temperature = 0;
float humidity = 0;
int smokeLevel = 0;
bool smokeDetected = false;

// Alert states
bool smokeAlert = false;

// Sensor status flags
bool oledFound = false;
bool dhtFound = false;
bool sensorsWarmedUp = false;
unsigned long warmUpStartTime = 0;
const unsigned long WARM_UP_TIME = 30000;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with explicit pin assignment
  Wire.begin(21, 22); // SDA, SCL
  Wire.setClock(100000); // Reduce I2C speed to 100kHz
  
  // Initialize pins
  pinMode(MQ_DIGITAL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize DHT sensor with better error handling
  Serial.println("Initializing DHT sensor...");
  dht.begin();
  delay(2000); // Give DHT sensor time to initialize
  
  // Test DHT sensor
  if (testDHT()) {
    dhtFound = true;
    Serial.println("DHT sensor found!");
  } else {
    Serial.println("DHT sensor not found. Check wiring and pull-up resistor!");
  }
  
  // Try to initialize OLED display
  Serial.println("Initializing OLED...");
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledFound = true;
    Serial.println("OLED found!");
    
    // Clear display buffer
    display.clearDisplay();
    
    // Display startup message with proper text boundaries
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Air Quality Monitor");
    display.setCursor(0, 20);
    display.println("Initializing...");
    display.display();
  } else {
    Serial.println("OLED not found. Continuing without display.");
  }
  
  // Start warm-up period
  warmUpStartTime = millis();
  Serial.println("Warming up gas sensor (30 seconds)...");
  Serial.println("Please ensure sensor is in clean air");
}

bool testDHT() {
  Serial.println("Testing DHT sensor...");
  
  for (int i = 0; i < 5; i++) {
    float testTemp = dht.readTemperature();
    float testHum = dht.readHumidity();
    
    if (!isnan(testTemp) && !isnan(testHum)) {
      Serial.print("DHT test successful: ");
      Serial.print(testTemp);
      Serial.print("°C, ");
      Serial.print(testHum);
      Serial.println("%");
      return true;
    }
    delay(1000);
  }
  
  Serial.println("DHT test failed after 5 attempts");
  return false;
}

void loop() {
  if (!sensorsWarmedUp) {
    handleWarmUpPeriod();
    return;
  }
  
  readSensors();
  checkAlerts();
  
  // Get weather condition and advice
  WeatherCondition currentWeather = determineWeather();
  String weatherAdvice = getWeatherAdvice(currentWeather);
  
  displayData(currentWeather, weatherAdvice);
  printSerialData(currentWeather, weatherAdvice);
  handleBuzzer();
  
  delay(2000);
}

void handleWarmUpPeriod() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - warmUpStartTime;
  unsigned int remainingTime = (WARM_UP_TIME - elapsedTime) / 1000;
  
  if (elapsedTime >= WARM_UP_TIME) {
    sensorsWarmedUp = true;
    Serial.println("Warm-up complete! Starting normal operation.");
    
    if (oledFound) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("Warm-up complete!");
      display.setCursor(0, 20);
      display.println("System ready");
      display.display();
      delay(2000);
    }
  } else {
    if (currentTime % 1000 < 100) {
      Serial.print("Warming up... ");
      Serial.print(remainingTime);
      Serial.println(" seconds remaining");
    }
    delay(100);
  }
}

void readSensors() {
  // Read DHT sensor with multiple attempts
  if (dhtFound) {
    int attempts = 0;
    while (attempts < 3) {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      
      if (!isnan(temperature) && !isnan(humidity)) {
        break;
      }
      attempts++;
      delay(100);
    }
    
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor after multiple attempts!");
      humidity = 0;
      temperature = 0;
    }
  }  
  
  // Read MQ-2 sensor
  smokeLevel = analogRead(MQ_ANALOG_PIN);
  smokeDetected = (digitalRead(MQ_DIGITAL_PIN) == LOW);
}

void checkAlerts() {
  smokeAlert = (smokeDetected || smokeLevel > SMOKE_THRESHOLD);
}

void handleBuzzer() {
  if (smokeAlert) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

WeatherCondition determineWeather() {
  if (isnan(temperature) || isnan(humidity)) {
    return UNKNOWN;
  }
  
  // Rule-based weather detection
  if (temperature > 30.0 && humidity < 50.0) {
    return SUNNY; // Hot and dry = sunny
  }
  else if (temperature > 25.0 && humidity < 70.0) {
    return SUNNY; // Warm and moderately dry = sunny
  }
  else if (temperature > 20.0 && humidity > 80.0) {
    return RAINY; // Moderate temperature with high humidity = rainy
  }
  else if (temperature < 15.0 && humidity > 75.0) {
    return RAINY; // Cool with high humidity = rainy
  }
  else if (humidity > 85.0) {
    return RAINY; // Very high humidity likely means rain
  }
  else if (temperature > 18.0 && temperature < 28.0 && humidity > 60.0 && humidity < 80.0) {
    return CLOUDY; // Moderate conditions = cloudy
  }
  else if (temperature < 10.0 && humidity < 50.0) {
    return SUNNY; // Cold but dry = sunny (winter day)
  }
  else {
    return CLOUDY; // Default to cloudy for other conditions
  }
}

String getWeatherAdvice(WeatherCondition condition) {
  switch(condition) {
    case SUNNY:
      return "Wear sunscreen!";
    case CLOUDY:
      return "Mild weather today";
    case RAINY:
      return "Bring an umbrella!";
    case UNKNOWN:
      return "Check sensor connection";
    default:
      return "Weather normal";
  }
}



String getWeatherText(WeatherCondition condition) {
  switch(condition) {
    case SUNNY:
      return "Sunny";
    case CLOUDY:
      return "Cloudy";
    case RAINY:
      return "Rainy";
    case UNKNOWN:
      return "Unknown";
    default:
      return "Normal";
  }
}

void displayData(WeatherCondition weather, String advice) {
  if (!oledFound) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header - ensure it fits within screen width
  display.setCursor(0, 0);
  display.println("Air Quality Monitor");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Temperature - fixed positioning
  display.setCursor(0, 15);
  display.print("Temp:");
  display.setCursor(40, 15); // Fixed position for value
  if (dhtFound) {
    display.print(temperature, 1);
    display.print("C");
  } else {
    display.print("Error");
  }
  
  // Humidity - fixed positioning
  display.setCursor(0, 25);
  display.print("Hum: ");
  display.setCursor(40, 25); // Fixed position for value
  if (dhtFound) {
    display.print(humidity, 1);
    display.print("%");
  } else {
    display.print("Error");
  }
  
  // Weather Status
  display.setCursor(0, 35);
  display.print("Weather:");
  display.setCursor(50, 35);
  display.print(getWeatherText(weather));
  
  // Air Quality - fixed positioning
  display.setCursor(0, 45);
  display.print("Air Q:");
  display.setCursor(40, 45);
  display.print(smokeLevel);
  if (smokeAlert) {
    display.print("!");
  }
  
  // Weather Advice (scroll if too long)
  display.setCursor(0, 55);
  display.print(advice);
  
  // Smoke Alert indicator
  if (smokeAlert) {
    display.fillRect(110, 0, 18, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(112, 1);
    display.print("SMK");
    display.setTextColor(SSD1306_WHITE);
  }
  
  display.display();
}


void printSerialData(WeatherCondition weather, String advice) {
  Serial.println("===== Sensor Readings =====");
  
  if (dhtFound) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    
    Serial.print("Weather: ");
    Serial.print(getWeatherText(weather));
    
    Serial.print("Advice: ");
    Serial.println(advice);
  } else {
    Serial.println("DHT Sensor: NOT CONNECTED");
  }
  
  Serial.print("Smoke Level: ");
  Serial.println(smokeLevel);
  
  Serial.print("Smoke Detected: ");
  Serial.println(smokeDetected ? "YES" : "NO");
  
  Serial.print("Smoke Alert: ");
  Serial.println(smokeAlert ? "YES" : "NO");
  
  Serial.println("===========================");
  Serial.println();
}