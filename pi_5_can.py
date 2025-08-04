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
