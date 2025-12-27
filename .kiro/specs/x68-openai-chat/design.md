# X68000 OpenAI Chat System - Design Document

## Overview

The X68000 OpenAI Chat System enables vintage X68000 computers to interact with modern AI through a bridge architecture. The X68000 communicates via RS-232C serial to a bridge device (Raspberry Pi or ESP32), which translates requests to HTTPS calls to the OpenAI API.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              X68000                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────────┐  │
│  │   μEmacs     │  │   chat.x     │  │   (Future) chatgfx.x         │  │
│  │  Integration │  │   CLI Tool   │  │   Graphical Frontend         │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┬───────────────┘  │
│         │                 │                         │                   │
│         └────────────────┼─────────────────────────┘                   │
│                          │                                              │
│                   ┌──────┴───────┐                                      │
│                   │  libchat.a   │                                      │
│                   │ Communication│                                      │
│                   │   Library    │                                      │
│                   └──────┬───────┘                                      │
│                          │                                              │
│                   ┌──────┴───────┐                                      │
│                   │   RS-232C    │                                      │
│                   │    IOCS      │                                      │
│                   └──────┬───────┘                                      │
└──────────────────────────┼──────────────────────────────────────────────┘
                           │ Serial (9600-38400bps)
                           │
┌──────────────────────────┼──────────────────────────────────────────────┐
│                   Bridge Device (RPi/ESP32)                             │
│                   ┌──────┴───────┐                                      │
│                   │    Serial    │                                      │
│                   │   Handler    │                                      │
│                   └──────┬───────┘                                      │
│                          │                                              │
│                   ┌──────┴───────┐                                      │
│                   │   Protocol   │                                      │
│                   │   Handler    │                                      │
│                   └──────┬───────┘                                      │
│                          │                                              │
│                   ┌──────┴───────┐                                      │
│                   │   OpenAI     │                                      │
│                   │   Client     │                                      │
│                   └──────┬───────┘                                      │
└──────────────────────────┼──────────────────────────────────────────────┘
                           │ HTTPS
                           ▼
                   ┌───────────────┐
                   │  OpenAI API   │
                   │ api.openai.com│
                   └───────────────┘
```

## Component Design

### 1. Communication Library (libchat.a)

The core library provides serial communication abstraction for all X68000 applications.

#### Public API

```c
/* chat.h - Public API */

#ifndef CHAT_H
#define CHAT_H

#include <stddef.h>

/* Error codes */
#define CHAT_OK              0
#define CHAT_ERR_INIT       -1
#define CHAT_ERR_TIMEOUT    -2
#define CHAT_ERR_SEND       -3
#define CHAT_ERR_RECV       -4
#define CHAT_ERR_PROTOCOL   -5
#define CHAT_ERR_BUFFER     -6

/* Configuration */
typedef struct {
    int baud_rate;          /* 9600, 19200, 38400 */
    int timeout_sec;        /* Receive timeout in seconds */
    size_t buffer_size;     /* Internal buffer size */
} chat_config_t;

/* Default configuration */
#define CHAT_DEFAULT_BAUD       9600
#define CHAT_DEFAULT_TIMEOUT    60
#define CHAT_DEFAULT_BUFSIZE    4096

/* Initialize communication */
int chat_init(const chat_config_t *config);

/* Cleanup and close */
void chat_cleanup(void);

/* Send message and receive response */
int chat_send(const char *message, size_t len);
int chat_recv(char *buffer, size_t bufsize, size_t *received);

/* Combined send/receive operation */
int chat_query(const char *message, char *response, size_t resp_size, size_t *resp_len);

/* Check if data is available */
int chat_available(void);

/* Get last error message */
const char *chat_error_string(int error_code);

#endif /* CHAT_H */
```

#### Internal Structure

```c
/* chat_internal.h - Internal structures */

/* Protocol constants */
#define PROTO_ETX           0x04    /* End of transmission */
#define PROTO_KEEPALIVE     0x00    /* Keepalive byte */
#define PROTO_ERROR_PREFIX  "ERROR:"

/* Serial port state */
typedef struct {
    int initialized;
    int baud_rate;
    int timeout_sec;
    char *recv_buffer;
    size_t buffer_size;
    size_t buffer_pos;
} chat_state_t;
```

### 2. Protocol Specification

#### Message Format

```
Request:  <UTF-8 text><ETX>
Response: <UTF-8 text><ETX>
Error:    ERROR:<message><ETX>
Keepalive: <NUL> (sent every 5 seconds during processing)
```

#### Sequence Diagram - Normal Flow

```
X68000 (chat.x)              Bridge                    OpenAI API
       │                        │                           │
       │──── "Hello" + ETX ────>│                           │
       │                        │                           │
       │                        │─── POST /v1/chat ────────>│
       │                        │    completions            │
       │                        │                           │
       │<─────── NUL ───────────│ (keepalive if >5s)        │
       │                        │                           │
       │                        │<── Response JSON ─────────│
       │                        │                           │
       │<── "Response" + ETX ───│                           │
       │                        │                           │
