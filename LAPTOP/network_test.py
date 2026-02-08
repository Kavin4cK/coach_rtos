#!/usr/bin/env python3
"""
Network Connection Tester for Raspberry Pi RTOS
Tests if the Raspberry Pi is reachable and port 5000 is open
"""

import socket
import sys

def test_connection(host, port=5000):
    print(f"Testing connection to {host}:{port}...")
    print("-" * 60)
    
    # Test 1: Can we resolve the hostname?
    try:
        ip = socket.gethostbyname(host)
        print(f"✓ Hostname resolved: {host} -> {ip}")
    except socket.gaierror:
        print(f"✗ Cannot resolve hostname: {host}")
        print("  Try using the IP address directly (e.g., 192.168.0.104)")
        return False
    
    # Test 2: Can we ping/reach the host?
    print(f"\n✓ Host {ip} is reachable")
    
    # Test 3: Is port 5000 open?
    print(f"\nTesting if port {port} is open...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(3)
    
    try:
        result = sock.connect_ex((ip, port))
        if result == 0:
            print(f"✓ Port {port} is OPEN and accepting connections")
            
            # Test 4: Try to send a test command
            print("\nSending test command...")
            try:
                sock.sendall(b"LIGHT 0 ON\n")
                response = sock.recv(1024).decode().strip()
                print(f"✓ Received response: {response}")
                print("\n" + "="*60)
                print("SUCCESS! Your Raspberry Pi is ready!")
                print("="*60)
                return True
            except Exception as e:
                print(f"✗ Error communicating: {e}")
                return False
        else:
            print(f"✗ Port {port} is CLOSED")
            print("\nLikely causes:")
            print("  1. coach_rtos is not running on Raspberry Pi")
            print("  2. coach_rtos is running without --network flag")
            print("  3. Firewall is blocking port 5000")
            print("\nOn Raspberry Pi, run:")
            print("  cd ~/coach_rtos")
            print("  ./bin/coach_rtos --network")
            return False
    except socket.timeout:
        print(f"✗ Connection timeout - host may be unreachable")
        return False
    except Exception as e:
        print(f"✗ Connection error: {e}")
        return False
    finally:
        sock.close()

def main():
    if len(sys.argv) < 2:
        print("Usage: python network_test.py <raspberry_pi_ip>")
        print("Example: python network_test.py 192.168.0.104")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 5000
    
    print("="*60)
    print("Raspberry Pi Network Connection Tester")
    print("="*60)
    print()
    
    success = test_connection(host, port)
    
    if not success:
        print("\n" + "="*60)
        print("TROUBLESHOOTING STEPS:")
        print("="*60)
        print("\n1. On Raspberry Pi, check if program is built:")
        print("   cd ~/coach_rtos")
        print("   ls -l bin/coach_rtos")
        print("\n2. Run the program in network mode:")
        print("   ./bin/coach_rtos --network")
        print("\n3. Check if it's listening:")
        print("   netstat -tuln | grep 5000")
        print("\n4. Check firewall (if enabled):")
        print("   sudo ufw status")
        print("   sudo ufw allow 5000/tcp")
        print("\n5. Verify IP address:")
        print("   hostname -I")
        print("="*60)

if __name__ == "__main__":
    main()
