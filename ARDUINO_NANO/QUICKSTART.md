# 🚂 Arduino Nano Temperature Sensor - Quick Start Guide

## What You Need
- ✅ Arduino Nano (ATmega328P)
- ✅ DS18B20 Temperature Sensor  
- ✅ 4.7kΩ Resistor (Yellow-Violet-Red)
- ✅ Breadboard + Jumper Wires
- ✅ USB Cable (Mini-B)
- ✅ Arduino IDE installed

## Circuit in 30 Seconds

```
Arduino 5V ──────┬───────► DS18B20 Pin 3 (VCC)
                 │
              4.7kΩ
                 │
Arduino D2 ──────┴───────► DS18B20 Pin 2 (DATA)

Arduino GND ─────────────► DS18B20 Pin 1 (GND)
```

**DS18B20 Pin Order** (flat side facing you):
```
   1     2     3
  GND  DATA  VCC
```

## Software Setup (5 Minutes)

### 1. Install Arduino IDE
1. Download: https://www.arduino.cc/en/software
2. Install and launch
3. Takes ~3 minutes

### 2. Install Libraries (2 minutes)
1. **Tools → Manage Libraries** (Ctrl+Shift+I)
2. Search: **"OneWire"** → Install (by Paul Stoffregen)
3. Search: **"DallasTemperature"** → Install (by Miles Burton)

### 3. Upload Code (2 minutes)
1. Open: `ARDUINO_NANO/temp_sensor/temp_sensor.ino`
2. Connect Arduino via USB
3. **Tools → Board → Arduino Nano**
4. **Tools → Processor → ATmega328P** (try "Old Bootloader" if upload fails)
5. **Tools → Port → COM# / /dev/ttyUSB#** (select your Arduino)
6. Click **Upload** button (→)

### 4. Configure Cabin Number
Edit line 21 in code:
```cpp
#define CABIN_ID 0  // Change to your cabin number (0-9)
```

### 5. Verify It Works
1. **Tools → Serial Monitor** (Ctrl+Shift+M)
2. Set baud rate: **115200** (bottom-right)
3. You should see:
```
==================================================
DS18B20 Temperature Monitor - Coach RTOS
Arduino Nano Edition
==================================================
Found 1 DS18B20 sensor(s)
[Reading 1] Cabin 0: 23.44°C
TEMP 0 23
```

**LED Indicators**:
- 3 blinks on startup = Success! ✅
- 1 blink every 5s = Reading temperature 📖
- 2 blinks every 10s = Sending data 📤
- 5 rapid blinks = HIGH TEMP EMERGENCY! ⚠️

## Raspberry Pi Setup

### 1. Connect to Raspberry Pi
Plug Arduino USB cable into Raspberry Pi USB port.

### 2. Find USB Port
```bash
ls -l /dev/ttyUSB*
# Should show: /dev/ttyUSB0 (or ttyUSB1, ttyUSB2)
```

### 3. Set Permissions
```bash
sudo chmod 666 /dev/ttyUSB0
# Or permanent solution:
sudo usermod -a -G dialout $USER
# (logout/login after this command)
```

### 4. Test Communication
```bash
cat /dev/ttyUSB0
# Should show:
# TEMP 0 23
# TEMP 0 23
# TEMP 0 24
```

### 5. Update RTOS for Arduino

Edit `RASPBERRY_PI/coach_rtos/src/usb_listener.c`:

**Find this line** (~line 35):
```c
const char* ports[] = {"/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2"};
```

**Change to**:
```c
const char* ports[] = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2"};
```

### 6. Rebuild and Run
```bash
cd ~/coach_rtos
make clean && make all
./bin/coach_rtos
```

You should see:
```
[USB0] Connected to /dev/ttyUSB0 at 115200 baud
[USB0 RX] TEMP 0 23
Cabin 0 Temperature: 23°C
```

## High Temperature Emergency Detection

