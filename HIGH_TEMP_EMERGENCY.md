# 🔥 High Temperature Emergency Detection

## Overview
The temperature sensor system now includes **automatic emergency detection** when cabin temperature exceeds 45°C. This safety feature provides continuous monitoring and alerts without requiring manual intervention.

## How It Works

### Detection Logic
```
Temperature > 45°C → Automatic EMERGENCY command sent every 3 seconds
Temperature ≤ 45°C → Normal operation (temp updates every 10s)
```

### System Behavior

**When High Temperature Detected (>45°C):**
1. **Pico sends**: `EMERGENCY <cabin_id>` command via USB serial
2. **Alert rate**: Every 3 seconds (faster than normal 10s interval)
3. **Visual indicator**: LED blinks rapidly (5 quick flashes)
4. **Console warning**: Shows `⚠️ HIGH TEMP ALERT! Cabin X: YY°C`
5. **RTOS response**: 
   - Sets emergency flag for that cabin
   - Triggers passenger emergency task (Priority 2)
   - Displays continuous emergency alerts in terminal
   - Emergency persists until manually cleared with CLEAR button

**When Temperature Returns to Normal (≤45°C):**
- Console message: `Temperature returned to normal: XX.XX°C`
- Emergency alerts stop sending (but emergency flag remains set on RTOS)
- Normal temperature reporting resumes
- **Note**: Emergency must still be manually cleared in RTOS using CLEAR button

## Configuration

Edit `RASPBERRY_PI_PICO/temp_sensor.py`:

```python
HIGH_TEMP_THRESHOLD = 45  # Temperature threshold for emergency (°C)
HIGH_TEMP_ALERT_INTERVAL = 3  # Alert every 3 seconds when over threshold
```

### Recommended Thresholds:
- **Default**: 45°C (dangerous for passengers)
- **Conservative**: 40°C (hot but tolerable)
- **Critical**: 50°C (immediate evacuation needed)

## Visual Indicators

### Pico LED Patterns:
| Pattern | Meaning |
|---------|---------|
| 3 blinks on startup | System initialized ✅ |
| 1 blink every 5s | Reading temperature 📖 |
| 2 blinks every 10s | Sending normal temp data 📤 |
| **5 rapid blinks every 3s** | **HIGH TEMP EMERGENCY ⚠️** |
| Solid ON | Sensor disconnected ❌ |

### Console Output:
```
Normal reading:
[Reading 42] Cabin 0: 23.44°C

High temperature detected:
⚠️ HIGH TEMP ALERT! Cabin 0: 47°C (Threshold: 45°C)
EMERGENCY 0
⚠️ HIGH TEMP ALERT! Cabin 0: 48°C (Threshold: 45°C)
EMERGENCY 0
...

Temperature returns to normal:
Temperature returned to normal: 43.56°C
[Reading 89] Cabin 0: 43.56°C
```

## RTOS Integration

The existing emergency system handles high-temperature alerts automatically:

### Command Flow:
```
Pico (temp_sensor.py)
    │
    ├─► Detects temp > 45°C
    ├─► Sends: "EMERGENCY <cabin_id>\n"
    │
    ▼
Raspberry Pi (/dev/ttyACM0)
    │
    ├─► usb_listener.c receives command
    ├─► Calls: cabin_set_emergency(cabin_id, true)
    ├─► Triggers: scheduler_preempt(PRIORITY_PASSENGER_EMERGENCY)
    │
    ▼
RTOS Scheduler
    │
    ├─► Runs task_passenger_emergency() (Priority 2)
    ├─► Displays: "⚠️  PASSENGER EMERGENCY - CABIN X ⚠️"
    └─► Continuous alerts until CLEAR button pressed
```

### Edge Detection:
The emergency system uses edge detection to prevent infinite loops:
- Only triggers when emergency flag changes from `false` → `true`
- Continuous alerts are visual only (terminal display)
- Each new EMERGENCY command from Pico sets the flag again
- Manual CLEAR required to reset emergency state

## Testing

### Simulate High Temperature (for testing without heat source):

**Option 1: Edit Pico threshold temporarily**
```python
# In temp_sensor.py, set very low threshold for testing
HIGH_TEMP_THRESHOLD = 25  # Will trigger at room temperature
```

**Option 2: Use heat source**
- Hair dryer (warm setting, 20-30cm away)
- Cup of hot water near sensor
- Hand warmth (may take several readings)

