"""
Raspberry Pi Pico - DS18B20 Temperature Sensor Reader
Sends temperature data to Raspberry Pi via USB Serial

Hardware:
- Raspberry Pi Pico
- DS18B20 Temperature Sensor
- 4.7kΩ Resistor (pull-up)
- USB cable to Raspberry Pi

Connections:
DS18B20 Pin 1 (GND)   → Pico GND
DS18B20 Pin 2 (DATA)  → Pico GP2 (with 4.7kΩ pull-up to 3.3V)
DS18B20 Pin 3 (VCC)   → Pico 3.3V
4.7kΩ Resistor        → Between DATA and VCC
"""

import machine
import onewire
import ds18x20
import time
import sys

# Configuration
DS18B20_PIN = 2  # GPIO pin connected to DS18B20 data line
CABIN_ID = 0     # Which cabin this sensor monitors (0-9)
READ_INTERVAL = 5  # Read temperature every 5 seconds
SEND_INTERVAL = 10  # Send update to Pi every 10 seconds
HIGH_TEMP_THRESHOLD = 45  # Temperature threshold for emergency (°C)
HIGH_TEMP_ALERT_INTERVAL = 3  # Alert every 3 seconds when over threshold

# Initialize DS18B20
ds_pin = machine.Pin(DS18B20_PIN)
ds_sensor = ds18x20.DS18X20(onewire.OneWire(ds_pin))

# Find DS18B20 devices
print("Scanning for DS18B20 sensors...")
roms = ds_sensor.scan()

if not roms:
    print("ERROR: No DS18B20 sensor found!")
    print("Check connections:")
    print("  - DS18B20 DATA pin connected to GP2")
    print("  - 4.7kΩ pull-up resistor between DATA and 3.3V")
    print("  - VCC to 3.3V, GND to GND")
    sys.exit(1)

print(f"Found {len(roms)} DS18B20 sensor(s)")
rom = roms[0]  # Use first sensor
print(f"Sensor ROM: {rom.hex()}")

# LED indicator (built-in LED)
led = machine.Pin("LED", machine.Pin.OUT)

def read_temperature():
    """Read temperature from DS18B20 sensor"""
    try:
        ds_sensor.convert_temp()
        time.sleep_ms(750)  # Wait for conversion (max 750ms for 12-bit)
        temp_c = ds_sensor.read_temp(rom)
        return temp_c
    except Exception as e:
        print(f"Error reading temperature: {e}")
        return None

def send_to_raspberry_pi(cabin_id, temperature):
    """Send temperature command to Raspberry Pi via USB serial"""
    # Format: TEMP <cabin_id> <temperature>
    temp_int = int(round(temperature))
    
    # Send actual temperature (clamped to display range for TEMP command)
    temp_display = temp_int
    if temp_display < 10:
        temp_display = 10
    elif temp_display > 35:
        temp_display = 35
    
    command = f"TEMP {cabin_id} {temp_display}\n"
    print(command.strip())
    sys.stdout.write(command)
    sys.stdout.flush()

def send_high_temp_emergency(cabin_id, temperature):
    """Send emergency alert for high temperature"""
    temp_int = int(round(temperature))
    command = f"EMERGENCY {cabin_id}\n"
    warning = f"HIGH TEMP ALERT! Cabin {cabin_id}: {temp_int}°C (Threshold: {HIGH_TEMP_THRESHOLD}°C)"
    print(warning)
    print(command.strip())
    sys.stdout.write(command)
    sys.stdout.flush()

def blink_led(times=1, delay=0.1):
    """Blink LED to indicate activity"""
    for _ in range(times):
        led.on()
        time.sleep(delay)
        led.off()
        time.sleep(delay)

def main():
    print("\n" + "="*50)
    print("DS18B20 Temperature Monitor - Coach RTOS")
    print("="*50)
    print(f"Monitoring Cabin: {CABIN_ID}")
    print(f"Read Interval: {READ_INTERVAL}s")
    print(f"Send Interval: {SEND_INTERVAL}s")
    print("="*50)
    print("\nStarting temperature monitoring...\n")
    
    last_temp = None
    send_counter = 0
    read_count = 0
    high_temp_alert_counter = 0
    in_high_temp_state = False
    
    # Blink 3 times to indicate startup
    blink_led(3)
    
    while True:
        try:
            # Read temperature
            temp = read_temperature()
            
            if temp is not None:
                read_count += 1
                send_counter += READ_INTERVAL
                high_temp_alert_counter += READ_INTERVAL
                
                # Check for high temperature emergency
                is_high_temp = temp > HIGH_TEMP_THRESHOLD
                
                # Show reading on console (for debugging)
                if is_high_temp:
                    print(f"[Reading {read_count}] ⚠️ HIGH TEMP! Cabin {CABIN_ID}: {temp:.2f}°C")
                else:
                    print(f"[Reading {read_count}] Cabin {CABIN_ID}: {temp:.2f}°C")
                
                # Blink once on successful read
                blink_led(1, 0.05)
                
                # Handle high temperature emergency
                if is_high_temp:
                    # Send emergency alert at faster rate when temperature is high
                    if high_temp_alert_counter >= HIGH_TEMP_ALERT_INTERVAL or not in_high_temp_state:
                        send_high_temp_emergency(CABIN_ID, temp)
                        in_high_temp_state = True
                        high_temp_alert_counter = 0
                        # Rapid blink for emergency
                        blink_led(5, 0.05)
                else:
                    # Clear emergency state when temperature drops
                    if in_high_temp_state:
                        print(f"Temperature returned to normal: {temp:.2f}°C")
                        in_high_temp_state = False
                    high_temp_alert_counter = 0
                
                # Send normal temperature update
                temp_changed = last_temp is None or abs(temp - last_temp) >= 1.0
                
                if send_counter >= SEND_INTERVAL or temp_changed:
                    send_to_raspberry_pi(CABIN_ID, temp)
                    last_temp = temp
                    send_counter = 0
                    # Double blink when sending (if not in emergency)
                    if not is_high_temp:
                        blink_led(2, 0.05)
            else:
                print("Failed to read temperature - sensor may be disconnected")
                led.on()  # Keep LED on to indicate error
            
            time.sleep(READ_INTERVAL)
            
        except KeyboardInterrupt:
            print("\n\nStopping temperature monitor...")
            led.off()
            break
        except Exception as e:
            print(f"Error in main loop: {e}")
            time.sleep(1)

if __name__ == "__main__":
    main()
