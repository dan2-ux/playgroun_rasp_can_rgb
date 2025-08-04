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

## Workflow
Rasberry pi 5 will connect with Arduino UNO R4 wifi through can bus protocle using 2 MCP2515 (can transciever). One pi 5 end, pi 5 will run sdv-runtime, it will connect pi 5 with playground. On the other side, R4 wifi will connect to neo-pixels.

## Wiring
|     Pi 5     |     MCP2515     |  
|--------------|-----------------|
|      5V      |       5V        |
|      GND     |       GND       |
|      INT     |       GPIO 2    | 
|--------------|     (optional)  |
|    GPIO 11   |       SCK       |
|    GPIO 10   |       SI        |
|    GPIO 9    |       SO        |
|    GPIO 8    |       CS        |




