#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
#define BUTTON_PIN 27      // Connect one side of button here
#define BUZZER_PIN 13     // Buzzer positive

// RGB LED Pins
#define RED_PIN 12
#define GREEN_PIN 14
#define BLUE_PIN 15

// Pomodoro timing constants (in seconds) - CHANGED NAMES
#define POMODORO_DURATION 25 * 60  // 25 minutes
#define SHORT_BREAK_DURATION 5 * 60     // 5 minutes
#define LONG_BREAK_DURATION 15 * 60     // 15 minutes

// States
enum TimerState {
  STOPPED,
  RUNNING,
  PAUSED
};

enum PomodoroPhase {
  POMODORO,
  SHORT_BREAK,
  LONG_BREAK
};

// Global variables
TimerState currentState = STOPPED;
PomodoroPhase currentPhase = POMODORO;
unsigned long remainingTime = POMODORO_DURATION;  // CHANGED HERE
unsigned long lastUpdateTime = 0;
int pomodoroCount = 0;
bool buttonPressed = false;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // ESP32 internal pull-up
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize RGB LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  
  Serial.println("Pomodoro Timer Ready!");
  updateRGB(); // Set initial RGB color
  updateDisplay();
}

void loop() {
  handleButton();
  updateTimer();
  delay(50); // Small delay for debouncing
}


// RGB LED Control Function - COMMON ANODE VERSION
void updateRGB() {
  // For Common Anode: HIGH = OFF, LOW = ON
  // Turn off all colors first (set all to HIGH)
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  
  // Set color based on state and phase
  if (currentState == PAUSED) {
    // Blue for paused
    digitalWrite(BLUE_PIN, LOW);  // LOW turns ON for common anode
    Serial.println("RGB: Blue (Paused)");
  } 
  else if (currentState == RUNNING) {
    if (currentPhase == POMODORO) {
      // Red for pomodoro running
      digitalWrite(RED_PIN, LOW);  // LOW turns ON for common anode
      Serial.println("RGB: Red (Pomodoro Running)");
    } else {
      // Green for break running
      digitalWrite(GREEN_PIN, LOW);  // LOW turns ON for common anode
      Serial.println("RGB: Green (Break Running)");
    }
  }
  else { // STOPPED state
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
  }
}


void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  static unsigned long lastDebounceTime = 0;
  static bool lastStableState = HIGH;
  
  // Debounce logic
  if (currentButtonState != lastStableState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > 50) {
    // State is stable
    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;
      
      if (currentButtonState == LOW) {
        // Button pressed - handle single click
        if (currentState == STOPPED) {
          startTimer();
        } else if (currentState == RUNNING) {
          pauseTimer();
        } else if (currentState == PAUSED) {
          resumeTimer();
        }
      }
    }
  }
  
  lastStableState = currentButtonState;
  
  // Long press detection (separate from debouncing)
  static unsigned long pressStartTime = 0;
  if (currentButtonState == LOW) {
    if (pressStartTime == 0) {
      pressStartTime = millis();
    } else if (millis() - pressStartTime > 2000) {
      restartTimer();
      pressStartTime = 0;
      // Wait for button release
      while(digitalRead(BUTTON_PIN) == LOW) {
        delay(50);
      }
    }
  } else {
    pressStartTime = 0;
  }
}

void startTimer() {
  currentState = RUNNING;
  lastUpdateTime = millis();
  playBuzzer();
  updateRGB(); // Update RGB color
  Serial.println("Timer started");
}

void pauseTimer() {
  currentState = PAUSED;
  playBuzzerOnce();
  updateRGB(); // Update RGB color
  Serial.println("Timer paused");
}

void resumeTimer() {
  currentState = RUNNING;
  lastUpdateTime = millis();
  playBuzzerOnce();
  updateRGB(); // Update RGB color
  Serial.println("Timer resumed");
}

void restartTimer() {
  currentState = STOPPED;
  resetTimer();
  playBuzzerOnce();
  updateRGB(); // Update RGB color
  Serial.println("Timer restarted");
}

void resetTimer() {
  switch(currentPhase) {
    case POMODORO:
      remainingTime = POMODORO_DURATION;  // CHANGED HERE
      break;
    case SHORT_BREAK:
      remainingTime = SHORT_BREAK_DURATION;  // CHANGED HERE
      break;
    case LONG_BREAK:
      remainingTime = LONG_BREAK_DURATION;  // CHANGED HERE
      break;
  }
  playBuzzer();
  updateDisplay();
}

void updateTimer() {
  if (currentState != RUNNING) return;
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdateTime >= 1000) { // Update every second
    if (remainingTime > 0) {
      remainingTime--;
      lastUpdateTime = currentTime;
      updateDisplay();
    } else {
      // Timer completed
      timerComplete();
    }
  }
}

void timerComplete() {
  currentState = STOPPED;
  playBuzzer();
  
  // Move to next phase
  switch(currentPhase) {
    case POMODORO:
      pomodoroCount++;
      if (pomodoroCount % 4 == 0) {
        currentPhase = LONG_BREAK;
        Serial.println("Pomodoro completed! Time for long break.");
      } else {
        currentPhase = SHORT_BREAK;
        Serial.println("Pomodoro completed! Time for short break.");
      }
      break;
      
    case SHORT_BREAK:
    case LONG_BREAK:
      currentPhase = POMODORO;
      Serial.println("Break completed! Time for next pomodoro.");
      break;
  }
  
  resetTimer();
  updateRGB(); // Update RGB color when phase changes
  updateDisplay();
}

void playBuzzer() {
  // Play completion sound
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void playBuzzerOnce() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }

void updateDisplay() {
  display.clearDisplay();
  
  // Display current phase
  display.setCursor(0, 0);
  display.setTextSize(2);
  
  switch(currentPhase) {
    case POMODORO:
      display.print("POMODORO");
      break;
    case SHORT_BREAK:
      display.print("BREAK");
      break;
    case LONG_BREAK:
      display.print("LONG BREAK");
      break;
  }
  
  // Display timer state
  display.setCursor(0, 17);
  display.setTextSize(1);
  switch(currentState) {
    case RUNNING:
      display.print("RUNNING");
      break;
    case PAUSED:
      display.print("PAUSED");
      break;
    case STOPPED:
      display.print("STOPPED");
      break;
  }
  
  // Display pomodoro count
  display.setCursor(70, 17);
  display.print("Count: ");
  display.print(pomodoroCount);
  
  // Display time
  display.setCursor(0, 26);
  display.setTextSize(3);
  
  int minutes = remainingTime / 60;
  int seconds = remainingTime % 60;
  
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10) display.print("0");
  display.print(seconds);
  
  // Display instructions
  display.setCursor(0, 48);
  display.setTextSize(1);
  display.print("Press:Start/Stop");

    // Display instructions
  display.setCursor(0, 57);
  display.setTextSize(1);
  display.print("Hold:Restart");
  
  display.display();
}