### Expected Behavior:
```
1. Temperature rising:
   [Reading 1] Cabin 0: 43.12°C
   [Reading 2] Cabin 0: 44.75°C
   [Reading 3] ⚠️ HIGH TEMP! Cabin 0: 46.31°C
   HIGH TEMP ALERT! Cabin 0: 46°C (Threshold: 45°C)
   EMERGENCY 0

2. RTOS terminal shows:
   ⚠️  PASSENGER EMERGENCY - CABIN 0 ⚠️
   [Repeating...]

3. Temperature drops:
   Temperature returned to normal: 42.19°C
   [Reading 15] Cabin 0: 42.19°C
   
4. Clear emergency:
   Press CLEAR button in GUI or send "CLEAR 0" command
```

## Safety Features

✅ **No manual intervention required** - Automatic detection and alerting  
✅ **Continuous monitoring** - Checks every 5 seconds  
✅ **Fast response** - 3-second alert interval during emergency  
✅ **Visual feedback** - LED blinks rapidly for immediate recognition  
✅ **Persistent alerts** - Emergency flag remains set until manually cleared  
✅ **Auto-recovery notification** - Console message when temp normalizes  
✅ **Fail-safe** - Sensor errors trigger LED solid-on warning  

## Use Cases

### 1. AC System Failure
- AC stops working on hot day
- Temperature rises gradually
- Reaches 45°C within 30-60 minutes
- **System alerts automatically**
- Maintenance can respond before passengers complain

### 2. Electrical Fire
- Overheated electrical component
- Temperature spikes rapidly
- Crosses 45°C threshold
- **Immediate emergency alert**
- Crew can investigate and extinguish

### 3. External Heat Source
- Train stopped in extreme heat location
- Sun exposure on stationary coach
- Temperature climbs slowly
- **Proactive warning system**
- Evacuation can be planned

## Integration with Multiple Sensors

For multiple cabins, deploy one Pico per cabin:

```
Raspberry Pi USB Ports:
├─ /dev/ttyACM0 → Pico (Cabin 0) → DS18B20
├─ /dev/ttyACM1 → Pico (Cabin 1) → DS18B20
├─ /dev/ttyACM2 → Pico (Cabin 2) → DS18B20
...
```

Each Pico:
1. Set unique `CABIN_ID` in temp_sensor.py
2. Monitors its own sensor independently
3. Sends alerts for its cabin only
4. All emergencies appear in RTOS terminal

## Troubleshooting

### Emergency not triggering:
- ✓ Check `HIGH_TEMP_THRESHOLD` value in temp_sensor.py
- ✓ Verify USB connection: `ls -l /dev/ttyACM0`
- ✓ Check Pico console output in Thonny
- ✓ Test with lower threshold (e.g., 25°C)

### Emergency keeps repeating:
- **This is normal behavior!** Emergency commands sent every 3s
- RTOS edge detection prevents task re-execution
- Visual alerts in terminal are intentional
- **Solution**: Press CLEAR button when addressed

### Temperature stuck at 35°C:
- Display temperature is clamped to 10-35°C range for normal TEMP command
- Actual temperature is still measured correctly
- Emergency detection uses REAL temperature (not clamped)
- Check Pico console for actual readings

## Files Modified

1. **RASPBERRY_PI_PICO/temp_sensor.py**
   - Added `HIGH_TEMP_THRESHOLD` configuration
   - Added `HIGH_TEMP_ALERT_INTERVAL` configuration
   - New function: `send_high_temp_emergency()`
   - Updated main loop with high-temperature detection logic
   - Added LED rapid blink pattern (5 blinks) for emergencies

2. **RASPBERRY_PI/coach_rtos/src/usb_listener.c**
   - Already supports EMERGENCY command (no changes needed)

3. **RASPBERRY_PI/coach_rtos/src/tasks.c**
   - Already has edge detection (no changes needed)

4. **Documentation**
   - README.md: Added high-temp feature description
   - QUICKSTART.md: Added configuration and behavior notes
   - HIGH_TEMP_EMERGENCY.md: This comprehensive guide

## Future Enhancements

Possible improvements:
- [ ] Configurable thresholds via USB command (runtime adjustment)
- [ ] Graduated emergency levels (Warning at 40°C, Critical at 45°C)
- [ ] Temperature trend analysis (rapid rise detection)
- [ ] Auto-clear emergency when temp drops (currently requires manual CLEAR)
- [ ] SMS/Email alerts via GSM module
- [ ] Data logging to SD card on Pico

---

**Implementation Date**: January 2025  
**Safety Critical**: Yes  
**Testing Status**: Code complete, hardware testing recommended  
**Maintenance**: Update threshold based on local climate and regulations
