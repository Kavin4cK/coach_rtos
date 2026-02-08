#!/usr/bin/env python3
"""
Indian Railways Coach Event Generator - Multi-USB Version
Sends concurrent commands through 3 USB ports simultaneously
"""

import sys
import time
import random
import threading
from queue import Queue

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("Warning: pyserial not installed. Install with: pip install pyserial")

try:
    from tkinter import *
    from tkinter import ttk, messagebox
    TKINTER_AVAILABLE = True
except ImportError:
    TKINTER_AVAILABLE = False
    print("Warning: tkinter not available")

class MultiUSBController:
    """Manages 3 USB serial connections with concurrent command sending"""
    
    def __init__(self, ports=None, baudrate=115200):
        self.baudrate = baudrate
        self.ports = []
        self.command_queues = [Queue(), Queue(), Queue()]
        self.sender_threads = []
        self.running = False
        
        # Default ports for different OS
        if ports is None:
            # Try to auto-detect
            if sys.platform.startswith('win'):
                ports = ['COM3', 'COM4', 'COM5']
            elif sys.platform.startswith('linux'):
                ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyUSB2']
            else:  # macOS
                ports = ['/dev/tty.usbserial-0', '/dev/tty.usbserial-1', '/dev/tty.usbserial-2']
        
        # Initialize serial connections
        for i, port in enumerate(ports):
            try:
                if SERIAL_AVAILABLE:
                    conn = serial.Serial(port, baudrate, timeout=1)
                    self.ports.append({
                        'id': i,
                        'port': port,
                        'connection': conn,
                        'active': True
                    })
                    print(f"[USB{i}] Connected to {port} at {baudrate} baud")
                else:
                    self.ports.append({
                        'id': i,
                        'port': port,
                        'connection': None,
                        'active': False
                    })
            except Exception as e:
                print(f"[USB{i}] Failed to open {port}: {e}")
                self.ports.append({
                    'id': i,
                    'port': port,
                    'connection': None,
                    'active': False
                })
        
        # Check if any ports are active
        active_count = sum(1 for p in self.ports if p['active'])
        if active_count == 0:
            print("WARNING: No USB ports connected. Using stdout mode.")
        else:
            print(f"Successfully connected to {active_count}/3 USB ports")
    
    def start_sender_threads(self):
        """Start background threads for each USB port"""
        self.running = True
        
        for i in range(3):
            thread = threading.Thread(target=self._sender_loop, args=(i,), daemon=True)
            thread.start()
            self.sender_threads.append(thread)
            print(f"[USB{i}] Sender thread started")
    
    def _sender_loop(self, port_id):
        """Background thread that sends commands from queue"""
        while self.running:
            try:
                # Get command from queue (blocks with timeout)
                cmd = self.command_queues[port_id].get(timeout=0.1)
                
                if self.ports[port_id]['active']:
                    # Send via serial
                    conn = self.ports[port_id]['connection']
                    conn.write(f"{cmd}\n".encode())
                    
                    # Read response
                    response = conn.readline().decode().strip()
                    print(f"[USB{port_id} TX] {cmd}")
                    print(f"[USB{port_id} RX] {response}")
                else:
                    # Fallback to stdout
                    print(f"[USB{port_id} STDOUT] {cmd}")
                
                self.command_queues[port_id].task_done()
                
            except Exception as e:
                if not isinstance(e, Exception) or 'Empty' not in str(type(e)):
                    print(f"[USB{port_id}] Error: {e}")
    
    def send_command(self, port_id, command):
        """Queue a command to be sent on specific USB port"""
        if 0 <= port_id < 3:
            self.command_queues[port_id].put(command)
        else:
            print(f"ERROR: Invalid port_id {port_id}")
    
    def send_concurrent(self, commands):
        """Send multiple commands simultaneously across different ports"""
        for i, cmd in enumerate(commands):
            if cmd and i < 3:
                self.send_command(i, cmd)
    
    def close(self):
        """Close all connections"""
        self.running = False
        
        for thread in self.sender_threads:
            thread.join(timeout=1)
        
        for port in self.ports:
            if port['active'] and port['connection']:
                port['connection'].close()
                print(f"[USB{port['id']}] Closed")