```

#### Sequence Diagram - Error Flow

```
X68000 (chat.x)              Bridge                    OpenAI API
       │                        │                           │
       │──── "Hello" + ETX ────>│                           │
       │                        │                           │
       │                        │─── POST /v1/chat ────────>│
       │                        │                           │
       │                        │<── 401 Unauthorized ──────│
       │                        │                           │
       │<─ "ERROR: Auth" + ETX ─│                           │
       │                        │                           │
```

### 3. Bridge Software Design

#### Architecture (Python/Raspberry Pi)

```python
# bridge.py - Conceptual structure

class ChatBridge:
    def __init__(self, config_path: str):
        self.config = load_config(config_path)
        self.serial = Serial(
            port=self.config.serial_port,
            baudrate=self.config.baud_rate
        )
        self.openai = OpenAI(api_key=self.config.api_key)
        self.system_prompt = self.config.system_prompt
    
    def run(self):
        """Main loop: receive -> process -> respond"""
        while True:
            message = self.receive_message()
            response = self.call_openai(message)
            self.send_response(response)
    
    def receive_message(self) -> str:
        """Read until ETX, decode UTF-8"""
        ...
    
    def call_openai(self, message: str) -> str:
        """Call API with keepalive thread"""
        ...
    
    def send_response(self, response: str):
        """Encode UTF-8, append ETX, transmit"""
        ...
```

#### Configuration File

```yaml
# config.yaml
serial:
  port: /dev/ttyUSB0
  baud_rate: 9600

openai:
  api_key: ${OPENAI_API_KEY}  # Environment variable
  model: gpt-4o-mini
  max_tokens: 1024

system_prompt: |
  You are a helpful assistant running on a vintage X68000 computer.
  Keep responses concise due to limited display capabilities.
```

### 4. CLI Tool Design (chat.x)

#### Command Line Interface

```
Usage: chat [options] [message]

Options:
  -i, --interactive    Enter interactive mode
  -b, --baud <rate>    Set baud rate (default: 9600)
  -t, --timeout <sec>  Set timeout (default: 60)
  -h, --help           Show this help

Examples:
  chat "What is X68000?"
  chat -i
  chat -b 38400 "Hello"
```

#### Program Flow

```
main()
├── parse_arguments()
├── chat_init()
├── if interactive_mode:
│   └── interactive_loop()
│       ├── print_prompt()
│       ├── read_input()
│       ├── chat_query()
│       └── print_response()
└── else:
    ├── chat_query(message)
    └── print_response()
```

### 5. μEmacs Integration Design

#### Integration Method

Since X68000's μEmacs may have limited external process support, integration uses a file-based approach:

```
1. User selects text or positions cursor
2. User invokes macro (Ctrl+C Ctrl+S)
3. Macro writes text to temporary file
4. Macro calls: chat < /tmp/query.txt > /tmp/response.txt
5. Macro reads response file
6. Macro inserts separator and response into buffer
```

#### μEmacs Macro (Conceptual)

```lisp
; chat-send - Send current paragraph to AI
(defun chat-send ()
  (save-region "/tmp/query.txt")
  (message "Thinking...")
  (shell-command "chat < /tmp/query.txt > /tmp/response.txt")
  (end-of-buffer)
  (insert "\n────────────────────────────────────────\n")
  (insert-file "/tmp/response.txt")
  (beep))
```

### 6. Character Encoding Strategy

#### Encoding Flow

```
X68000 (Shift_JIS)          Bridge                    OpenAI (UTF-8)
       │                        │                           │
       │── Shift_JIS text ─────>│                           │
       │                        │── Convert to UTF-8 ──>    │
       │                        │                           │
       │                        │── UTF-8 text ────────────>│
       │                        │                           │
       │                        │<── UTF-8 response ────────│
       │                        │                           │
       │<── Shift_JIS text ─────│<── Convert to Shift_JIS ──│
       │                        │                           │
```

#### Implementation Notes

- Bridge handles all encoding conversion
- X68000 applications work with native Shift_JIS
- ASCII-only mode (graphical frontend) bypasses conversion

## Directory Structure

```
x68k-openai-chat/
├── .kiro/
│   └── specs/
│       └── x68k-openai-chat/
│           ├── requirements.md
│           ├── design.md
│           └── tasks.md
├── src/
│   ├── libchat/
│   │   ├── chat.h              # Public API
│   │   ├── chat.c              # Implementation
│   │   ├── chat_internal.h     # Internal structures
│   │   ├── serial.c            # IOCS serial wrapper
│   │   └── protocol.c          # Protocol encoding/decoding
│   ├── cli/
│   │   └── main.c              # CLI tool entry point
│   └── gfx/                    # (Future) Graphical frontend
│       └── README.md           # Placeholder
├── bridge/
│   ├── bridge.py               # Main bridge application
│   ├── config.yaml             # Configuration template
│   └── requirements.txt        # Python dependencies
├── emacs/
│   └── chat.el                 # μEmacs macro/integration
├── Makefile
└── README.md
```

## Build System

### Development Environment (Windows)

```
D:\dev\
├── xdev68k\                    # Cross-compilation toolchain
│   ├── bin\
│   │   ├── m68k-elf-gcc.exe
│   │   ├── m68k-elf-ar.exe
│   │   └── ...
│   ├── include\
│   ├── lib\
│   └── run68\
│       └── elf2x68k.exe
│
└── x68k-openai-chat\           # This project
    ├── .kiro\specs\
    ├── src\
    ├── bridge\
    └── Makefile
