#include "esp_camera.h"           // ESP32 camera driver library
#include <WiFi.h>                 // WiFi library for network connectivity
#include <ArduinoWebsockets.h>    // WebSocket library for network communication

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"         // Contains camera pin definitions for specific board

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "ESP32-Camera-Server";    // WiFi network name to connect to
const char* password = "123456789";          // WiFi password

const char* websockets_server_host = "192.168.4.1";  // Server IP address (AP mode IP)
const uint16_t websockets_server_port = 8888;        // WebSocket server port number

using namespace websockets;       // Use websockets namespace
WebsocketsClient client;          // WebSocket client object for connecting to server

void setup() {
  Serial.begin(115200);           // Start serial communication at 115200 baud
  Serial.setDebugOutput(true);    // Enable ESP32 debug output to serial
  Serial.println();               // Print empty line

  // Camera configuration structure
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;    // LED control channel for XCLK
  config.ledc_timer = LEDC_TIMER_0;        // LED control timer
  // Camera data pins (D0-D7)
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  // Camera control pins
  config.pin_xclk = XCLK_GPIO_NUM;         // System clock
  config.pin_pclk = PCLK_GPIO_NUM;         // Pixel clock
  config.pin_vsync = VSYNC_GPIO_NUM;       // Vertical sync
  config.pin_href = HREF_GPIO_NUM;         // Horizontal reference
  config.pin_sccb_sda = SIOD_GPIO_NUM;     // I2C data (camera control)
  config.pin_sccb_scl = SIOC_GPIO_NUM;     // I2C clock (camera control)
  config.pin_pwdn = PWDN_GPIO_NUM;         // Power down pin
  config.pin_reset = RESET_GPIO_NUM;       // Reset pin
  
  // Camera performance settings
  config.xclk_freq_hz = 10000000;          // 10MHz XCLK frequency
  config.frame_size = FRAMESIZE_QVGA;      // 320x240 resolution
  config.pixel_format = PIXFORMAT_JPEG;    // JPEG format for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // Grab frame when buffer empty
  config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM for frame buffer
  config.jpeg_quality = 12;                // JPEG quality (0-63, lower=better)
  config.fb_count = 1;                     // Number of frame buffers

  // Optimize settings if PSRAM is available
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {                    // Check if PSRAM is available
      config.jpeg_quality = 10;            // Better quality with PSRAM
      config.fb_count = 2;                 // Use double buffering
      config.grab_mode = CAMERA_GRAB_LATEST; // Always get latest frame
    } else {
      // Limited settings without PSRAM
      config.frame_size = FRAMESIZE_QVGA;  // Keep smaller resolution
      config.fb_location = CAMERA_FB_IN_DRAM; // Use DRAM instead
    }
  } else {
    config.frame_size = FRAMESIZE_240X240; // Non-JPEG format resolution
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;                   // ESP32-S3 specific optimization
#endif
  }

  // Initialize camera with configuration
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {                     // Check if initialization succeeded
    Serial.printf("Camera init failed with error 0x%x", err);
    return;                                // Stop if camera failed
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);              // Start WiFi connection
  WiFi.setSleep(false);                    // Disable WiFi sleep for better performance

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {  // Wait for connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Display connection info
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());            // Print local IP address
  Serial.println("' to connect");

  // Connect to WebSocket server with timeout
  Serial.println("Connecting to WebSocket server...");
  unsigned long startAttemptTime = millis();         // Record start time
  const unsigned long timeout = 10000;               // 10 second timeout
  
  // Attempt connection with timeout
  while(!client.connect(websockets_server_host, websockets_server_port, "/")) {
    if (millis() - startAttemptTime > timeout) {     // Check if timeout reached
      Serial.println("WebSocket connection failed - timeout");
      break;
    }
    delay(500);
    Serial.print(".");
  }
  
  // Check connection status
  if (client.available()) {
    Serial.println("WebSocket Connected!");
  } else {
    Serial.println("WebSocket connection failed!");
  }
}


void loop() {
  // Capture frame from camera
  camera_fb_t *fb = NULL;                  // Frame buffer pointer
  esp_err_t res = ESP_OK;                  // Error code variable
  fb = esp_camera_fb_get();                // Get frame buffer from camera
  
  // Check if frame capture was successful
  if(!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);              // Return frame buffer
    return;                                 // Skip this iteration
  }

  size_t fb_len = 0;                       // Frame buffer length
  
  // Verify frame format is JPEG
  if(fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG data not implemented");
    return;                                 // Skip if not JPEG
  }

  // Send JPEG image via WebSocket as binary data
  client.sendBinary((const char*) fb->buf, fb->len);
  
  // Return frame buffer to camera for reuse
  esp_camera_fb_return(fb);
}