class CoachEventGenerator:
    """High-level event generator with multi-USB support"""
    
    def __init__(self, usb_controller):
        self.usb = usb_controller
        self.num_cabins = 10
        
        self.cabin_states = {
            'lights': [False] * self.num_cabins,
            'temperatures': [22] * self.num_cabins,
            'emergencies': [False] * self.num_cabins,
            'fires': [False] * self.num_cabins
        }
    
    def toggle_light(self, cabin_id, usb_port=0):
        """Toggle light on specific USB port"""
        self.cabin_states['lights'][cabin_id] = not self.cabin_states['lights'][cabin_id]
        state = "ON" if self.cabin_states['lights'][cabin_id] else "OFF"
        self.usb.send_command(usb_port, f"LIGHT {cabin_id} {state}")
    
    def set_temperature(self, cabin_id, temp, usb_port=0):
        """Set temperature via specific USB port"""
        if 10 <= temp <= 35:
            self.cabin_states['temperatures'][cabin_id] = temp
            self.usb.send_command(usb_port, f"TEMP {cabin_id} {temp}")
    
    def trigger_emergency(self, cabin_id, usb_port=0):
        """Trigger emergency via specific USB port"""
        self.cabin_states['emergencies'][cabin_id] = True
        self.usb.send_command(usb_port, f"EMERGENCY {cabin_id}")
    
    def trigger_fire(self, cabin_id, usb_port=0):
        """Trigger fire alert via specific USB port"""
        self.cabin_states['fires'][cabin_id] = True
        self.usb.send_command(usb_port, f"FIRE {cabin_id}")
    
    def set_power(self, state, usb_port=0):
        """Set power state via specific USB port"""
        self.usb.send_command(usb_port, f"POWER {state}")
    
    def pull_chain(self, usb_port=0):
        """Simulate chain pull via specific USB port"""
        self.usb.send_command(usb_port, "CHAIN PULL")
    
    def send_concurrent_scenario(self, scenario_name):
        """Send pre-defined concurrent test scenarios"""
        scenarios = {
            'lights': [
                "LIGHT 0 ON",
                "LIGHT 3 ON", 
                "LIGHT 7 ON"
            ],
            'mixed': [
                "LIGHT 1 ON",
                "TEMP 4 25",
                "LIGHT 8 OFF"
            ],
            'emergency': [
                "EMERGENCY 2",
                "FIRE 5",
                "CHAIN PULL"
            ],
            'stress_test': [
                f"LIGHT {random.randint(0,9)} ON",
                f"TEMP {random.randint(0,9)} {random.randint(18,28)}",
                f"LIGHT {random.randint(0,9)} OFF"
            ]
        }
        
        if scenario_name in scenarios:
            print(f"\n=== CONCURRENT SCENARIO: {scenario_name.upper()} ===")
            self.usb.send_concurrent(scenarios[scenario_name])
        else:
            print(f"Unknown scenario: {scenario_name}")


