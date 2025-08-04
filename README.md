# dreamKIT to neo-pixel with can.

In readme file, you will learn how to set up the connection to control NeoPixels through can-bus.

## Hardware Requirements

- Raspberry Pi 5
- NeoPixel RGB LED WS2812
- Jumper wires
- Arduino Uno R4 wifi
- 2 MCP2515

## Software Requirements

- Thonny IDE
- Libraries:
  - `kuksa_client`
  - `docker`
  - `https://github.com/autowp/arduino-mcp2515`
  - `Adafruit_NeoPixel`
  - `Arduino_LED_Matrix`
  - `arduinoIDE`

## Workflow
Rasberry pi 5 will connect with Arduino UNO R4 wifi through can bus protocle using 2 MCP2515 (can transciever). One pi 5 end, pi 5 will run sdv-runtime, it will connect pi 5 with playground. On the other side, R4 wifi will connect to neo-pixels.

## Wiring
### Wiring for Raspberry pi 5 and MCP2515.

|     Pi 5     |     MCP2515     |  
|--------------|-----------------|
|      5V      |       5V        |
|      GND     |       GND       |
|      INT     |       GPIO 2    | 
|    GPIO 11   |       SCK       |
|    GPIO 10   |       SI        |
|    GPIO 9    |       SO        |
|    GPIO 8    |       CS        |

###  Wiring for Arduino UNO R4 WIFI and MCP2515.

|      R4 WIFI     |     MCP2515     |
|        3.3V      |       5V        |
|        GND       |       GND       |
|        Pin 2     |       INT       |
|        Pin 13    |       SCK       |
|        Pin 11    |       SI        |
|        Pin 12    |       SO        |
|        Pin 10    |       CS        |

### Wiring for Arduino UNO R4 WIFI and neo-pixels.

|     R4 WIFI     |     NEO-PIXELS     |
|       5V        |         5V         |
|       GND       |         GND        |
|       Pin 6     |         Din        |

### Wiring between 2 MCP2515.

|     MCP2515     |     MCP2515     |
|      canH       |       canH      |
|      canL       |       canL      |

## Step-by-Step Guide

### Step 1: Run the sdv-runtime natively on Pi 5

## Docker Run Command

To start the SDV runtime, use the following command:

<pre>  docker run -it --rm -e RUNTIME_NAME="KKK" -p 55555:55555 --name sdv-runtime ghcr.io/eclipse-autowrx/sdv-runtime:latest  </pre>

### Step 2: Configure Pi 5 status

#### Enable SPI

1. Open terminal and run:

<pre> sudo raspi-config </pre>

Choose **interface option** -> **SPI** -> **enable**.
Then, reopen terminal then execute

<pre> sudo nano /boot/config.txt </pre>
 
Add **dtparam=spi=on** then press **ctrl + s** to save and **ctrl + x** to leave.
Make sure to reboot to save all the changes.
<pre>
  dtoverlay=mcp2315-can0,oscillator=16000000,interrupt=25
  dtoverlay=spi-bcm2835
</pre>

then press **ctrl + s** to save and **ctrl + x** to leave.
Make sure to reboot to save all the changes.

### Step 3: create a virtual environment by executing.

<pre>  
#create virtual enviroment         
python3 -m venv venv               

#activate the virtual environment  
source ./venv/bin/activate         
</pre>

Then install all the dependencies:

<pre>
# Upgrade pip first 
pip3 install --upgrade pip 

# Install kuksa_client (may require git or custom installation if not on PyPI) 
pip3 install kuksa-client 
  
</pre>

**After that:** If you using Thonny, then open it and choose **Tools**, scroll down and click on **Options…** . Then go to **Interpreter** and choose you **Python executable** which is your newly create virtual environment. When finish click **OK** to save.
Then paste the code in **pi_5_can.py**.

<pre>

import asyncio
import can
from kuksa_client.grpc.aio import VSSClient
import math

# Initialize CAN bus
can_bus = can.interface.Bus(channel='can0', bustype='socketcan')

last_state = None
intent_state = 0.0
color_state = ""

def parse_color(color_str: str):
    """Convert a hex color string like '#FFAABB' to (r, g, b)."""
    color_str = color_str.lstrip('#')  # Remove '#' if present
    if len(color_str) == 8:
        color_str = color_str[2:]  # Ignore alpha if present
    elif len(color_str) != 6:
        return (0, 0, 0)  # Default to black on error
    r = int(color_str[0:2], 16)
    g = int(color_str[2:4], 16)
    b = int(color_str[4:6], 16)
    return (r, g, b)

async def get_state(client):
    ambient = await client.get_current_values([
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.IsLightOn',
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Intensity',
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Color'
    ])
    return (
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.IsLightOn'),
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Intensity'),
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Color')
    )

async def send_can_message(is_light_on: bool, intent_value: float, color_str: str):
    led_state = 0x01 if is_light_on else 0x00
    intent_scaled = math.ceil(intent_value /10 )

    r, g, b = parse_color(color_str)

    # CAN data: [led, red, green, blue, 0, 0, 0, intent]
    data = [led_state, r, g, b, 0x00, 0x00, 0x00, intent_scaled]

    msg = can.Message(arbitration_id=0x123, data=data, is_extended_id=False)
    try:
        can_bus.send(msg)
        print(f"CAN message sent → Light: {'ON' if is_light_on else 'OFF'} | Intensity: {intent_scaled * 10}% | Color: {color_str}")
    except can.CanError as e:
        print(f"Failed to send CAN message: {e}")

async def monitor_ambient_light():
    global last_state, intent_state, color_state
    async with VSSClient('localhost', 55555) as client:
        while True:
            state_detect, inten_detect, color_detect = await get_state(client)

            state = getattr(state_detect, 'value', False) if state_detect else False
            inten = getattr(inten_detect, 'value', 0.0) if inten_detect else 0.0
            color = getattr(color_detect, 'value', '#FFFFFF') if color_detect else '#FFFFFF'

            if state != last_state or inten != intent_state or color != color_state:
                await send_can_message(state, inten, color)
                last_state = state
                intent_state = inten
                color_state = color

            await asyncio.sleep(0.1)

def main():
    asyncio.run(monitor_ambient_light())

if __name__ == "__main__":
    main()

  
</pre>

For Arduino UNO R4, you have to install the zip file in the following link.
<pre> https://github.com/autowp/arduino-mcp2515 </pre>

Then open **arduinoIDE**, choose **Sketch** -> **Include Library** -> **Add .ZIP Library...**, then choose the zip file you just install.
Finally, execute the code in **arduino_uno_r4_wifi_can.ino**.

<pre>
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

</pre>

