# Indian Railways RTOS Coach Control System

## Project Overview

Real-Time Operating System (RTOS) simulation for managing subsystems within Indian Railways LHB coaches. The system implements priority-based task scheduling, preemptive multitasking, and concurrent multi-channel USB communication.

### Key Features
- 8 prioritized tasks (P1-P10) managing critical subsystems
- Concurrent 3-channel USB communication
- **Real-time temperature monitoring** with DS18B20 sensors via Raspberry Pi Pico
- Deterministic response times (<100ms for critical events)
- Fire emergency, passenger emergency, and chain pull handlers
- Power management with load shedding
- Temperature regulation and lighting control
- Real-time terminal display or TFT framebuffer output
- **Network control** via Ethernet (TCP/IP)

## System Architecture

```
┌─────────────┐       USB/Network          ┌──────────────────┐
│             │    ──────────────────►      │                  │
│   LAPTOP    │    ◄──────────────────      │  RASPBERRY PI 4  │
│  (Python    │    Ethernet or USB          │   (C + POSIX)    │
│  Event Gen) │                             │                  │
└─────────────┘                             └────────┬─────────┘
      GUI                                            │ USB
  Event Control                                      │
                                                     │
                               ┌────────────────────┴─────────┐
                               │                              │
                      ┌────────▼────────┐          ┌─────────▼────────┐
                      │ PICO + DS18B20  │          │ PICO + DS18B20  │
                      │  Cabin 0 Temp   │          │  Cabin 1 Temp   │
                      └─────────────────┘          └─────────────────┘
                         (via /dev/ttyACM0)           (via /dev/ttyACM1)
``USB ports for Pico sensors
- Optional: 3.5" TFT Display (480×320, SPI)

### Temperature Sensors (Optional):
- Raspberry Pi Pico (per cabin)
- DS18B20 Digital Temperature Sensor
- 4.7kΩ Resistor (pull-up)
- USB cables (Pico to Pi
## Hardware Requirements

### Raspberry Pi Side:
- Raspberry Pi 4 (4GB RAM recommended)
- Raspberry Pi OS (Lite or Desktop)
- 3 available USB ports (for serial communication)
- Optional: 3.5" TFT Display (480×320, SPI)

### Laptop Side:
- Any computer with Python 3.7+
- 3 USB ports
- 3x USB-A to USB-A cables (for serial connection)

## Software Requirements

### Raspberry Pi:
```bash
sudo apt update
sudo apt install build-essential gcc make
```

### Laptop:
```bash
pip install pyserial tkinter
```

---

## PART 1: Raspberry Pi Setup

### Step 1: Transfer Code to Raspberry Pi

From your laptop, copy the RASPBERRY_PI folder to your Raspberry Pi:

```bash
# On laptop (if using SSH)
scp -r ./RASPBERRY_PI/coach_rtos pi@raspberrypi.local:~/

# Or use USB drive, git clone, etc.
```

### Step 2: Connect USB Cables

1. Connect 3 USB-A to USB-A cables from your laptop to Raspberry Pi USB ports
2. On Raspberry Pi, verify USB devices are detected:

```bash
ls -l /dev/ttyUSB*
```

Expected output:
```
/dev/ttyUSB0
/dev/ttyUSB1
/dev/ttyUSB2
```

> **Note**: If devices show as `/dev/ttyACM*` instead, edit `usb_listener.c` line 64-68 to use those names.

### Step 3: Set USB Permissions

```bash
sudo usermod -a -G dialout $USER
sudo chmod 666 /dev/ttyUSB*

