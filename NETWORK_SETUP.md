# Network Setup Guide - Ethernet Connection

## Quick Start: Connect via Ethernet Cable

### Physical Setup
1. **Connect Ethernet cable** between Laptop and Raspberry Pi
2. Both devices will auto-configure link-local addresses (169.254.x.x)

---

## On Raspberry Pi

### 1. Build with network support (already included)
```bash
cd ~/coach_rtos
make clean
make all
```

### 2. Run in network mode
```bash
./bin/coach_rtos --network
```

You should see:
```
[MAIN] Starting network mode on port 5000...
[NETWORK] Server initialized on port 5000
[NETWORK] Listening on port 5000...
```

### 3. Find your Raspberry Pi's IP address
```bash
# Check Ethernet interface (usually eth0)
ip addr show eth0

# Or simpler:
hostname -I
```

Example output: `192.168.1.100` or `169.254.45.123`

---

## On Laptop (Windows)

### 1. Find Raspberry Pi's IP
```powershell
# Ping by hostname (if on same network)
ping raspberrypi.local

# Or scan for devices
arp -a
```

### 2. Run the network event generator
```powershell
cd C:\Users\kavin\COACH_RTOS\LAPTOP
python event_generator_network.py 192.168.1.100
```

Replace `192.168.1.100` with your Raspberry Pi's actual IP address.

---

## Connection Options

### Option A: Direct Ethernet Cable
- Connect laptop ↔ Raspberry Pi directly
- Both get auto-configured IPs (169.254.x.x)
- No router needed!

### Option B: Same Network/Router
- Both connected to same WiFi/router
- Use `raspberrypi.local` or actual IP
- More reliable

### Option C: Raspberry Pi Hotspot
- Configure Pi as WiFi hotspot
- Laptop connects to Pi's WiFi
- Fully portable!

---

## Testing the Connection

### From Laptop Command Line:
```powershell
# Test if port 5000 is open
Test-NetConnection -ComputerName 192.168.1.100 -Port 5000

# Or simple telnet test
telnet 192.168.1.100 5000
```

### Send manual commands via telnet:
```
telnet 192.168.1.100 5000
LIGHT 3 ON
TEMP 5 24
EMERGENCY 2
```

---

## Troubleshooting

**Can't connect?**
```bash
# On Raspberry Pi - check if service is running:
netstat -tuln | grep 5000

# Check firewall (usually not needed on Pi):
sudo ufw status
sudo ufw allow 5000/tcp
```

**Wrong IP?**
```bash
# On Raspberry Pi:
hostname -I          # Show all IPs
ip route get 1.1.1.1 # Show default route IP
```

**Direct cable not working?**
- Some laptops need a crossover Ethernet cable
- Most modern devices auto-detect (Auto-MDIX)
- Try using a router/switch instead

---

## File Summary

**Raspberry Pi files:**
- `src/network_listener.c` - Network server implementation
- `src/main.c` - Modified to support `--network` flag

**Laptop files:**
- `event_generator_network.py` - Network client GUI

**Commands:**
```bash
# Raspberry Pi:
./bin/coach_rtos --network

# Laptop:
python event_generator_network.py raspberrypi.local
# Or:
python event_generator_network.py 192.168.1.100
```

---

## Advantages of Network Mode

✅ No USB serial adapter needed  
✅ Works over WiFi  
✅ Can control from anywhere on network  
✅ Multiple clients can connect (up to 3)  
✅ More reliable than USB serial  
✅ Easier debugging with telnet  

Enjoy your network-enabled RTOS Coach Control System! 🚂