### How It Works
- **Threshold**: 45°C (configurable in code)
- **Action**: Automatically sends `EMERGENCY` command every 3 seconds
- **Visual**: LED blinks rapidly (5 quick flashes)
- **RTOS**: Displays continuous emergency alerts

### Configuration
Edit in `temp_sensor.ino`:
```cpp
#define HIGH_TEMP_THRESHOLD 45  // Emergency threshold (°C)
#define HIGH_TEMP_ALERT_INTERVAL 3000  // Alert every 3 seconds (ms)
```

### Testing High Temp Alert

**Option 1**: Lower threshold temporarily
```cpp
#define HIGH_TEMP_THRESHOLD 25  // Test at room temperature
```
Re-upload code. Should trigger immediately.

**Option 2**: Use heat source
- Hair dryer (warm setting, 20-30cm away)
- Cup of hot water near sensor
- LED will blink rapidly when over threshold

### Expected Behavior
```
Normal:
[Reading 10] Cabin 0: 42.31°C
TEMP 0 35

Over 45°C:
[Reading 11] ⚠️ HIGH TEMP! Cabin 0: 46.12°C
HIGH TEMP ALERT! Cabin 0: 46°C (Threshold: 45°C)
EMERGENCY 0
[LED blinks 5 times rapidly, repeats every 3 seconds]

Returns to normal:
[Reading 15] Cabin 0: 43.81°C
Temperature returned to normal: 43.81°C
TEMP 0 35
```

## Multiple Cabins

### Hardware:
```
Raspberry Pi 4
├─ USB Port 1 → Arduino #1 (Cabin 0)
├─ USB Port 2 → Arduino #2 (Cabin 1)
├─ USB Port 3 → Arduino #3 (Cabin 2)
└─ USB Hub    → Arduino #4-10
```

### Setup Each Arduino:
1. Edit `temp_sensor.ino`: Set unique `CABIN_ID` (0-9)
2. Upload to Arduino
3. Label Arduino physically (write "Cabin 0" on tape)
4. Plug into Raspberry Pi

### Verify:
```bash
cat /dev/ttyUSB0  # Should show: TEMP 0 XX
cat /dev/ttyUSB1  # Should show: TEMP 1 XX
cat /dev/ttyUSB2  # Should show: TEMP 2 XX
```

## Troubleshooting

### "Found 0 DS18B20 sensor(s)"
- ✓ Check DS18B20 orientation (flat side: GND-DATA-VCC)
- ✓ Verify 4.7kΩ resistor between DATA and VCC
- ✓ Ensure D2 connection is solid

### Upload Failed: "stk500_getsync()"
- ✓ Close Serial Monitor before uploading
- ✓ Try: **Tools → Processor → ATmega328P (Old Bootloader)**
- ✓ Check USB cable (must support data, not power-only)
- ✓ Try different USB port

### Temperature Reads -127.00°C
- ✓ Missing 4.7kΩ pull-up resistor (critical!)
- ✓ Sensor disconnected or bad wiring
- ✓ Check 5V and GND connections

### Serial Monitor Shows Garbage
- ✓ Set baud rate to **115200**
- ✓ Wait 2-3 seconds after opening Serial Monitor
- ✓ Press Arduino reset button

### LED Stays Solid ON
- ✓ Sensor error detected
- ✓ Open Serial Monitor to see error message
- ✓ Check wiring and resistor

### Raspberry Pi: "Permission denied" on /dev/ttyUSB0
```bash
sudo chmod 666 /dev/ttyUSB0
# Or permanently:
sudo usermod -a -G dialout $USER
```

## Configuration Reference

### All Configurable Parameters:
```cpp
#define DS18B20_PIN 2           // Data pin (change if using different pin)
#define CABIN_ID 0              // Cabin number (0-9)
#define READ_INTERVAL 5000      // Read every 5 seconds (ms)
#define SEND_INTERVAL 10000     // Send every 10 seconds (ms)
#define HIGH_TEMP_THRESHOLD 45  // Emergency at 45°C
#define HIGH_TEMP_ALERT_INTERVAL 3000  // Alert every 3 seconds (ms)
#define SERIAL_BAUD 115200      // Serial communication speed
```