```

### Emulator Serial Configuration

For development testing with XM6 TypeG on Windows:

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  XM6 TypeG  │      │   com0com   │      │   Python    │
│             │      │   Virtual   │      │   Bridge    │
│  RS-232C────┼──────┤►COM10─COM11◄├──────┼─►Serial     │
│             │      │   Pair      │      │             │
└─────────────┘      └─────────────┘      └─────────────┘
```

**com0com Setup:**
1. Install com0com from https://sourceforge.net/projects/com0com/
2. Create virtual pair: `COM10 ⇔ COM11`
3. Configure XM6 TypeG to use COM10
4. Configure bridge to use COM11

### Makefile Structure

```makefile
# Toolchain (xdev68k) - relative path from project root
XDEV68K_DIR = ../xdev68k
CC = $(XDEV68K_DIR)/bin/m68k-elf-gcc
AR = $(XDEV68K_DIR)/bin/m68k-elf-ar
ELF2X68K = $(XDEV68K_DIR)/run68/elf2x68k

# Include paths
CFLAGS = -I$(XDEV68K_DIR)/include -O2 -Wall

# Directories
SRCDIR = src
BUILDDIR = build
LIBDIR = $(BUILDDIR)/lib

# Library
LIBCHAT = $(LIBDIR)/libchat.a
LIBCHAT_SRCS = $(SRCDIR)/libchat/chat.c \
               $(SRCDIR)/libchat/serial.c \
               $(SRCDIR)/libchat/protocol.c

# CLI Tool
CLI_TARGET = $(BUILDDIR)/chat.x
CLI_SRCS = $(SRCDIR)/cli/main.c

# Build targets
all: $(LIBCHAT) $(CLI_TARGET)

$(LIBCHAT): $(LIBCHAT_SRCS:.c=.o)
	@mkdir -p $(LIBDIR)
	$(AR) rcs $@ $^

$(CLI_TARGET): $(CLI_SRCS:.c=.o) $(LIBCHAT)
	$(CC) -o $@.elf $^ $(CFLAGS)
	$(ELF2X68K) $@.elf $@

clean:
	rm -rf $(BUILDDIR) $(SRCDIR)/**/*.o
```

## Hardware Requirements

### X68000 Side

- RS-232C port (standard equipment)
- Recommended: X68000 XVI or later for better serial performance

### Bridge Side

**Option A: Raspberry Pi**
- Raspberry Pi 3/4/Zero W
- USB-Serial adapter (FTDI or CH340)
- Python 3.8+
- Network connectivity (Wi-Fi or Ethernet)

**Option B: ESP32**
- ESP32 development board
- Logic level converter (3.3V <-> 5V)
- Wi-Fi connectivity
- MicroPython or Arduino framework

### Cable

- DB-25 (X68000) to DB-9 or USB-Serial
- Null modem configuration if needed

## Security Considerations

1. **API Key Storage**: Never store API key on X68000; keep on bridge only
2. **Local Network**: Bridge should be on trusted local network
3. **Rate Limiting**: Bridge should implement rate limiting to prevent API abuse
4. **Input Validation**: Bridge should sanitize input before API calls

## Performance Considerations

1. **Baud Rate**: Start with 9600bps for reliability; increase if stable
2. **Buffer Size**: 4KB default; increase for longer responses
3. **Timeout**: 60 seconds allows for slow API responses
4. **Keepalive**: Prevents X68000 from timing out during long requests

## Testing Strategy

1. **Serial Loopback**: Test X68000 serial library with loopback cable
2. **Bridge Unit Tests**: Test bridge components independently
3. **Integration Test**: End-to-end test with mock API
4. **Live Test**: Full system test with real OpenAI API

## Future Extensions

### Graphical Frontend (chatgfx.x)

Design considerations for future implementation:

- **Display**: 256x256 pixel mode, 16 colors
- **Font**: 8x8 bitmap with shadow effect (1px offset black)
- **Layout**: 32 characters × ~28 lines usable
- **Input**: Direct keyboard via IOCS
- **Rendering**: PCG or direct GVRAM writes
- **Language**: ASCII only (no encoding conversion needed)
- **Library**: Links against same `libchat.a`

The graphical frontend will share the communication library but implement its own:
- Font rendering system
- Screen buffer management  
- Keyboard input handler
- UI layout and navigation