# OR permanently:
sudo nano /etc/udev/rules.d/99-usb-serial.rules
# Add this line:
# SUBSYSTEM=="tty", GROUP="dialout", MODE="0666"
```

Log out and back in for group changes to take effect.

### Step 4: Build the Project

```bash
cd ~/coach_rtos
make clean
make all
```

Expected output:
```
gcc -Wall -Wextra -pthread -I./include -O2 -c src/scheduler.c -o obj/scheduler.o
gcc -Wall -Wextra -pthread -I./include -O2 -c src/tasks.c -o obj/tasks.o
gcc -Wall -Wextra -pthread -I./include -O2 -c src/display.c -o obj/display.o
gcc -Wall -Wextra -pthread -I./include -O2 -c src/usb_listener.c -o obj/usb_listener.o
gcc -Wall -Wextra -pthread -I./include -O2 -c src/main.c -o obj/main.o
gcc obj/*.o -o bin/coach_rtos -pthread -lrt -lm
Build complete: bin/coach_rtos
```

### Step 5: Run the System

```bash
./bin/coach_rtos
```

For framebuffer mode (if you have TFT display):
```bash
./bin/coach_rtos --framebuffer
```

### Troubleshooting Raspberry Pi:

**Problem**: `make: command not found`
```bash
sudo apt install build-essential
```

**Problem**: USB ports not opening
```bash
# Check permissions
ls -l /dev/ttyUSB*

# Should show: crw-rw-rw- or crw-rw----

# Fix permissions:
sudo chmod 666 /dev/ttyUSB*
```

**Problem**: `pthread` errors during compilation
```bash
# Ensure -pthread flag is in Makefile CFLAGS and LDFLAGS
# Already included in provided Makefile
```

**Problem**: Only 1 or 2 USB ports detected
- System will work with whatever ports are available
- Missing ports will be logged as failed but won't crash the system

---

## PART 1B: Temperature Sensor Setup (Optional)

### Real-Time Temperature Monitoring with Raspberry Pi Pico

Add live temperature readings from DS18B20 sensors using Raspberry Pi Pico!

#### Hardware Needed (per cabin):
- **Raspberry Pi Pico** - $4
- **DS18B20 Temperature Sensor** - $2  
- **4.7kΩ Resistor** (Yellow-Violet-Red)
- Breadboard & jumper wires
- USB cable (Micro USB)

#### Circuit:
```
Pico 3.3V (Pin 36) ──┬────► DS18B20 VCC (Pin 3)
                     │
                  4.7k pull-up
                     │
Pico GP2 (Pin 4) ────┴────► DS18B20 DATA (Pin 2)

Pico GND (Pin 3) ─────────► DS18B20 GND (Pin 1)
```

#### Quick Setup:

**1. Install MicroPython on Pico:**
- Download: https://micropython.org/download/rp2-pico/
- Hold **BOOTSEL** button while plugging in USB
- Copy .uf2 file to **RPI-RP2** drive

**2. Upload Temperature Code:**
- Install Thonny IDE: https://thonny.org/
- Open `RASPBERRY_PI_PICO/temp_sensor.py`
- File → Save As → Raspberry Pi Pico → Save as `main.py`
- Edit: `CABIN_ID = 0` (set your cabin number 0-9)

**3. Connect to Raspberry Pi:**
```bash
# Check Pico appears
ls -l /dev/ttyACM0

# Set permissions
sudo chmod 666 /dev/ttyACM0
```

**4. Test:**
```bash
# Watch temperature updates
cat /dev/ttyACM0
# Should show: TEMP 0 23 (updates every 10 sec)
```

**That's it!** The RTOS automatically listens to `/dev/ttyACM0` and updates temperatures.

**LED Status on Pico:**
- 3 blinks = Startup OK ✅
- 1 blink = Reading temp 📖
- 2 blinks = Sending data 📤
- 5 rapid blinks = HIGH TEMP EMERGENCY ⚠️
- Solid ON = Sensor error ❌

**🔥 Automatic High Temperature Detection:**
- **Threshold**: 45°C (configurable in `HIGH_TEMP_THRESHOLD`)
- **Action**: Automatically sends `EMERGENCY` command to RTOS
- **Alert Rate**: Every 3 seconds while temperature exceeds threshold
- **Recovery**: Auto-clears when temperature returns below threshold
- **Safety Feature**: Provides continuous alerts without manual intervention

**📖 Full Guides:**
- Quick Start: `RASPBERRY_PI_PICO/QUICKSTART.md`
- Circuit Details: `RASPBERRY_PI_PICO/WIRING_DIAGRAM.txt` 
- Integration: `TEMPERATURE_SENSOR_INTEGRATION.md`

---

## PART 2: Laptop Setup (Event Generator)

### Step 1: Install Dependencies

```bash
pip install pyserial
# tkinter usually comes with Python, if not:
# Windows/Mac: Already included
# Linux: sudo apt install python3-tk
```

### Step 2: Identify USB Serial Ports

**Windows:**
```powershell
# Open Device Manager → Ports (COM & LPT)
# Note the COM port numbers (e.g., COM3, COM4, COM5)
```

**Linux:**
```bash
ls -l /dev/ttyUSB* /dev/ttyACM*
# Typically /dev/ttyUSB0, /dev/ttyUSB1, /dev/ttyUSB2
```

**macOS:**
```bash
ls -l /dev/tty.usbserial*
# Typically /dev/tty.usbserial-0, etc.
```

### Step 3: Run Event Generator

**Auto-detect ports:**
```bash
cd LAPTOP
python event_generator.py
```

**Specify custom ports (Windows example):**
```bash
python event_generator.py COM3 COM4 COM5
```

**Specify custom ports (Linux example):**
```bash
python event_generator.py /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB2
```

### Step 4: Use the GUI

The GUI will launch with:
- **USB Port Status**: Shows which ports are active (green) or inactive (red)
- **USB Port Selector**: Choose which port (0-2) to send individual commands
- **Cabin Controls** (10 cabins):
  - 💡 Light toggle
  - 🌡️ Temperature adjust (±2°C)
  - 🚨 Emergency trigger
  - 🔥 Fire alert trigger
- **Concurrent Scenarios** (uses all 3 USB ports simultaneously):
  - ⚡ Lights Test
  - 🔀 Mixed Commands
  - 🚨 Emergency Test
  - 💥 Stress Test
- **System Controls**:
  - Power LOW/NORMAL
  - Chain Pull

### Troubleshooting Laptop:

**Problem**: `ModuleNotFoundError: No module named 'serial'`
```bash
pip install pyserial
# NOT 'pip install serial' (different package)
```

**Problem**: `PermissionError` on Linux
```bash
sudo chmod 666 /dev/ttyUSB*
# Or add user to dialout group:
sudo usermod -a -G dialout $USER
# Then logout/login
```

**Problem**: GUI says "WARNING: No USB ports connected"
- Check that USB cables are connected
- Verify port names match your system
- Try manually specifying ports in command line
- Python will fall back to stdout mode (prints commands without sending)

**Problem**: Commands sent but Raspberry Pi doesn't respond
- Check that Raspberry Pi program is running
- Verify same baud rate (115200) on both sides
- Check that cables are data-capable (not charge-only cables)

---

## PART 3: Testing the System

### Test 1: Individual Commands
1. Start Raspberry Pi program: `./bin/coach_rtos`
2. Start laptop GUI: `python event_generator.py`
3. Select "USB 0" in GUI
4. Click "💡" for Cabin 0
5. **Expected**: Raspberry Pi terminal shows:
   ```
   [USB0 RX] Received: 'LIGHT 0 ON'
   ```

### Test 2: Concurrent Commands
1. Click "⚡ Lights Test" button in GUI
2. **Expected**: All 3 USB ports send commands simultaneously:
   ```
   [USB0 TX] LIGHT 0 ON
   [USB1 TX] LIGHT 3 ON
   [USB2 TX] LIGHT 7 ON
   ```

### Test 3: Priority Preemption
1. Send: FIRE 5 (via any cabin Fire button)
2. **Expected**:
   - Fire Emergency task (P=10) immediately preempts other tasks
   - Response time < 100ms
   - Logs show: `[FIRE EMERGENCY] Processing fire alert - HIGHEST PRIORITY`

### Test 4: Display Update
1. Send multiple light/temperature commands
2. **Expected**: Coach status display updates every 500ms showing current state:
   ```
   ╔════════════════════════════════════════════════════════════╗
   ║         INDIAN RAILWAYS - COACH STATUS DISPLAY            ║
   ╠════════════════════════════════════════════════════════════╣
   ║  Cabin 0: Light ON, 22°C                                  ║
   ║  Cabin 1: Normal, 22°C                                    ║
   ...
   ```

### Test 5: Stress Test
1. Click "💥 Stress Test" button repeatedly
2. **Expected**: System handles 30+ commands/second without crashes

---

## Command Protocol Reference

### Format
```
<COMMAND> <ARG1> [ARG2]
```

### Commands

| Command | Arguments | Example | Response |
|---------|-----------|---------|----------|
| LIGHT | cabin_id (0-9), ON/OFF | `LIGHT 3 ON` | `OK: Light ON` |
| TEMP | cabin_id (0-9), temp (10-35) | `TEMP 5 24` | `OK: Temperature set` |
| EMERGENCY | cabin_id (0-9) | `EMERGENCY 2` | `OK: Emergency activated` |
| FIRE | cabin_id (0-9) | `FIRE 7` | `OK: Fire alert activated` |
| POWER | LOW/NORMAL | `POWER LOW` | `OK: Power set to LOW` |
| CHAIN | PULL | `CHAIN PULL` | `OK: Chain pulled` |

### Error Responses
- `ERROR: Invalid format`
- `ERROR: Invalid cabin ID`
- `ERROR: Temperature range 10-35°C`
- `ERROR: Unknown command`

---

## Task Priority Table

| Priority | Task | Cycle Time | Description |
|----------|------|------------|-------------|
| 10 | Fire Emergency | 100ms | Highest priority, handles fire alerts |
| 9 | Passenger Emergency | 150ms | Handles emergency button presses |
| 8 | Chain Pull | 200ms | Emergency stop mechanism |
| 7 | Power Management | 500ms | Load shedding, battery management |
| 4 | Temperature Regulation | 1s | HVAC control |
| 3 | Lighting Control | 800ms | Cabin lighting management |
| 2 | Display Update | 500ms | Terminal/framebuffer rendering |
| 1 | System Logging | 2s | Periodic status logging |

---

## Performance Metrics

### Measured Results:
- Fire emergency response: **45-58ms**
- Passenger emergency response: **87-103ms**
- Command throughput: **35+ commands/second** across 3 USB ports
- Context switch overhead: **~2ms**
- CPU utilization: **15-25%** normal, **45%** peak stress

---

## Cleanup

### Stop Programs:
```bash
# On Raspberry Pi:
Ctrl+C

# On Laptop:
Close GUI window or Ctrl+C
```

### Clean Build Artifacts:
```bash
cd ~/coach_rtos
make clean
```

---

## Project Structure

```
RASPBERRY_PI/coach_rtos/
├── Makefile              # Build configuration
├── bin/                  # Compiled binary output
├── obj/                  # Object files (intermediate)
├── include/
│   ├── common.h         # Shared types and constants
│   ├── scheduler.h      # Task scheduler API
│   ├── tasks.h          # Task function declarations
│   └── display.h        # Display API
└── src/
    ├── main.c           # Entry point, initialization
    ├── scheduler.c      # Priority-based scheduler
    ├── tasks.c          # Task implementations + utils
    ├── display.c        # Terminal/framebuffer display
    └── usb_listener.c   # Multi-USB serial communication

LAPTOP/
└── event_generator.py   # Python GUI for sending commands
```

---

## Known Limitations

1. **Not a bare-metal RTOS**: Runs on Linux, uses POSIX threads
2. **Soft real-time**: Not suitable for safety-critical certified systems
3. **USB latency**: Serial communication adds ~10-20ms latency
4. **Display refresh**: Limited to 2Hz to avoid terminal flicker

---

## Future Enhancements

- Actual sensor integration (GPIO temperature sensors, smoke detectors)
- Wireless communication (WiFi, Bluetooth)
- Machine learning for predictive maintenance
- Multi-coach networked coordination
- Web dashboard for remote monitoring
- Database logging for historical analysis

---

## Troubleshooting Summary

### Compilation Errors:
1. Check that all source files are present
2. Ensure `gcc`, `make` are installed
3. Verify `pthread`, `rt`, `lm` libraries available

### USB Communication Issues:
1. Check cable connections (must be data-capable)
2. Verify port names match your system
3. Ensure permissions are set correctly (dialout group or chmod 666)
4. Try different baud rates if needed (unlikely)

### Runtime Crashes:
1. Check that USB ports are properly initialized
2. Verify mutexes are correctly locked/unlocked
3. Enable debug mode: `make debug && ./bin/coach_rtos`

### Performance Issues:
1. Reduce display update frequency
2. Check CPU load with `htop`
3. Ensure Raspberry Pi is not throttling (check temperature)

---

## Support and Documentation

For detailed project report, see the PDF document in `/report` folder.

**Author**: Kavin Krishnan C (1RV24CS126)
**Institution**: RV College of Engineering, Bengaluru
**Course**: Operating Systems [CS235AI] - Experiential Learning Lab
**Year**: 2025-2026

---

## License

This is an academic project. All intellectual property rights belong to RV College of Engineering.
