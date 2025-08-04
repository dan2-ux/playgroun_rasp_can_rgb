import asyncio                      # Import asyncio for asynchronous programming
import can                         # Import python-can for CAN bus communication
from kuksa_client.grpc.aio import VSSClient  # Import asynchronous VSSClient for vehicle signal specification access
import math                        # Import math library for math functions

# Initialize CAN bus
can_bus = can.interface.Bus(channel='can0', bustype='socketcan')  # Create CAN bus interface on 'can0' with socketcan driver

last_state = None                  # Store last known light on/off state
intent_state = 0.0                # Store last known intensity value
color_state = ""                  # Store last known color string

def parse_color(color_str: str):
    """Convert a hex color string like '#FFAABB' to (r, g, b)."""
    color_str = color_str.lstrip('#')  # Remove '#' if present
    if len(color_str) == 8:             # If color includes alpha channel (e.g. '#AARRGGBB')
        color_str = color_str[2:]       # Ignore alpha by taking last 6 chars (RRGGBB)
    elif len(color_str) != 6:
        return (0, 0, 0)                # Return black if invalid format
    r = int(color_str[0:2], 16)         # Parse red component
    g = int(color_str[2:4], 16)         # Parse green component
    b = int(color_str[4:6], 16)         # Parse blue component
    return (r, g, b)                    # Return RGB tuple

async def get_state(client):
    ambient = await client.get_current_values([    # Request current ambient light values from VSS
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.IsLightOn',
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Intensity',
        'Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Color'
    ])
    return (
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.IsLightOn'),  # Boolean if light is on
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Intensity'),  # Intensity value
        ambient.get('Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Color')       # Color string
    )

async def send_can_message(is_light_on: bool, intent_value: float, color_str: str):
    led_state = 0x01 if is_light_on else 0x00   # Convert light state to 0x01 (on) or 0x00 (off)
    intent_scaled = math.ceil(intent_value / 10)  # Scale intensity (0-100) down to 0-10 and ceil

    r, g, b = parse_color(color_str)             # Parse hex color string to RGB values

    # CAN data: [led_state, red, green, blue, 0, 0, 0, intent_scaled]
    data = [led_state, r, g, b, 0x00, 0x00, 0x00, intent_scaled]

    msg = can.Message(arbitration_id=0x123, data=data, is_extended_id=False)  # Create CAN message with ID 0x123
    try:
        can_bus.send(msg)                         # Send CAN message on the bus
        print(f"CAN message sent → Light: {'ON' if is_light_on else 'OFF'} | Intensity: {intent_scaled * 10}% | Color: {color_str}")
    except can.CanError as e:
        print(f"Failed to send CAN message: {e}")  # Print error on failure

async def monitor_ambient_light():
    global last_state, intent_state, color_state
    async with VSSClient('localhost', 55555) as client:  # Connect to VSS client asynchronously
        while True:
            state_detect, inten_detect, color_detect = await get_state(client)  # Get current light states

            state = getattr(state_detect, 'value', False) if state_detect else False  # Extract boolean value safely
            inten = getattr(inten_detect, 'value', 0.0) if inten_detect else 0.0     # Extract intensity safely
            color = getattr(color_detect, 'value', '#FFFFFF') if color_detect else '#FFFFFF'  # Extract color safely

            # Check if any state has changed since last update
            if state != last_state or inten != intent_state or color != color_state:
                await send_can_message(state, inten, color)  # Send updated CAN message
                last_state = state
                intent_state = inten
                color_state = color

            await asyncio.sleep(0.1)  # Sleep briefly before next poll

def main():
    # Entry point for running the asyncio event loop (not shown here)
    pass
