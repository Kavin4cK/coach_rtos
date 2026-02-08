# Temperature Sensor Integration Guide

## System Overview

Your RTOS Coach Control System now includes **real-time temperature monitoring** using:
- **Raspberry Pi Pico** running MicroPython
- **DS18B20** digital temperature sensor
- **USB connection** to Raspberry Pi (appears as `/dev/ttyACM0`)

---

## Quick Start

### 1. Hardware Setup

**Components:**
- Raspberry Pi Pico
- DS18B20 Temperature Sensor
- 4.7kΩ Resistor
- Breadboard and jumper wires
- USB cable (Micro USB)

**Circuit:**
```
Pico 3.3V ──┬─────────────── DS18B20 VCC (Pin 3)
            │
         4.7kΩ
            │
Pico GP2 ───┴─────────────── DS18B20 DATA (Pin 2)

Pico GND ───────────────────── DS18B20 GND (Pin 1)
```

### 2. Software Setup on Pico

**Install MicroPython:**
1. Download from: https://micropython.org/download/rp2-pico/
2. Hold BOOTSEL button on Pico while connecting USB
3. Copy .uf2 file to RPI-RP2 drive
4. Pico reboots with MicroPython

**Upload Temperature Sensor Code:**
1. Open Thonny IDE (https://thonny.org/)
2. Select "MicroPython (Raspberry Pi Pico)" interpreter
3. Open `RASPBERRY_PI_PICO/temp_sensor.py`
4. Save to Pico as `main.py` (File → Save As → Raspberry Pi Pico)
5. Click Run button (or press F5)

**Configure Cabin ID:**
Edit `main.py` on Pico:
```python
CABIN_ID = 0  # Change to cabin number (0-9)
```

### 3. Connect to Raspberry Pi

**Physical Connection:**
1. Connect Pico USB cable to Raspberry Pi USB port
2. Verify device appears:
   ```bash
   ls -l /dev/ttyACM0
   ```

**Permissions:**
```bash
sudo chmod 666 /dev/ttyACM0
# Or permanent:
sudo usermod -a -G dialout $USER
```

### 4. Run RTOS System

```bash
cd ~/coach_rtos
make clean && make all
./bin/coach_rtos
```

You should see:
```
[USB0] Connected to /dev/ttyACM0 at 115200 baud
[USB0 RX] TEMP 0 23
[USB0] OK: Temperature set
```

---

## How It Works

### Data Flow

```
DS18B20 Sensor
    ↓ (OneWire)
Raspberry Pi Pico (MicroPython)
    ↓ (USB Serial 115200 baud)
Raspberry Pi USB Port (/dev/ttyACM0)
    ↓ (USB Listener Thread)
RTOS Task Scheduler
    ↓ (Temperature Regulation Task)
System State Update
    ↓ (Display Task)
Terminal/Framebuffer Display
```

### Message Protocol

**Pico → Raspberry Pi:**
```
TEMP <cabin_id> <temperature>\n
```

Example:
```
TEMP 0 23
TEMP 0 24
TEMP 0 25
```

**Raspberry Pi → Pico:**
```
OK: Temperature set
```

### Update Frequency

- **Read**: Every 5 seconds from DS18B20
- **Send**: Every 10 seconds OR when temperature changes ±1°C
- **Display**: Updated by Display Task (Priority 2)

---

## LED Indicators on Pico

| Pattern | Meaning |
|---------|---------|
| 3 quick blinks | Startup - sensor found |
| 1 short blink | Temperature read successful |
| 2 short blinks | Data sent to Raspberry Pi |
| Solid ON | Error - sensor disconnected |

---

## Testing

### Test 1: Verify Pico Connection
```bash
# On Raspberry Pi
ls -l /dev/ttyACM*
# Should show: /dev/ttyACM0

# Read raw data from Pico
cat /dev/ttyACM0
# Should show: TEMP 0 23 (etc.)
```

### Test 2: Interactive Test
```bash
# On Raspberry Pi
screen /dev/ttyACM0 115200
# Should see temperature updates every 10 seconds
# Press Ctrl+A then K to exit
```

### Test 3: Full System Test
1. Start coach_rtos
2. Touch DS18B20 sensor with finger
3. Watch temperature increase in display
4. Should see temperature regulation messages

---

## Multiple Sensors (Multi-Cabin Setup)

### Hardware
- Connect multiple Picos to different USB ports
- Each Pico monitors one cabin

### Configuration
**Pico 1 (Cabin 0):**
```python
CABIN_ID = 0
```
Connects to: `/dev/ttyACM0`

**Pico 2 (Cabin 1):**
```python
CABIN_ID = 1
```
Connects to: `/dev/ttyACM1`

**Pico 3 (Cabin 2):**
```python
CABIN_ID = 2
```
Connects to: `/dev/ttyACM2`

### Update Code
Modify `usb_listener.c` to add more ACM ports:
```c
const char *devices[MAX_USB_PORTS] = {
    "/dev/ttyACM0",  // Cabin 0
    "/dev/ttyACM1",  // Cabin 1
    "/dev/ttyACM2"   // Cabin 2
};
```

---

## Troubleshooting

### Pico Not Detected

**Check device:**
```bash
ls -l /dev/ttyACM*
# If nothing, check:
dmesg | tail -20
```

**Common causes:**
- USB cable is charge-only (needs data cable)
- Pico not running MicroPython
- Pico crashed (reset by unplugging)

**Solution:**
```bash
# Reconnect Pico
sudo dmesg -w
# Watch for: usb 1-1: new full-speed USB device
# Should create: /dev/ttyACM0
```

### Permission Denied

**Error:**
```
[USB0] Failed to open /dev/ttyACM0: Permission denied
```

**Solution:**
```bash
sudo chmod 666 /dev/ttyACM0
# Or permanent:
sudo usermod -a -G dialout $USER
sudo reboot
```

### No Temperature Readings

**Check Pico is running:**
```bash
cat /dev/ttyACM0
# Should see: TEMP 0 XX
```

**If blank:**
1. Open Thonny
2. Connect to Pico
3. Check for error messages
4. Verify DS18B20 connections
5. Check 4.7kΩ pull-up resistor

### Wrong Temperature Values

**Too high/low:**
- Check sensor is not touching hot/cold surfaces
- DS18B20 range: -55°C to +125°C
- Code clamps to 10-35°C for safety

**Erratic readings:**
- Check pull-up resistor is exactly 4.7kΩ
- Check wire connections are solid
- Try shorter wires (< 1 meter for testing)

### Pico Keeps Resetting

**Check power:**
```bash
dmesg | grep "usb disconnect"
# If frequent, power supply issue
```

**Solution:**
- Use quality USB cable
- Powered USB hub if using many Picos
- Check Raspberry Pi power supply (5V 3A minimum)

---

## Temperature Sensor Accuracy

**DS18B20 Specifications:**
- Accuracy: ±0.5°C from -10°C to +85°C
- Resolution: 0.0625°C (12-bit)
- Conversion time: 750ms (12-bit mode)
- Range: -55°C to +125°C

**In This Application:**
- Rounded to nearest °C for display
- Valid range: 10-35°C (coach comfort range)
- Updates every 10 seconds or on ±1°C change

---

## Physical Installation Tips

**For Real Railway Coach:**

1. **Sensor Placement:**
   - Mount at passenger height (~1.5m from floor)
   - Away from direct sunlight
   - Away from AC vents (false readings)
   - Central location in cabin

2. **Wiring:**
   - Use shielded 3-wire cable
   - Maximum length: 20 meters (with 4.7kΩ pull-up)
   - For longer: use 1.5kΩ pull-up
   - Strain relief at connector

3. **Pico Enclosure:**
   - Small project box (IP65 rated)
   - Ventilation holes (if needed)
   - Label with cabin number
   - Easy access for maintenance

4. **Cable Management:**
   - Secure along coach frame
   - P-clips or cable ties
   - Protect from passenger access
   - Waterproof cable glands

---

## Advanced: Long Distance Sensors

**For Extended Range (>20m):**

Use stronger pull-up resistor:
```
Distance    Pull-up
20m         4.7kΩ
50m         2.2kΩ
100m        1.5kΩ
```

Or use DS18B20 in parasitic mode:
```python
# Not shown in basic example
# Requires timing changes
```

Or use multiple sensors on same bus:
```python
# Each sensor has unique ROM address
# OneWire bus supports up to 127 devices
```

---

## Files Included

**Raspberry Pi Pico:**
- `temp_sensor.py` - MicroPython code
- `PICO_SETUP.md` - Detailed setup guide

**Raspberry Pi:**
- `usb_listener.c` - Updated for /dev/ttyACM0
- `main.c` - Updated help text

**Documentation:**
- `TEMPERATURE_SENSOR_INTEGRATION.md` - This file

---

## Next Steps

1. ✅ Build circuit on breadboard
2. ✅ Upload code to Pico
3. ✅ Test with single sensor
4. 🔲 Add more Picos for other cabins
5. 🔲 Create permanent circuit on PCB
6. 🔲 Install in coach enclosures
7. 🔲 Add calibration/offset per sensor

Enjoy your real-time temperature monitoring system! 🌡️🚂
