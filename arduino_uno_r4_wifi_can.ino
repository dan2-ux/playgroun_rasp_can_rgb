#include <SPI.h>
#include <mcp2515.h>
#include <math.h>
#include "Arduino_LED_Matrix.h"
#include <Adafruit_NeoPixel.h>

#define PIN 6        // Pin where NeoPixel data line is connected
#define NUMPIXELS 100

ArduinoLEDMatrix matrix;
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

struct can_frame canMsg;
MCP2515 mcp2515(10);

void setup() {
  Serial.begin(115200);
  matrix.begin();
  mcp2515.reset();
  strip.begin();           // Initialize the strip
  strip.show(); 
  
  if (mcp2515.setBitrate(CAN_500KBPS) != MCP2515::ERROR_OK) {
    Serial.println("Failed to set bitrate!");
  } else {
    Serial.println("Bitrate set OK");
    if (mcp2515.setNormalMode() != MCP2515::ERROR_OK) {
      Serial.println("Failed to set normal mode!");
    } else {
      Serial.println("------- CAN Read ----------");
      Serial.println("ID  DLC   DATA");
    }
  }
}


const uint32_t happy[] = {
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0XFFFFFFFF
};

const uint32_t blank[] = {
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000  
};
int num_turn = 0;


void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");

    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    matrix.loadFrame(happy);
    delay(100);

    uint8_t check_lights = canMsg.data[0];
    uint8_t check_intent = canMsg.data[7];
    uint8_t check_r = canMsg.data[1];
    uint8_t check_g = canMsg.data[2];
    uint8_t check_b = canMsg.data[3];
    check_intent = ceil(check_intent * 255 / 10);
  
    if (check_lights == 0x01) {
      strip.setBrightness(check_intent);
      for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(check_r, check_g, check_b)); 
      }
      strip.show();
      Serial.println("LED Strip ON");
    }
    else if (check_lights == 0x00){
      
      for (int i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); 
      }
      strip.show();
      Serial.println("LED Strip OFF");
      }
    }
   matrix.loadFrame(blank);
   delay(100);
}
