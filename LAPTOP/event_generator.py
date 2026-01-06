#!/usr/bin/env python3
"""
RTOS Coach System - Event Generator (Laptop)
Generates events and sends commands to Raspberry Pi via USB serial
"""

import serial
import time
import random
import sys
from typing import Optional

class CoachEventGenerator:
    def __init__(self, port: str = '/dev/ttyACM0', baudrate: int = 115200):
        """Initialize connection to Raspberry Pi"""
        self.port = port
        self.baudrate = baudrate
        self.ser: Optional[serial.Serial] = None
        self.num_cabins = 10
        
    def connect(self):
        """Establish serial connection"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)  # Wait for connection to stabilize
            print(f"✓ Connected to Raspberry Pi on {self.port}")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to connect: {e}")
            print("Tip: Check 'ls /dev/ttyACM*' or 'ls /dev/ttyUSB*'")
            return False
    
    def disconnect(self):
        """Close serial connection"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✓ Disconnected")
    
    def send_command(self, command: str):
        """Send command to Raspberry Pi"""
        if not self.ser or not self.ser.is_open:
            print("✗ Not connected")
            return False
        
        try:
            cmd = command.strip() + '\n'
            self.ser.write(cmd.encode('utf-8'))
            print(f"→ Sent: {command}")
            return True
        except Exception as e:
            print(f"✗ Error sending command: {e}")
            return False
    
    def light_control(self, cabin_id: int, state: str):
        """Control cabin light"""
        self.send_command(f"LIGHT {cabin_id} {state}")
    
    def temperature_adjust(self, cabin_id: int, temp: int):
        """Adjust cabin temperature"""
        self.send_command(f"TEMP {cabin_id} {temp}")
    
    def trigger_emergency(self, cabin_id: int):
        """Trigger emergency in cabin"""
        self.send_command(f"EMERGENCY {cabin_id}")
    
    def trigger_fire(self, cabin_id: int):
        """Trigger fire alert in cabin"""
        self.send_command(f"FIRE {cabin_id}")
    
    def power_low(self):
        """Trigger low power condition"""
        self.send_command("POWER LOW")
    
    def chain_pull(self):
        """Simulate chain pull"""
        self.send_command("CHAIN PULL")
    
    def request_status(self):
        """Request system status"""
        self.send_command("STATUS")
    
    def random_event(self):
        """Generate a random event"""
        cabin_id = random.randint(0, self.num_cabins - 1)
        event_type = random.choice(['light', 'temp', 'emergency', 'fire'])
        
        if event_type == 'light':
            state = random.choice(['ON', 'OFF'])
            self.light_control(cabin_id, state)
        elif event_type == 'temp':
            temp = random.randint(18, 28)
            self.temperature_adjust(cabin_id, temp)
        elif event_type == 'emergency':
            self.trigger_emergency(cabin_id)
        elif event_type == 'fire':
            self.trigger_fire(cabin_id)
    
    def demo_sequence(self):
        """Run a demonstration sequence"""
        print("\n" + "="*60)
        print("Starting Demo Sequence")
        print("="*60 + "\n")
        
        # 1. Turn on lights in cabins 0-4
        print("Step 1: Turning on lights in cabins 0-4")
        for i in range(5):
            self.light_control(i, 'ON')
            time.sleep(0.5)
        
        time.sleep(2)
        
        # 2. Adjust temperatures
        print("\nStep 2: Adjusting temperatures")
        for i in range(0, 5):
            temp = 20 + i
            self.temperature_adjust(i, temp)
            time.sleep(0.5)
        
        time.sleep(2)
        
        # 3. Trigger emergency
        print("\nStep 3: Triggering emergency in cabin 3")
        self.trigger_emergency(3)
        time.sleep(3)
        
        # 4. Trigger fire alert
        print("\nStep 4: Triggering FIRE in cabin 7")
        self.trigger_fire(7)
        time.sleep(3)
        
        # 5. Low power
        print("\nStep 5: Activating low power mode")
        self.power_low()
        time.sleep(2)
        
        # 6. Chain pull
        print("\nStep 6: Chain pull emergency")
        self.chain_pull()
        time.sleep(2)
        
        # 7. Status
        print("\nStep 7: Requesting system status")
        self.request_status()
        
        print("\n" + "="*60)
        print("Demo Sequence Complete")
        print("="*60 + "\n")

def print_menu():
    """Print interactive menu"""
    print("\n" + "="*60)
    print("  RTOS Coach System - Event Generator")
    print("="*60)
    print("\nCommands:")
    print("  1  - Turn light ON in a cabin")
    print("  2  - Turn light OFF in a cabin")
    print("  3  - Adjust temperature in a cabin")
    print("  4  - Trigger EMERGENCY in a cabin")
    print("  5  - Trigger FIRE in a cabin")
    print("  6  - Activate LOW POWER mode")
    print("  7  - Trigger CHAIN PULL")
    print("  8  - Request system STATUS")
    print("  9  - Generate RANDOM event")
    print("  d  - Run DEMO sequence")
    print("  q  - Quit")
    print("="*60 + "\n")

def main():
    """Main function"""
    # Parse command line arguments
    port = '/dev/ttyACM0'
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    # Create event generator
    generator = CoachEventGenerator(port=port)
    
    # Connect to Raspberry Pi
    if not generator.connect():
        print("\nTroubleshooting:")
        print("1. Check USB connection")
        print("2. Verify Raspberry Pi is running the coach_rtos program")
        print("3. Try different port: python3 event_generator.py /dev/ttyUSB0")
        return
    
    # Interactive mode
    try:
        print_menu()
        
        while True:
            try:
                choice = input("Enter command: ").strip().lower()
                
                if choice == 'q':
                    break
                elif choice == '1':
                    cabin = int(input("Cabin ID (0-9): "))
                    generator.light_control(cabin, 'ON')
                elif choice == '2':
                    cabin = int(input("Cabin ID (0-9): "))
                    generator.light_control(cabin, 'OFF')
                elif choice == '3':
                    cabin = int(input("Cabin ID (0-9): "))
                    temp = int(input("Temperature (18-28°C): "))
                    generator.temperature_adjust(cabin, temp)
                elif choice == '4':
                    cabin = int(input("Cabin ID (0-9): "))
                    generator.trigger_emergency(cabin)
                elif choice == '5':
                    cabin = int(input("Cabin ID (0-9): "))
                    generator.trigger_fire(cabin)
                elif choice == '6':
                    generator.power_low()
                elif choice == '7':
                    generator.chain_pull()
                elif choice == '8':
                    generator.request_status()
                elif choice == '9':
                    generator.random_event()
                elif choice == 'd':
                    generator.demo_sequence()
                else:
                    print("Invalid command")
                
                time.sleep(0.5)
                
            except ValueError:
                print("Invalid input")
            except KeyboardInterrupt:
                print("\n")
                break
    
    finally:
        generator.disconnect()

if __name__ == "__main__":
    main()