# Arduino Nano Temperature Sensor Setup Guide

## Overview
This guide will help you set up an Arduino Nano with a DS18B20 temperature sensor to monitor cabin temperatures and automatically detect high-temperature emergencies for the Coach RTOS system.

## Table of Contents
1. [Hardware Requirements](#hardware-requirements)
2. [Software Requirements](#software-requirements)
3. [Circuit Assembly](#circuit-assembly)
4. [Arduino IDE Setup](#arduino-ide-setup)
5. [Upload Code](#upload-code)
6. [Configuration](#configuration)
7. [Testing](#testing)
8. [Raspberry Pi Integration](#raspberry-pi-integration)
9. [Troubleshooting](#troubleshooting)

---

## Hardware Requirements

### Per Cabin Sensor:
| Component | Quantity | Specifications | Approx. Cost |
|-----------|----------|----------------|--------------|
| Arduino Nano | 1 | ATmega328P, USB Mini-B | $3-5 |
| DS18B20 Temperature Sensor | 1 | Digital, OneWire, -55°C to +125°C | $2-3 |
| 4.7kΩ Resistor | 1 | Pull-up resistor (Yellow-Violet-Red) | $0.10 |
| Breadboard | 1 | 400 or 830 points (for prototyping) | $2 |
| Jumper Wires | 5-10 | Male-to-Male | $1 |
| USB Cable | 1 | Mini-B to USB-A | $2 |

**Total Cost per Sensor**: ~$10-13

### Alternative: Pre-wired DS18B20
- Some DS18B20 modules come with built-in pull-up resistor
- Look for "DS18B20 module" (3-pin: VCC, DATA, GND)
- Slightly more expensive but easier to wire

---

## Software Requirements

### Arduino IDE
1. **Download**: https://www.arduino.cc/en/software
2. **Version**: 1.8.19 or newer (or Arduino IDE 2.x)
3. **OS**: Windows, macOS, or Linux

### Required Libraries
Install these through Arduino IDE Library Manager:

1. **OneWire** by Paul Stoffregen
   - Handles 1-Wire protocol for DS18B20
   - Version 2.3.7 or newer

2. **DallasTemperature** by Miles Burton
   - Simplifies DS18B20 temperature reading
   - Version 3.9.0 or newer

**Installation Steps**:
```
Arduino IDE → Tools → Manage Libraries
Search: "OneWire" → Install
Search: "DallasTemperature" → Install
```

---

## Circuit Assembly

### Pin Connections

#### DS18B20 Pins (looking at flat side):
```
   ┌─────────────┐
   │   DS18B20   │
   │   (TO-92)   │
   │             │
   │  |||        │  (Flat side)
   └──┬┬┬────────┘
      123
```

| DS18B20 Pin | Name | Connect To |
|-------------|------|------------|
| 1 (Left) | GND | Arduino GND |
| 2 (Middle) | DATA | Arduino D2 + 4.7kΩ to 5V |
| 3 (Right) | VCC | Arduino 5V |

#### Arduino Nano Pinout:
```
Arduino Nano pins used:
- D2  : DS18B20 DATA line
- 5V  : Power to DS18B20 VCC
- GND : Ground to DS18B20 GND
- D13 : Built-in LED (automatic indicator)
```

### Complete Circuit Diagram

```
Arduino Nano                      DS18B20 Temperature Sensor
┌──────────────┐                 ┌───────────────┐
│              │                 │               │
│  5V  ────────┼─────┬───────────┤ VCC (Pin 3)   │
│              │     │           │               │
│              │  4.7kΩ          │               │
│              │     │           │               │
│  D2  ────────┼─────┴───────────┤ DATA (Pin 2)  │
│              │                 │               │
│  GND ────────┼─────────────────┤ GND (Pin 1)   │
│              │                 │               │
│  D13 (LED)   │                 └───────────────┘
│              │                   
│  USB ────────┼──► To Raspberry Pi
│              │
└──────────────┘
```

### Breadboard Layout (Top View)

```
      Arduino Nano
      ┌───────────┐
      │ ●●●····●●● │
      │ ●●●····●●● │
      └─┬─┬─┬─┬─┬─┘
        │ │ │ │ │
  ──────┘ │ │ │ └────── (Other pins)
 5V       │ │ └──────── GND
          │ └────────── D2
          (USB Mini-B)

Breadboard:
─────────────────────────────────
Rail (+) Red   : 5V ────┐
                        │
                     4.7kΩ
                        │
Row 10        : D2 ─────┴──── DS18B20 Pin 2 (DATA)
Row 11        : 5V ─────────── DS18B20 Pin 3 (VCC)
Row 12        : GND ────────── DS18B20 Pin 1 (GND)
                        
Rail (-) Blue  : GND
─────────────────────────────────
```

### Assembly Steps:

1. **Place Arduino Nano on breadboard** (straddle the center gap)
2. **Insert DS18B20** into breadboard (3 pins in consecutive rows)
3. **Connect GND**: Arduino GND → DS18B20 Pin 1 (jumper wire)
4. **Connect VCC**: Arduino 5V → DS18B20 Pin 3 (jumper wire)
5. **Connect DATA**: Arduino D2 → DS18B20 Pin 2 (jumper wire)
6. **Add Pull-up Resistor**: 4.7kΩ between DATA (Pin 2) and VCC (Pin 3)
7. **Double-check polarity** (DS18B20 pin order is critical!)

### Visual Verification Checklist:
- [ ] DS18B20 flat side facing you when reading pin 1-2-3 left-to-right
- [ ] 4.7kΩ resistor bridging DATA and VCC lines
- [ ] Arduino powered via USB (red LED on Arduino should light up)
- [ ] No short circuits between adjacent pins

---

## Arduino IDE Setup

### Step 1: Install Arduino IDE
1. Download from https://www.arduino.cc/en/software
2. Install for your operating system
3. Launch Arduino IDE

### Step 2: Configure Board
1. **Connect Arduino Nano** to your computer via USB
2. In Arduino IDE:
   - **Tools → Board → Arduino AVR Boards → Arduino Nano**
3. Select Processor:
   - **Tools → Processor → ATmega328P** (most common)
   - If upload fails, try: **ATmega328P (Old Bootloader)**
4. Select Port:
   - **Tools → Port → COM# (Arduino Nano)** (Windows)
   - **Tools → Port → /dev/ttyUSB#** (Linux)
   - **Tools → Port → /dev/cu.usbserial-####** (macOS)

### Step 3: Install Libraries
1. **Tools → Manage Libraries** (or Ctrl+Shift+I)
2. Search: **"OneWire"**
   - Author: Paul Stoffregen
   - Click **Install**
3. Search: **"DallasTemperature"**
   - Author: Miles Burton
   - Click **Install**
4. Close Library Manager

### Step 4: Test Setup (Optional)
Run a simple blink test to verify Arduino works:
```cpp
void setup() {
  pinMode(13, OUTPUT);
}
void loop() {
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
}
```
Upload this code. LED should blink. If it works, Arduino is ready!

---

## Upload Code

### Step 1: Open Sketch
1. Navigate to: `COACH_RTOS/ARDUINO_NANO/temp_sensor/`
2. Open: `temp_sensor.ino` in Arduino IDE
3. Or: **File → Open** → Browse to the .ino file

### Step 2: Configure Cabin ID
Edit line 21 in the code:
```cpp
#define CABIN_ID 0  // Change to your cabin number (0-9)
```

**For multiple cabins**, use different IDs:
- Cabin 0: `#define CABIN_ID 0`
- Cabin 1: `#define CABIN_ID 1`
- Cabin 2: `#define CABIN_ID 2`
- etc.

### Step 3: Verify Code
1. Click **Verify** button (✓) or **Sketch → Verify/Compile**
2. Wait for compilation (10-15 seconds)
3. Check for "Done compiling" message
4. Should show: `Sketch uses XXXX bytes (XX%) of program storage space`

### Step 4: Upload to Arduino
1. Click **Upload** button (→) or **Sketch → Upload**
2. Wait for upload (15-20 seconds)
3. You'll see:
   ```
   Compiling sketch...
   Uploading...
   avrdude: writing flash...
   avrdude done. Thank you.
   ```
4. Arduino will reset automatically

### Step 5: Verify Upload
1. **Tools → Serial Monitor** (or Ctrl+Shift+M)
2. Set baud rate: **115200** (bottom-right dropdown)
3. You should see:
   ```
   ==================================================
   DS18B20 Temperature Monitor - Coach RTOS
   Arduino Nano Edition
   ==================================================
   Monitoring Cabin: 0
   High Temp Threshold: 45°C
   ==================================================
   Found 1 DS18B20 sensor(s)
   Sensor Address: 28:AA:BB:CC:DD:EE:FF:00
   Starting temperature monitoring...
   
   [Reading 1] Cabin 0: 23.44°C
   TEMP 0 23
   [Reading 2] Cabin 0: 23.50°C
   ```

**LED Behavior**:
- **3 blinks** on startup (success!)
- **1 blink** every 5 seconds (reading temperature)
- **2 blinks** when sending data to Raspberry Pi
- **5 rapid blinks** = HIGH TEMP EMERGENCY!

---

## Configuration

### Editable Parameters (top of temp_sensor.ino):

```cpp
#define DS18B20_PIN 2           // Change if using different pin
#define CABIN_ID 0              // Set unique ID per cabin (0-9)
#define READ_INTERVAL 5000      // Temperature read frequency (ms)
#define SEND_INTERVAL 10000     // Update send frequency (ms)
#define HIGH_TEMP_THRESHOLD 45  // Emergency threshold (°C)
#define HIGH_TEMP_ALERT_INTERVAL 3000  // Alert frequency when hot (ms)
#define SERIAL_BAUD 115200      // Must match Raspberry Pi setting
```

### Recommended Settings by Use Case:

**Standard Operation** (default):
```cpp
READ_INTERVAL 5000            // Read every 5 seconds
SEND_INTERVAL 10000           // Send every 10 seconds
HIGH_TEMP_THRESHOLD 45        // Alert at 45°C
```

**High-Sensitivity Monitoring**:
```cpp
READ_INTERVAL 2000            // Read every 2 seconds
SEND_INTERVAL 5000            // Send every 5 seconds
HIGH_TEMP_THRESHOLD 40        // Alert at 40°C (earlier warning)
```

**Power-Saving Mode**:
```cpp
READ_INTERVAL 10000           // Read every 10 seconds
SEND_INTERVAL 30000           // Send every 30 seconds
HIGH_TEMP_THRESHOLD 45        // Standard threshold
```

---

## Testing

### Basic Functionality Test

1. **Open Serial Monitor** (Tools → Serial Monitor, 115200 baud)
2. **Observe output**:
   ```
   [Reading 1] Cabin 0: 22.31°C
   TEMP 0 22
   [Reading 2] Cabin 0: 22.44°C
   [Reading 3] Cabin 0: 22.56°C
   TEMP 0 23
   ```
3. **Watch LED**: Should blink 1x every 5 seconds, 2x every 10 seconds

### Temperature Accuracy Test

1. **Room temperature**: Should read 20-25°C in typical environment
2. **Hand warmth**: Gently hold sensor (should rise to 28-32°C)
3. **Ice water**: Place sensor in ice water (should drop to ~0-5°C)
4. **Hot water**: Place in warm water (should rise to water temp)

**⚠️ WARNING**: DS18B20 max temperature is 125°C. Don't use boiling water!

### High Temperature Emergency Test

**Option 1: Lower threshold temporarily**
```cpp
#define HIGH_TEMP_THRESHOLD 25  // Test at room temperature
```
Re-upload code. Should immediately trigger:
```
[Reading X] ⚠️ HIGH TEMP! Cabin 0: 26.12°C
HIGH TEMP ALERT! Cabin 0: 26°C (Threshold: 25°C)
EMERGENCY 0
```
LED should blink rapidly (5 quick flashes).

**Option 2: Use heat source**
- Hair dryer (warm setting, 20-30cm away)
- Cup of hot water near sensor
- Hand warmth (may take 30-60 seconds)

**Expected Emergency Behavior**:
```
Normal operation:
[Reading 10] Cabin 0: 42.31°C
TEMP 0 35

Temperature exceeds 45°C:
[Reading 11] ⚠️ HIGH TEMP! Cabin 0: 46.12°C
HIGH TEMP ALERT! Cabin 0: 46°C (Threshold: 45°C)
EMERGENCY 0
[LED blinks 5 times rapidly]

(3 seconds later)
[Reading 12] ⚠️ HIGH TEMP! Cabin 0: 47.25°C
HIGH TEMP ALERT! Cabin 0: 47°C (Threshold: 45°C)
EMERGENCY 0
[LED blinks 5 times rapidly]

Temperature returns to normal:
[Reading 15] Cabin 0: 43.81°C
Temperature returned to normal: 43.81°C
TEMP 0 35
```

### Communication Test

**Windows**:
```powershell
# Find Arduino port
Get-PnpDevice -Class Ports | Where-Object {$_.Status -eq "OK"}

# Read serial data (PowerShell)
$port = New-Object System.IO.Ports.SerialPort COM3,115200,None,8,One
$port.Open()
while ($true) {
    $line = $port.ReadLine()
    Write-Host $line
}
```

**Linux/Raspberry Pi**:
```bash
# Check port
ls -l /dev/ttyUSB0

# Set permissions
sudo chmod 666 /dev/ttyUSB0

# Read data
cat /dev/ttyUSB0
# Should show: TEMP 0 23, TEMP 0 24, etc.
```

---

## Raspberry Pi Integration

### Step 1: Connect Arduino to Raspberry Pi
1. Plug Arduino Nano into Raspberry Pi USB port
2. Arduino will appear as `/dev/ttyUSB0` (or ttyUSB1, ttyUSB2, etc.)

### Step 2: Verify USB Connection
```bash
# List USB devices
lsusb
# Should show: "QinHeng Electronics HL-340 USB-Serial adapter"
# or "FTDI FT232 USB-UART"

# Check serial port
ls -l /dev/ttyUSB*
# Should show: /dev/ttyUSB0

# View port details
dmesg | grep tty
# Should show: "USB Serial device"
```

### Step 3: Set Permissions
```bash
# One-time permission (resets on reboot)
sudo chmod 666 /dev/ttyUSB0

# Permanent solution: Add user to dialout group
sudo usermod -a -G dialout $USER
# Logout and login for changes to take effect
```

### Step 4: Test Communication
```bash
# Read serial data
cat /dev/ttyUSB0

# Expected output:
# TEMP 0 23
# TEMP 0 23
# TEMP 0 24
# (updates every 10 seconds)
```

### Step 5: Update RTOS USB Listener

Edit `usb_listener.c` to use `/dev/ttyUSB0` instead of `/dev/ttyACM0`:

```c
// For Arduino Nano (USB-Serial adapter)
const char* ports[] = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2"};
```

See "Update usb_listener.c for Arduino port" section in main README.

### Step 6: Rebuild and Run RTOS
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

---

## Troubleshooting

### Sensor Not Found
```
Found 0 DS18B20 sensor(s)
ERROR: No DS18B20 sensors found!
```

**Solutions**:
1. Check wiring (especially DATA pin to D2)
2. Verify 4.7kΩ pull-up resistor is connected
3. Check DS18B20 polarity (Pin 1=GND, 2=DATA, 3=VCC)
4. Try different DS18B20 (might be defective)
5. Change `DS18B20_PIN` to different pin (e.g., D3, D4)

### Upload Failed
```
avrdude: stk500_getsync(): not in sync
```

**Solutions**:
1. Check USB cable (some are power-only, need data cable)
2. Try: **Tools → Processor → ATmega328P (Old Bootloader)**
3. Close Serial Monitor before uploading
4. Try different USB port
5. Press reset button on Arduino, then immediately upload

### Temperature Reads -127°C
```
[Reading 1] Cabin 0: -127.00°C
```

**Solutions**:
1. Sensor disconnected or wrong wiring
2. Missing pull-up resistor (4.7kΩ required!)
3. Power issue (check 5V and GND connections)
4. Try adding small delay in code: `delay(1000);` after `sensors.requestTemperatures();`

### Serial Monitor Shows Garbage Characters
```
�F�▒▒╫╫╪╪���
```

**Solutions**:
1. Wrong baud rate: Set to **115200** in Serial Monitor
2. Check `Serial.begin(115200);` in code matches Serial Monitor setting
3. Arduino still resetting (wait 2-3 seconds after opening Serial Monitor)

### LED Stays Solid On
- **Meaning**: Sensor read error
- **Check**: Wiring, pull-up resistor, DS18B20 connection
- **Test**: Re-upload code and check Serial Monitor for error messages

### Raspberry Pi Can't Read /dev/ttyUSB0
```
cat: /dev/ttyUSB0: Permission denied
```

**Solutions**:
```bash
sudo chmod 666 /dev/ttyUSB0
# Or add user to dialout group:
sudo usermod -a -G dialout $USER
# Then logout/login
```

### Multiple Arduinos Not Detected
- Each Arduino needs unique USB port (`/dev/ttyUSB0`, `/dev/ttyUSB1`, etc.)
- Check with: `ls -l /dev/ttyUSB*`
- Use USB hub if Raspberry Pi has limited ports
- Set unique `CABIN_ID` for each Arduino

---

## Multi-Cabin Deployment

### Hardware Setup:
```
Raspberry Pi 4
├─ USB Port 1 → Arduino Nano #1 (Cabin 0) → DS18B20
├─ USB Port 2 → Arduino Nano #2 (Cabin 1) → DS18B20  
├─ USB Port 3 → Arduino Nano #3 (Cabin 2) → DS18B20
└─ USB Hub    → Arduino Nano #4-10 (Cabins 3-9)
```

### Software Configuration:

**For each Arduino:**
1. Edit `temp_sensor.ino`: Set unique `CABIN_ID`
2. Upload to Arduino
3. Label Arduino physically (Cabin 0, Cabin 1, etc.)
4. Plug into Raspberry Pi

**Verify assignments**:
```bash
# Check which cabin is on which port
cat /dev/ttyUSB0  # Should show TEMP 0 XX
cat /dev/ttyUSB1  # Should show TEMP 1 XX
cat /dev/ttyUSB2  # Should show TEMP 2 XX
```

---

## Advanced Features

### 1. External Power Supply
For permanent installations, power Arduino from external 5V supply instead of USB:
- Connect 5V to **VIN** pin (not 5V pin!)
- Connect GND to **GND** pin
- Still use USB for serial data (Arduino can accept power from VIN and USB simultaneously)

### 2. Sensor Calibration
If temperature reads consistently high/low, add offset:
```cpp
float readTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempC(sensorAddress);
  temp = temp - 1.5;  // Adjust offset as needed
  return temp;
}
```

### 3. Multiple Sensors per Arduino
One Arduino can read up to ~20 DS18B20 sensors on the same pin:
```cpp
// In setup():
int deviceCount = sensors.getDeviceCount();
for (int i = 0; i < deviceCount; i++) {
  sensors.getAddress(sensorAddress[i], i);
}

// In loop():
for (int i = 0; i < deviceCount; i++) {
  float temp = sensors.getTempCByIndex(i);
  // Process each sensor...
}
```

### 4. Data Logging to SD Card
Add SD card module to Arduino for local logging:
```cpp
#include <SD.h>
File logFile = SD.open("temps.csv", FILE_WRITE);
logFile.print(cabin_id);
logFile.print(",");
logFile.println(temperature);
logFile.close();
```

---

## Performance Specifications

| Metric | Value |
|--------|-------|
| Temperature Range | -55°C to +125°C (DS18B20) |
| Accuracy | ±0.5°C (-10°C to +85°C) |
| Resolution | 12-bit (0.0625°C steps) |
| Read Time | ~750ms (12-bit resolution) |
| Update Rate | Every 5 seconds (configurable) |
| Serial Baud Rate | 115200 bps |
| Power Consumption | ~40mA (Arduino + sensor) |
| Response Time | <5 seconds (emergency detection) |

---

## Cost Analysis

### Single Cabin Sensor:
- Arduino Nano: $4
- DS18B20: $2
- 4.7kΩ Resistor: $0.10
- Breadboard: $2
- Wires: $1
- USB Cable: $2
- **Total**: ~$11 per cabin

### 10-Cabin System:
- 10x Sensors: $110
- USB Hub (7-port): $15
- **Total**: ~$125 for full coach monitoring

**Compare to**: Commercial railway temperature monitoring systems ($500-$2000)

---

## Next Steps

1. ✅ Verify Arduino Nano works with blink test
2. ✅ Wire DS18B20 with pull-up resistor
3. ✅ Upload temp_sensor.ino sketch
4. ✅ Test serial output on PC (Serial Monitor)
5. ✅ Connect to Raspberry Pi and verify /dev/ttyUSB0
6. ✅ Update RTOS usb_listener.c for Arduino port
7. ✅ Rebuild and test full system integration
8. ✅ Test high-temperature emergency detection
9. ✅ Deploy multiple cabins (if needed)

**See also**:
- `ARDUINO_NANO/QUICKSTART.md` - Fast setup guide
- `ARDUINO_NANO/WIRING_DIAGRAM.txt` - ASCII circuit diagrams
- Main `README.md` - Full RTOS documentation

---

**Implementation Date**: February 2025  
**Hardware**: Arduino Nano (ATmega328P)  
**Sensor**: DS18B20 Digital Temperature Sensor  
**Protocol**: USB Serial (115200 baud)  
**Status**: Production Ready ✅