class MultiUSBGUI:
    """GUI with multi-USB port assignment"""
    
    def __init__(self, generator):
        self.generator = generator
        self.usb = generator.usb
        self.root = Tk()
        self.root.title("Indian Railways - Multi-USB Coach Control")
        self.root.geometry("1000x750")
        self.root.configure(bg='#2c3e50')
        
        self.create_widgets()
    
    def create_widgets(self):
        # Title
        title = Label(self.root,
                     text="INDIAN RAILWAYS\nMulti-USB Concurrent Event Generator",
                     font=('Arial', 18, 'bold'),
                     bg='#2c3e50', fg='white', pady=15)
        title.pack()
        
        # USB Status
        status_frame = Frame(self.root, bg='#34495e', relief=RAISED, bd=2)
        status_frame.pack(padx=10, pady=5, fill=X)
        
        Label(status_frame, text="USB PORT STATUS:",
              font=('Arial', 12, 'bold'),
              bg='#34495e', fg='white').pack(side=LEFT, padx=10)
        
        for port in self.usb.ports:
            color = 'green' if port['active'] else 'red'
            status = 'ACTIVE' if port['active'] else 'INACTIVE'
            Label(status_frame,
                  text=f"USB{port['id']}: {status}",
                  font=('Arial', 10),
                  bg='#34495e', fg=color).pack(side=LEFT, padx=10)
        
        # Main control frame
        main_frame = Frame(self.root, bg='#34495e', relief=RAISED, bd=2)
        main_frame.pack(padx=20, pady=10, fill=BOTH, expand=True)
        
        # USB Port selector
        self.selected_usb = IntVar(value=0)
        usb_selector = Frame(main_frame, bg='#34495e')
        usb_selector.pack(pady=10)
        
        Label(usb_selector, text="Send commands via:",
              font=('Arial', 11, 'bold'),
              bg='#34495e', fg='white').pack(side=LEFT, padx=5)
        
        for i in range(3):
            Radiobutton(usb_selector, text=f"USB {i}",
                       variable=self.selected_usb, value=i,
                       font=('Arial', 10),
                       bg='#34495e', fg='white',
                       selectcolor='#2c3e50').pack(side=LEFT, padx=5)
        
        # Cabin controls
        cabin_frame = Frame(main_frame, bg='#34495e')
        cabin_frame.pack(pady=5)
        
        Label(cabin_frame, text="CABIN CONTROLS (Select USB port above)",
              font=('Arial', 12, 'bold'),
              bg='#34495e', fg='white').grid(row=0, column=0, columnspan=5, pady=5)
        
        for i in range(10):
            row = (i // 5) + 1
            col = i % 5
            
            cabin_box = Frame(cabin_frame, bg='#2c3e50', relief=RAISED, bd=2)
            cabin_box.grid(row=row, column=col, padx=3, pady=3)
            
            Label(cabin_box, text=f"Cabin {i}",
                  font=('Arial', 9, 'bold'),
                  bg='#2c3e50', fg='white').pack()
            
            Button(cabin_box, text="💡",
                   command=lambda x=i: self.toggle_light(x),
                   bg='#3498db', fg='white', width=8).pack(pady=1)
            
            temp_f = Frame(cabin_box, bg='#2c3e50')
            temp_f.pack(pady=1)
            Button(temp_f, text="-", command=lambda x=i: self.adjust_temp(x, -2),
                   bg='#3498db', fg='white', width=3).pack(side=LEFT)
            Label(temp_f, text="🌡️", bg='#2c3e50', fg='white',
                  font=('Arial', 8)).pack(side=LEFT, padx=1)
            Button(temp_f, text="+", command=lambda x=i: self.adjust_temp(x, 2),
                   bg='#3498db', fg='white', width=3).pack(side=LEFT)
            
            Button(cabin_box, text="🚨",
                   command=lambda x=i: self.trigger_emergency(x),
                   bg='#e74c3c', fg='white', width=8).pack(pady=1)
            
            Button(cabin_box, text="🔥",
                   command=lambda x=i: self.trigger_fire(x),
                   bg='#e67e22', fg='white', width=8).pack(pady=1)
        
        # Concurrent scenario buttons
        concurrent_frame = Frame(main_frame, bg='#34495e')
        concurrent_frame.pack(pady=10)
        
        Label(concurrent_frame, text="CONCURRENT SCENARIOS (Uses all 3 USB ports)",
              font=('Arial', 12, 'bold'),
              bg='#34495e', fg='white').pack()
        
        scenario_buttons = Frame(concurrent_frame, bg='#34495e')
        scenario_buttons.pack(pady=5)
        
        Button(scenario_buttons, text="⚡ Lights Test",
               command=lambda: self.generator.send_concurrent_scenario('lights'),
               bg='#9b59b6', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        Button(scenario_buttons, text="🔀 Mixed Commands",
               command=lambda: self.generator.send_concurrent_scenario('mixed'),
               bg='#3498db', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        Button(scenario_buttons, text="🚨 Emergency Test",
               command=lambda: self.generator.send_concurrent_scenario('emergency'),
               bg='#e74c3c', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        Button(scenario_buttons, text="💥 Stress Test",
               command=lambda: self.generator.send_concurrent_scenario('stress_test'),
               bg='#c0392b', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        # System controls
        system_frame = Frame(main_frame, bg='#34495e')
        system_frame.pack(pady=10)
        
        Button(system_frame, text="⚡ Power LOW",
               command=lambda: self.generator.set_power('LOW', self.selected_usb.get()),
               bg='#f39c12', fg='white', width=12, height=2).pack(side=LEFT, padx=5)
        
        Button(system_frame, text="✓ Power NORMAL",
               command=lambda: self.generator.set_power('NORMAL', self.selected_usb.get()),
               bg='#27ae60', fg='white', width=12, height=2).pack(side=LEFT, padx=5)
        
        Button(system_frame, text="⛓️ Pull Chain",
               command=lambda: self.generator.pull_chain(self.selected_usb.get()),
               bg='#c0392b', fg='white', width=12, height=2).pack(side=LEFT, padx=5)
    
    def toggle_light(self, cabin_id):
        self.generator.toggle_light(cabin_id, self.selected_usb.get())
    
    def adjust_temp(self, cabin_id, delta):
        current = self.generator.cabin_states['temperatures'][cabin_id]
        new_temp = max(10, min(35, current + delta))
        self.generator.set_temperature(cabin_id, new_temp, self.selected_usb.get())
    
    def trigger_emergency(self, cabin_id):
        self.generator.trigger_emergency(cabin_id, self.selected_usb.get())
        messagebox.showwarning("Emergency", f"Emergency in Cabin {cabin_id} via USB{self.selected_usb.get()}")
    
    def trigger_fire(self, cabin_id):
        self.generator.trigger_fire(cabin_id, self.selected_usb.get())
        messagebox.showerror("Fire", f"Fire in Cabin {cabin_id} via USB{self.selected_usb.get()}")
    
    def run(self):
        self.root.mainloop()


def main():
    print("=" * 60)
    print("Indian Railways Multi-USB Concurrent Event Generator")
    print("=" * 60)
    
    # Parse command line arguments for custom ports
    ports = None
    if len(sys.argv) > 1:
        ports = sys.argv[1:4]  # Take up to 3 ports from command line
    
    # Initialize multi-USB controller
    usb_controller = MultiUSBController(ports=ports)
    usb_controller.start_sender_threads()
    
    # Create event generator
    generator = CoachEventGenerator(usb_controller)
    
    if TKINTER_AVAILABLE:
        print("\nLaunching GUI...")
        gui = MultiUSBGUI(generator)
        
        try:
            gui.run()
        finally:
            usb_controller.close()
    else:
        print("\nGUI not available, running CLI mode")
        print("Commands: light <cabin> <port>, temp <cabin> <val> <port>, etc.")
        print("Type 'quit' to exit\n")
        
        try:
            while True:
                cmd = input("> ").strip().split()
                if not cmd:
                    continue
                if cmd[0] == 'quit':
                    break
                # Add CLI handling here
        finally:
            usb_controller.close()
    
    print("\nGoodbye!")


if __name__ == "__main__":
    main()