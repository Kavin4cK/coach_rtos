#!/usr/bin/env python3
"""
Indian Railways Coach Event Generator - Network Version
Sends commands to Raspberry Pi over TCP/IP instead of USB
"""

import sys
import time
import random
import threading
import socket
from queue import Queue

try:
    from tkinter import *
    from tkinter import ttk, messagebox
    TKINTER_AVAILABLE = True
except ImportError:
    TKINTER_AVAILABLE = False
    print("Warning: tkinter not available")

class NetworkController:
    """Connects to Raspberry Pi over network"""
    
    def __init__(self, host='raspberrypi.local', port=5000):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5)
            self.sock.connect((host, port))
            self.connected = True
            print(f"✓ Connected to Raspberry Pi at {host}:{port}")
        except Exception as e:
            print(f"✗ Could not connect to {host}:{port}: {e}")
            print("  Running in STDOUT mode (simulation)")
            self.connected = False
    
    def send_command(self, command):
        """Send command over network or print to stdout"""
        if self.connected:
            try:
                self.sock.sendall(f"{command}\n".encode())
                response = self.sock.recv(1024).decode().strip()
                print(f"[TX] {command}")
                print(f"[RX] {response}")
            except Exception as e:
                print(f"[ERROR] {e}")
                self.connected = False
        else:
            print(f"[STDOUT] {command}")
    
    def close(self):
        if self.sock:
            self.sock.close()


class CoachEventGenerator:
    """Event generator with network support"""
    
    def __init__(self, controller):
        self.controller = controller
        self.num_cabins = 10
        
        self.cabin_states = {
            'lights': [False] * self.num_cabins,
            'temperatures': [22] * self.num_cabins,
        }
    
    def toggle_light(self, cabin_id):
        """Toggle light"""
        self.cabin_states['lights'][cabin_id] = not self.cabin_states['lights'][cabin_id]
        state = "ON" if self.cabin_states['lights'][cabin_id] else "OFF"
        self.controller.send_command(f"LIGHT {cabin_id} {state}")
    
    def set_temperature(self, cabin_id, temp):
        """Set temperature"""
        if 10 <= temp <= 35:
            self.cabin_states['temperatures'][cabin_id] = temp
            self.controller.send_command(f"TEMP {cabin_id} {temp}")
    
    def trigger_emergency(self, cabin_id):
        """Trigger emergency"""
        self.controller.send_command(f"EMERGENCY {cabin_id}")
    
    def trigger_fire(self, cabin_id):
        """Trigger fire alert"""
        self.controller.send_command(f"FIRE {cabin_id}")
    
    def set_power(self, state):
        """Set power state"""
        self.controller.send_command(f"POWER {state}")
    
    def pull_chain(self):
        """Simulate chain pull"""
        self.controller.send_command("CHAIN PULL")


