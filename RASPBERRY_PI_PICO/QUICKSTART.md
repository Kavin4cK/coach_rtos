# 🚂 Temperature Sensor Quick Start Guide

## What You Need
- ✅ Raspberry Pi Pico
- ✅ DS18B20 Temperature Sensor  
- ✅ 4.7kΩ Resistor (Yellow-Violet-Red)
- ✅ Breadboard + Jumper Wires
- ✅ USB Cable (Micro USB)

## Circuit in 30 Seconds

```
Pico Pin 36 (3.3V) ──┬───────► DS18B20 Pin 3 (VCC)
                     │
                  4.7kΩ
                     │
Pico Pin 4 (GP2) ────┴───────► DS18B20 Pin 2 (DATA)

Pico Pin 3 (GND) ────────────► DS18B20 Pin 1 (GND)
```

## Software Setup

### 1. Install MicroPython on Pico (2 minutes)
1. Download: https://micropython.org/download/rp2-pico/
2. Hold **BOOTSEL** button on Pico
3. Plug in USB cable
4. Drag .uf2 file to **RPI-RP2** drive
5. Done! Pico reboots automatically

### 2. Upload Temperature Code (1 minute)
1. Install Thonny: https://thonny.org/
2. Open Thonny, select "MicroPython (Raspberry Pi Pico)"
3. Open: `RASPBERRY_PI_PICO/temp_sensor.py`
4. Save to Pico as **`main.py`**
5. Click **Run** (F5)

You should see:
```
DS18B20 Temperature Monitor - Coach RTOS
Found 1 DS18B20 sensor(s)
Starting temperature monitoring...
[Reading 1] Cabin 0: 23.44°C
```

### 3. Configure Cabin Number
Edit line 15 in `temp_sensor.py`:
```python
CABIN_ID = 0  # Change to your cabin number (0-9)
```

### 4. High Temperature Emergency Detection
The sensor automatically detects dangerous temperatures:
- **Threshold**: 45°C (configurable in `HIGH_TEMP_THRESHOLD`)
- **Action**: Sends `EMERGENCY` command to RTOS every 3 seconds
- **Visual**: Pico LED blinks rapidly (5 quick flashes)
- **Console**: Shows ⚠️ HIGH TEMP ALERT warnings

When temperature drops below threshold, normal operation resumes automatically.

## Raspberry Pi Setup

### 1. Check USB Connection
```bash
ls -l /dev/ttyACM0
# Should exist when Pico is plugged in
```

### 2. Set Permissions
```bash
sudo chmod 666 /dev/ttyACM0
```

### 3. Rebuild RTOS
```bash
cd ~/coach_rtos
make clean && make all
```

### 4. Run System
```bash
./bin/coach_rtos
```

You'll see:
```
[USB0] Connected to /dev/ttyACM0 at 115200 baud
[USB0 RX] TEMP 0 23
```

## LED Status on Pico

| LED Blinks | Meaning |
|------------|---------|
| 3 quick | ✅ Startup OK |
| 1 short | 📖 Reading temp |
| 2 short | 📤 Sending data |
| Solid ON | ❌ Sensor error |

## Verification Checklist

- [ ] Pico LED blinks 3 times on startup
- [ ] `/dev/ttyACM0` exists on Raspberry Pi
- [ ] Pico shows temperature readings in Thonny
- [ ] RTOS receives `TEMP` commands
- [ ] Temperature displayed in terminal

## Testing

**Touch the sensor** with your finger:
- Temperature should increase from ~23°C to ~30°C
- Update appears within 10 seconds
- RTOS logs: `Cabin X: Temperature set to XXX°C`

## Files Summary

**Raspberry Pi Pico:**
- `temp_sensor.py` → MicroPython code
- Save as `main.py` on Pico

**Raspberry Pi:**
- Updated `usb_listener.c` → Listens to `/dev/ttyACM0`
- Updated `main.c` → Shows Pico in help

**Documentation:**
- `PICO_SETUP.md` → Detailed setup guide
- `WIRING_DIAGRAM.txt` → ASCII circuit diagrams
- `TEMPERATURE_SENSOR_INTEGRATION.md` → Full integration guide

## Common Issues

**"No sensor found"**
→ Check 4.7kΩ resistor between DATA and VCC

**"Permission denied" on /dev/ttyACM0**
→ Run: `sudo chmod 666 /dev/ttyACM0`

**Pico not detected**
→ Try different USB cable (needs data, not just power)

**Wrong temperatures**
→ Check DS18B20 pin orientation (flat side with pins down)

## Next Steps

1. ✅ Test with single sensor
2. 🔲 Add GUI button to see temperature
3. 🔲 Add more Picos for other cabins
4. 🔲 Create permanent circuit
5. 🔲 Mount in enclosure

---

📖 For detailed info, see: `TEMPERATURE_SENSOR_INTEGRATION.md`
🔌 For wiring details, see: `WIRING_DIAGRAM.txt`
