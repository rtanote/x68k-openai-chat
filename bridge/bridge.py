#!/usr/bin/env python3
"""
X68000 OpenAI Chat Bridge

Bridges serial communication from X68000 to OpenAI API.
Handles Shift_JIS <-> UTF-8 encoding conversion.
"""

import os
import sys
import time
import threading
import logging
from pathlib import Path
from typing import Optional

import yaml
import serial
from openai import OpenAI

# Protocol constants
ETX = 0x04  # End of transmission
KEEPALIVE = 0x00  # Keepalive byte (NUL)
KEEPALIVE_INTERVAL = 5  # seconds
ERROR_PREFIX = "ERROR:"

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


def load_config(config_path: str) -> dict:
    """Load configuration from YAML file with environment variable substitution."""
    with open(config_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Substitute environment variables
    for key, value in os.environ.items():
        content = content.replace(f'${{{key}}}', value)

    config = yaml.safe_load(content)
    return config


class ChatBridge:
    """Bridge between X68000 serial and OpenAI API."""

    def __init__(self, config: dict):
        self.config = config
        self.serial_port = None
        self.openai_client = None
        self.keepalive_thread = None
        self.keepalive_stop = threading.Event()
        self.conversation_history = []

    def start(self):
        """Initialize and start the bridge."""
        logger.info("Starting X68000 OpenAI Chat Bridge...")

        # Initialize serial port
        serial_config = self.config['serial']
        try:
            self.serial_port = serial.Serial(
                port=serial_config['port'],
                baudrate=serial_config['baud_rate'],
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1  # Short timeout for polling
            )
            logger.info(f"Serial port opened: {serial_config['port']} @ {serial_config['baud_rate']} bps")
        except serial.SerialException as e:
            logger.error(f"Failed to open serial port: {e}")
            raise

        # Initialize OpenAI client
        openai_config = self.config['openai']
        api_key = openai_config['api_key']
        if not api_key or api_key.startswith('${'):
            logger.error("OpenAI API key not configured. Set OPENAI_API_KEY environment variable.")
            raise ValueError("Missing API key")

        self.openai_client = OpenAI(api_key=api_key)
        logger.info("OpenAI client initialized")

        # Main loop
        logger.info("Bridge ready. Waiting for messages...")
        self.run()

    def run(self):
        """Main message processing loop."""
        while True:
            try:
                # Receive message from X68000
                message = self.receive_message()
                if message is None:
                    continue

                logger.info(f"Received: {message[:50]}..." if len(message) > 50 else f"Received: {message}")

                # Call OpenAI API with keepalive
                response = self.call_openai(message)

                # Send response back
                self.send_response(response)
                logger.info(f"Sent response ({len(response)} chars)")

            except KeyboardInterrupt:
                logger.info("Shutting down...")
                break
            except Exception as e:
                logger.error(f"Error in main loop: {e}")
                self.send_error(str(e))

    def receive_message(self) -> Optional[str]:
        """Read message from serial until ETX."""
        buffer = bytearray()

        while True:
            if self.serial_port.in_waiting > 0:
                byte = self.serial_port.read(1)
                if len(byte) == 0:
                    continue

                b = byte[0]
                if b == ETX:
                    break
                buffer.append(b)
            else:
                # No data, short sleep to prevent busy waiting
                time.sleep(0.01)

                # Return None if buffer is empty (no message yet)
                if len(buffer) == 0:
                    return None

        if len(buffer) == 0:
            return None

        # Convert from Shift_JIS to UTF-8
        try:
            message = buffer.decode('shift_jis')
        except UnicodeDecodeError:
            # Try as raw bytes if not valid Shift_JIS
            message = buffer.decode('latin-1')

        return message

    def call_openai(self, message: str) -> str:
        """Call OpenAI API with keepalive thread."""
        openai_config = self.config['openai']

        # Start keepalive thread
        self.keepalive_stop.clear()
        self.keepalive_thread = threading.Thread(target=self._keepalive_sender)
        self.keepalive_thread.start()

        try:
            # Build messages list
            messages = [
                {"role": "system", "content": self.config.get('system_prompt', '')}
            ]
            messages.extend(self.conversation_history)
            messages.append({"role": "user", "content": message})

            # Call API
            response = self.openai_client.chat.completions.create(
                model=openai_config['model'],
                messages=messages,
                max_tokens=openai_config.get('max_tokens', 1024)
            )

            assistant_message = response.choices[0].message.content

            # Update conversation history (keep last 10 exchanges)
            self.conversation_history.append({"role": "user", "content": message})
            self.conversation_history.append({"role": "assistant", "content": assistant_message})
            if len(self.conversation_history) > 20:
                self.conversation_history = self.conversation_history[-20:]

            return assistant_message

        except Exception as e:
            logger.error(f"OpenAI API error: {e}")
            return f"{ERROR_PREFIX} {str(e)}"

        finally:
            # Stop keepalive thread
            self.keepalive_stop.set()
            self.keepalive_thread.join()

    def _keepalive_sender(self):
        """Send keepalive bytes periodically."""
        while not self.keepalive_stop.wait(KEEPALIVE_INTERVAL):
            try:
                self.serial_port.write(bytes([KEEPALIVE]))
                self.serial_port.flush()
                logger.debug("Sent keepalive")
            except Exception as e:
                logger.error(f"Failed to send keepalive: {e}")

    def send_response(self, response: str):
        """Send response via serial with ETX terminator."""
        try:
            # Convert from UTF-8 to Shift_JIS
            data = response.encode('shift_jis', errors='replace')
        except Exception:
            data = response.encode('ascii', errors='replace')

        # Send data + ETX
        self.serial_port.write(data)
        self.serial_port.write(bytes([ETX]))
        self.serial_port.flush()

    def send_error(self, error_message: str):
        """Send error message via serial."""
        self.send_response(f"{ERROR_PREFIX} {error_message}")

    def stop(self):
        """Cleanup and stop the bridge."""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            logger.info("Serial port closed")


def main():
    """Main entry point."""
    # Find config file
    script_dir = Path(__file__).parent
    config_path = script_dir / 'config.yaml'

    if not config_path.exists():
        logger.error(f"Config file not found: {config_path}")
        sys.exit(1)

    # Load configuration
    try:
        config = load_config(str(config_path))
    except Exception as e:
        logger.error(f"Failed to load config: {e}")
        sys.exit(1)

    # Create and start bridge
    bridge = ChatBridge(config)
    try:
        bridge.start()
    except KeyboardInterrupt:
        pass
    finally:
        bridge.stop()


if __name__ == '__main__':
    main()
