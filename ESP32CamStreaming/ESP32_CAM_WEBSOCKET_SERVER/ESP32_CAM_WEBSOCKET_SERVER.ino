#include <SPI.h>                    // Serial Peripheral Interface library for TFT communication
#include <ArduinoWebsockets.h>      // WebSocket library for network communication
#include <WiFi.h>                   // WiFi library for network connectivity
#include <TFT_eSPI.h>               // TFT display library
#include <TJpg_Decoder.h>           // JPEG decoder library for image rendering

#define TFT_MOSI 23  // Master Out Slave In (data line from ESP32 to TFT)
#define TFT_SCLK 18  // Serial Clock (clock signal)
#define TFT_CS   15  // Chip Select (activates TFT)
#define TFT_DC    2  // Data/Command (selects data vs command mode)
#define TFT_RST   4  // Reset pin

const char* ssid = "ESP32-Camera-Server";  // WiFi network name
const char* password = "123456789";        // WiFi password

using namespace websockets;        // Use websockets namespace
WebsocketsServer server;           // WebSocket server object
WebsocketsClient client;           // WebSocket client object

TFT_eSPI tft = TFT_eSPI();        // TFT display object
bool clientConnected = false;      // Track client connection status

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop decoding if image runs off bottom of screen
  if (y >= tft.height()) return 0;

  // Render the image block to TFT (automatically handles clipping)
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to continue decoding next block
  return 1;
}

void setup() {
  Serial.begin(115200);      // Start serial communication at 115200 baud
  delay(1000);               // Wait 1 second for stabilization

  // TFT Display Initialization
  tft.begin();               // Initialize TFT display
  tft.setRotation(3);        // Set display orientation (0-3)
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // White text on black background
  tft.fillScreen(TFT_RED);   // Fill screen with red color
  tft.setSwapBytes(true);    // Swap color bytes for correct endianness

  // JPEG Decoder Configuration
  TJpgDec.setJpgScale(1);    // Set JPEG scaling factor (1 = no scaling)
  TJpgDec.setCallback(tft_output);  // Set rendering callback function

  // Access Point Setup
  Serial.println();
  Serial.println("Setting up Access Point...");
  
  // Display setup message on TFT
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Setting AP...");
  
  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  
  // Get and display IP address
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  tft.setCursor(10, 40);
  tft.print("IP: ");
  tft.println(IP);

  // WebSocket Server Setup
  server.listen(8888);       // Start listening on port 8888
  Serial.println("WebSocket server started on port 8888");
  
  // Display connection information
  tft.setCursor(10, 70);
  tft.println("Port: 8888");
  tft.setCursor(10, 100);
  tft.println("Waiting for");
  tft.setCursor(10, 120);
  tft.println("client...");
}

void loop() {
  // Check for new client connections
  if (server.poll()) {
    client = server.accept();    // Accept incoming client connection
    clientConnected = true;      // Set connection flag
    Serial.println("Client connected!");
    
    // Update display to show connected status
    tft.fillScreen(TFT_GREEN);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("CLIENT");
    tft.setCursor(10, 40);
    tft.println("CONNECTED!");
  }
  
  // Handle messages from connected client
  if (client.available() && clientConnected) {
    client.poll();  // Process WebSocket events
    
    if (client.available()) {
      WebsocketsMessage msg = client.readBlocking();  // Read incoming message

      // Basic validation - check if data looks like JPEG (minimum size)
      if (msg.length() > 100) {
        uint32_t t = millis();  // Start timing for performance measurement

        // Extract JPEG dimensions from the image data
        uint16_t w = 0, h = 0;
        TJpgDec.getJpgSize(&w, &h, (const uint8_t*)msg.c_str(), msg.length());
        Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

        // Only render if we have valid dimensions
        if (w > 0 && h > 0) {
          // Decode and display JPEG image at top-left corner (0,0)
          TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());

          // Calculate and display rendering time
          t = millis() - t;
          Serial.print(t); Serial.println(" ms");
          
        } else {
          Serial.println("Invalid JPEG dimensions");
        }
      } else {
        Serial.println("Received data too small to be JPEG");
      }
    }
  } else if (!client.available() && clientConnected) {
    // Handle client disconnection
    clientConnected = false;
    Serial.println("Client disconnected!");
    
    // Update display to show disconnected status
    tft.fillScreen(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("CLIENT");
    tft.setCursor(10, 40);
    tft.println("DISCONNECTED!");
    delay(2000);  // Show message for 2 seconds
    
    // Return to waiting state
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Waiting for");
    tft.setCursor(10, 40);
    tft.println("client...");
  }
  
  delay(10);  // Small delay to prevent overwhelming the processor
}