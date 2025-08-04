#include <SPI.h>                       // Include SPI library for communication with MCP2515 CAN controller
#include <mcp2515.h>                  // Include MCP2515 CAN controller library
#include <math.h>                     // Include math library for functions like ceil()
#include "Arduino_LED_Matrix.h"       // Include custom library for controlling LED matrix
#include <Adafruit_NeoPixel.h>        // Include Adafruit NeoPixel library for controlling RGB LED strip

#define PIN 6        // Pin where NeoPixel data line is connected
#define NUMPIXELS 100                   // Number of pixels in the NeoPixel strip

ArduinoLEDMatrix matrix;               // Create an instance of the LED matrix
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);  // Initialize NeoPixel strip with 100 pixels on PIN 6 with GRB color order and 800kHz signal

struct can_frame canMsg;               // CAN message frame structure for receiving messages
MCP2515 mcp2515(10);                   // Create MCP2515 object on SPI chip select pin 10

void setup() {
  Serial.begin(115200);                // Start serial communication at 115200 baud for debugging
  matrix.begin();                     // Initialize the LED matrix
  mcp2515.reset();                   // Reset the MCP2515 CAN controller
  strip.begin();                     // Initialize the NeoPixel strip
  strip.show();                     // Clear NeoPixel strip to off state
  
  if (mcp2515.setBitrate(CAN_500KBPS) != MCP2515::ERROR_OK) {  // Set CAN bitrate to 500kbps, check for errors
    Serial.println("Failed to set bitrate!");                  // Print error if bitrate setting fails
  } else {
    Serial.println("Bitrate set OK");                           // Confirm bitrate set success
    if (mcp2515.setNormalMode() != MCP2515::ERROR_OK) {        // Set MCP2515 to normal mode (ready to send/receive)
      Serial.println("Failed to set normal mode!");            // Print error if mode setting fails
    } else {
      Serial.println("------- CAN Read ----------");           // Print header for CAN read output
      Serial.println("ID  DLC   DATA");
    }
  }
}

const uint32_t happy[] = {             // Define a pattern (all pixels on) for LED matrix to show when CAN message received
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0XFFFFFFFF
};

const uint32_t blank[] = {             // Define a blank (all pixels off) pattern for LED matrix
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000  
};

int num_turn = 0;                      // Variable (unused in current code, possibly for future use)

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {  // Check if a CAN message is received successfully
    Serial.print(canMsg.can_id, HEX); // print CAN ID in hexadecimal
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print Data Length Code (number of bytes in message)
    Serial.print(" ");

    for (int i = 0; i < canMsg.can_dlc; i++) {               // Loop through each byte of CAN data payload
      Serial.print(canMsg.data[i], HEX);                      // Print each data byte in hex
      Serial.print(" ");
    }
    Serial.println();

    matrix.loadFrame(happy);      // Display 'happy' pattern on LED matrix to indicate message received
    delay(100);

    uint8_t check_lights = canMsg.data[0];   // First byte indicates whether to turn LED strip on/off
    uint8_t check_intent = canMsg.data[7];   // Eighth byte used for brightness level (intent)
    uint8_t check_r = canMsg.data[1];        // Second byte for red color value
    uint8_t check_g = canMsg.data[2];        // Third byte for green color value
    uint8_t check_b = canMsg.data[3];        // Fourth byte for blue color value

    check_intent = ceil(check_intent * 255 / 10);  // Scale intent value (0-10) to brightness (0-255)

    if (check_lights == 0x01) {            // If lights on command received
      strip.setBrightness(check_intent);  // Set LED strip brightness
      for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(check_r, check_g, check_b));  // Set each pixel color to RGB values
      }
      strip.show();                        // Update LED strip to show new colors
      Serial.println("LED Strip ON");     // Print status message
    }
    else if (check_lights == 0x00){        // If lights off command received
      for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));   // Turn off all pixels by setting color to black
      }
      strip.show();                        // Update LED strip to turn off LEDs
      Serial.println("LED Strip OFF");    // Print status message
    }
  }
  
  matrix.loadFrame(blank);  // Clear the LED matrix (set to blank pattern)
  delay(100);              // Short delay before next loop iteration
}

