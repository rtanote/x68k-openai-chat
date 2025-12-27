#!/usr/bin/env python3
"""
Echo Server for Testing

Simple echo server that responds to messages without calling OpenAI API.
Useful for testing serial communication before integrating with the real API.

Usage:
    python echo_server.py [COM_PORT]

Example:
    python echo_server.py COM11
"""

import sys
import time
import serial

# Protocol constants
ETX = 0x04

def main():
    # Get COM port from command line or use default
    port = sys.argv[1] if len(sys.argv) > 1 else 'COM11'

    print(f"Echo Server - Testing serial communication")
    print(f"Opening {port}...")

    try:
        ser = serial.Serial(
            port=port,
            baudrate=9600,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1
        )
        print(f"Serial port opened: {port} @ 9600 bps")
        print("Waiting for messages... (Ctrl+C to quit)")
        print("-" * 40)

    except serial.SerialException as e:
        print(f"Error: Could not open {port}: {e}")
        sys.exit(1)

    buffer = bytearray()

    while True:
        try:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                for b in data:
                    if b == ETX:
                        # Message complete
                        try:
                            message = buffer.decode('shift_jis')
                        except UnicodeDecodeError:
                            message = buffer.decode('latin-1')

                        print(f"Received: {message}")

                        # Create echo response
                        response = f"Echo: {message}"
                        print(f"Sending:  {response}")

                        # Send response
                        try:
                            response_bytes = response.encode('shift_jis')
                        except UnicodeEncodeError:
                            response_bytes = response.encode('ascii', errors='replace')

                        ser.write(response_bytes)
                        ser.write(bytes([ETX]))
                        ser.flush()

                        buffer.clear()
                        print("-" * 40)
                    else:
                        buffer.append(b)

            time.sleep(0.01)

        except KeyboardInterrupt:
            print("\nShutting down...")
            break

    ser.close()
    print("Done.")


if __name__ == '__main__':
    main()
