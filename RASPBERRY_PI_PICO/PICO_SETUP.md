# Raspberry Pi Pico - DS18B20 Temperature Sensor Setup

## Hardware Required
- Raspberry Pi Pico
- DS18B20 Digital Temperature Sensor (TO-92 package)
- 4.7kΩ Resistor (exact value important!)
- Breadboard and jumper wires
- USB cable (Pico to Raspberry Pi)

---

## Circuit Connections

```
DS18B20 Temperature Sensor (TO-92 Package)
Looking at flat side with pins down:

     _____
    |     |
    | DS  |
    |18B20|
    |_____|
     | | |
     1 2 3

Pin 1 (GND)  - Ground
Pin 2 (DATA) - Data (with pull-up resistor)
Pin 3 (VCC)  - Power (3.3V)
```

### Wiring Diagram:

```
Raspberry Pi Pico              DS18B20 Sensor
┌─────────────┐
│             │
│   (USB)     │
│             │
│         3.3V├──────┬──────────────► Pin 3 (VCC)
│             │      │
│             │   4.7kΩ Resistor
│             │      │
│          GP2├──────┴──────────────► Pin 2 (DATA)
│             │
│          GND├────────────────────── Pin 1 (GND)
│             │
└─────────────┘
```

### Step-by-Step Connections:

1. **DS18B20 Pin 1 (GND)** → Pico **GND** (any GND pin)
2. **DS18B20 Pin 2 (DATA)** → Pico **GP2** (GPIO 2)
3. **DS18B20 Pin 3 (VCC)** → Pico **3.3V** (3V3 OUT)
4. **4.7kΩ Resistor** → Between DS18B20 Pin 2 (DATA) and Pin 3 (VCC)

```
Breadboard Layout:

       4.7kΩ
3.3V ──┬──┬── DATA (GP2)
       │  └── To Pico GP2
       └────── To DS18B20 VCC & DATA
       
GND ────────── To DS18B20 GND
```

---

## Pin Reference

**Raspberry Pi Pico Pins Used:**
- **GP2** (Physical Pin 4) - DS18B20 DATA line
- **3V3(OUT)** (Physical Pin 36) - Power supply
- **GND** (Physical Pin 3, 8, 13, 18, 23, 28, 33, 38) - Ground

**Built-in LED** (GP25) - Used for status indication:
- 3 blinks on startup
- 1 blink on temperature read
- 2 blinks when sending data to Pi

---

## Software Setup

### 1. Install MicroPython on Pico

1. Download MicroPython UF2 file from: https://micropython.org/download/rp2-pico/
2. Hold BOOTSEL button on Pico while connecting USB
3. Drag and drop .uf2 file to RPI-RP2 drive
4. Pico reboots with MicroPython installed

### 2. Upload Code to Pico

**Using Thonny IDE (Recommended):**
1. Install Thonny: https://thonny.org/
2. Select MicroPython (Raspberry Pi Pico) in bottom-right
3. Open `temp_sensor.py`
4. Save to Pico as `main.py` (runs on boot)
5. Click Run

**Using ampy (Command line):**
```bash
pip install adafruit-ampy
ampy --port COM3 put temp_sensor.py /main.py
```

### 3. Configuration

Edit these variables in `temp_sensor.py`:

```python
CABIN_ID = 0         # Which cabin (0-9) to monitor
READ_INTERVAL = 5    # Read temp every 5 seconds
SEND_INTERVAL = 10   # Send to Pi every 10 seconds
```

---

## Testing

### Test 1: Verify DS18B20 Detection
Run this in Thonny REPL:
```python
import machine, onewire, ds18x20
ds = ds18x20.DS18X20(onewire.OneWire(machine.Pin(2)))
print(ds.scan())  # Should show ROM address
```

### Test 2: Read Temperature
```python
roms = ds.scan()
ds.convert_temp()
import time; time.sleep_ms(750)
print(ds.read_temp(roms[0]))  # Should show temperature
```

### Test 3: Full System Test
1. Upload `main.py` to Pico
2. Reset Pico (unplug/replug USB)
3. Should see 3 LED blinks on startup
4. Watch serial output in Thonny for temperature readings

---

## Communication Protocol

**Pico sends to Raspberry Pi:**
```
TEMP <cabin_id> <temperature>\n
```

**Examples:**
```
TEMP 0 23
TEMP 0 24
TEMP 0 25
```

Temperature is automatically:
- Rounded to nearest integer
- Clamped to valid range (10-35°C)
- Sent every 10 seconds or when changed ±1°C

---

## LED Status Indicators

| Pattern | Meaning |
|---------|---------|
| 3 quick blinks | Startup successful |
| 1 short blink | Temperature read |
| 2 short blinks | Data sent to Pi |
| Solid ON | Error - sensor disconnected |

---

## Troubleshooting

**No sensor found:**
- Check 4.7kΩ resistor is between DATA and VCC
- Verify DS18B20 pin orientation (flat side with pins down)
- Test with multimeter: DATA line should be ~3.3V

**Wrong temperature readings:**
- Ensure good contact on breadboard
- Check power supply is stable
- Try different DS18B20 sensor

**Pico not recognized:**
- Reinstall MicroPython
- Try different USB cable
- Check Pico is powered (LED should be lit)

**Pi not receiving data:**
- Check `/dev/ttyACM0` exists: `ls -l /dev/ttyACM*`
- Verify baud rate matches (115200)
- Check Pico is sending: watch serial in Thonny

---

## Multiple Sensors (Optional)

To monitor multiple cabins, connect multiple Picos:
- `/dev/ttyACM0` → Cabin 0 (set `CABIN_ID = 0`)
- `/dev/ttyACM1` → Cabin 1 (set `CABIN_ID = 1`)
- `/dev/ttyACM2` → Cabin 2 (set `CABIN_ID = 2`)

Update Raspberry Pi code to listen to all ports.

---

## Power Considerations

**DS18B20 Power Modes:**
1. **Normal Mode** (as shown above) - Recommended
   - Separate VCC connection
   - More reliable
   
2. **Parasitic Mode** (not recommended)
   - Powers from DATA line
   - Requires stronger pull-up (1kΩ)

**Pico Power:**
- Powered via USB from Raspberry Pi
- Draws ~20mA typical
- DS18B20 adds ~1mA

Total per Pico+sensor: ~25mA (well within USB limits)

---

## Physical Mounting

**For Railway Coach Application:**
- Mount Pico in small project box
- Extend DS18B20 with shielded 3-wire cable (up to 20 meters)
- Use screw terminals for easy maintenance
- Weather-proof enclosure for exposed locations
- Label each sensor with cabin number

---

## Files Included

- `temp_sensor.py` - Main MicroPython code for Pico
- `PICO_SETUP.md` - This setup guide

Upload `temp_sensor.py` as `main.py` to Pico.