### Preset Configurations:

**Standard** (default):
```cpp
READ_INTERVAL 5000       // Every 5 seconds
SEND_INTERVAL 10000      // Every 10 seconds
HIGH_TEMP_THRESHOLD 45   // 45°C alert
```

**High-Sensitivity**:
```cpp
READ_INTERVAL 2000       // Every 2 seconds
SEND_INTERVAL 5000       // Every 5 seconds
HIGH_TEMP_THRESHOLD 40   // 40°C alert (earlier warning)
```

**Power-Saving**:
```cpp
READ_INTERVAL 10000      // Every 10 seconds
SEND_INTERVAL 30000      // Every 30 seconds
HIGH_TEMP_THRESHOLD 45   // 45°C alert
```

## Quick Tests

### 1. Blink Test (verify Arduino works):
```cpp
void setup() { pinMode(13, OUTPUT); }
void loop() { 
  digitalWrite(13, HIGH); delay(500);
  digitalWrite(13, LOW); delay(500);
}
```
LED should blink on/off.

### 2. Serial Test (verify USB communication):
```cpp
void setup() { Serial.begin(115200); }
void loop() {
  Serial.println("Hello from Arduino!");
  delay(1000);
}
```
Open Serial Monitor (115200 baud). Should see messages.

### 3. Sensor Test (full functionality):
Upload `temp_sensor.ino` and check Serial Monitor.

## Performance Specs

| Metric | Value |
|--------|-------|
| Temperature Range | -55°C to +125°C |
| Accuracy | ±0.5°C (-10°C to +85°C) |
| Resolution | 0.0625°C (12-bit) |
| Read Time | ~750ms |
| Update Rate | Every 5 seconds |
| Emergency Response | <5 seconds |
| Power Consumption | ~40mA (Arduino + sensor) |

## Cost

- Arduino Nano: $4
- DS18B20: $2
- Resistor + wires: $1
- USB Cable: $2
- **Total per cabin**: ~$9

## Next Steps

1. ✅ Wire circuit on breadboard
2. ✅ Install Arduino IDE and libraries
3. ✅ Upload temp_sensor.ino
4. ✅ Test with Serial Monitor
5. ✅ Connect to Raspberry Pi
6. ✅ Update usb_listener.c for /dev/ttyUSB0
7. ✅ Rebuild RTOS and test integration
8. ✅ Test high-temperature emergency
9. ✅ Deploy multiple cabins (if needed)

## Additional Documentation

- **ARDUINO_SETUP.md** - Comprehensive setup guide with detailed steps
- **WIRING_DIAGRAM.txt** - ASCII circuit diagrams and pinouts
- **temp_sensor.ino** - Arduino source code with comments
- **../README.md** - Main RTOS project documentation
- **../HIGH_TEMP_EMERGENCY.md** - Emergency detection details

## Support

**Common Issues**:
- Sensor not found → Check wiring and 4.7kΩ resistor
- Upload failed → Try "Old Bootloader" processor option
- Permission denied → `sudo chmod 666 /dev/ttyUSB0`
- Wrong readings → Verify DS18B20 orientation

**Online Resources**:
- Arduino Forum: https://forum.arduino.cc/
- OneWire Library: https://github.com/PaulStoffregen/OneWire
- DS18B20 Datasheet: https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

---

**Last Updated**: February 2026  
**Hardware**: Arduino Nano (ATmega328P)  
**Sensor**: DS18B20 Digital Temperature Sensor  
**Status**: Production Ready ✅

**Total Setup Time**: 10-15 minutes for first cabin  
**Difficulty**: Beginner-friendly  
**Prerequisites**: Basic Arduino IDE knowledge helpful but not required