class NetworkGUI:
    """Simplified GUI for network mode"""
    
    def __init__(self, generator):
        self.generator = generator
        self.root = Tk()
        self.root.title("Indian Railways - Network Coach Control")
        self.root.geometry("800x650")
        self.root.configure(bg='#2c3e50')
        
        self.create_widgets()
    
    def create_widgets(self):
        # Title
        title = Label(self.root,
                     text="INDIAN RAILWAYS\nNetwork Event Generator",
                     font=('Arial', 18, 'bold'),
                     bg='#2c3e50', fg='white', pady=15)
        title.pack()
        
        # Status
        status_frame = Frame(self.root, bg='#34495e', relief=RAISED, bd=2)
        status_frame.pack(padx=10, pady=5, fill=X)
        
        status_color = 'green' if self.generator.controller.connected else 'red'
        status_text = 'CONNECTED' if self.generator.controller.connected else 'DISCONNECTED (STDOUT MODE)'
        
        Label(status_frame, text=f"Status: {status_text}",
              font=('Arial', 12, 'bold'),
              bg='#34495e', fg=status_color).pack(padx=10, pady=5)
        
        # Main control frame
        main_frame = Frame(self.root, bg='#34495e', relief=RAISED, bd=2)
        main_frame.pack(padx=20, pady=10, fill=BOTH, expand=True)
        
        # Cabin controls
        cabin_frame = Frame(main_frame, bg='#34495e')
        cabin_frame.pack(pady=10)
        
        Label(cabin_frame, text="CABIN CONTROLS",
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
            
            Button(cabin_box, text="💡 Light",
                   command=lambda x=i: self.toggle_light(x),
                   bg='#3498db', fg='white', width=10).pack(pady=1)
            
            temp_f = Frame(cabin_box, bg='#2c3e50')
            temp_f.pack(pady=1)
            Button(temp_f, text="-", command=lambda x=i: self.adjust_temp(x, -2),
                   bg='#3498db', fg='white', width=3).pack(side=LEFT)
            Label(temp_f, text="🌡️", bg='#2c3e50', fg='white',
                  font=('Arial', 8)).pack(side=LEFT, padx=1)
            Button(temp_f, text="+", command=lambda x=i: self.adjust_temp(x, 2),
                   bg='#3498db', fg='white', width=3).pack(side=LEFT)
            
            Button(cabin_box, text="🚨 Emergency",
                   command=lambda x=i: self.trigger_emergency(x),
                   bg='#e74c3c', fg='white', width=10).pack(pady=1)
            
            Button(cabin_box, text="🔥 Fire",
                   command=lambda x=i: self.trigger_fire(x),
                   bg='#e67e22', fg='white', width=10).pack(pady=1)
        
        # System controls
        system_frame = Frame(main_frame, bg='#34495e')
        system_frame.pack(pady=15)
        
        Label(system_frame, text="SYSTEM CONTROLS",
              font=('Arial', 12, 'bold'),
              bg='#34495e', fg='white').pack(pady=5)
        
        btn_frame = Frame(system_frame, bg='#34495e')
        btn_frame.pack()
        
        Button(btn_frame, text="⚡ Power LOW",
               command=lambda: self.generator.set_power('LOW'),
               bg='#f39c12', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        Button(btn_frame, text="✓ Power NORMAL",
               command=lambda: self.generator.set_power('NORMAL'),
               bg='#27ae60', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
        
        Button(btn_frame, text="⛓️ Pull Chain",
               command=lambda: self.generator.pull_chain(),
               bg='#c0392b', fg='white', width=15, height=2).pack(side=LEFT, padx=5)
    
    def toggle_light(self, cabin_id):
        self.generator.toggle_light(cabin_id)
    
    def adjust_temp(self, cabin_id, delta):
        current = self.generator.cabin_states['temperatures'][cabin_id]
        new_temp = max(10, min(35, current + delta))
        self.generator.set_temperature(cabin_id, new_temp)
    
    def trigger_emergency(self, cabin_id):
        self.generator.trigger_emergency(cabin_id)
        messagebox.showwarning("Emergency", f"Emergency triggered in Cabin {cabin_id}")
    
    def trigger_fire(self, cabin_id):
        self.generator.trigger_fire(cabin_id)
        messagebox.showerror("Fire", f"Fire alert in Cabin {cabin_id}")
    
    def run(self):
        self.root.mainloop()


def main():
    print("=" * 60)
    print("Indian Railways Network Event Generator")
    print("=" * 60)
    
    # Get host from command line or use default
    host = sys.argv[1] if len(sys.argv) > 1 else 'raspberrypi.local'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 5000
    
    # Initialize network controller
    controller = NetworkController(host=host, port=port)
    
    # Create event generator
    generator = CoachEventGenerator(controller)
    
    if TKINTER_AVAILABLE:
        print("\nLaunching GUI...")
        gui = NetworkGUI(generator)
        
        try:
            gui.run()
        finally:
            controller.close()
    else:
        print("\nGUI not available")
        controller.close()
    
    print("\nGoodbye!")


if __name__ == "__main__":
    main()
