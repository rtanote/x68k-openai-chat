# X68000 OpenAI Chat System - Requirements Document

## Introduction

This specification defines a chat system that enables X68000 computers to communicate with the OpenAI API. The system consists of a communication library running on the X68000, a bridge device (Raspberry Pi or ESP32) that translates serial communication to HTTPS requests, and integration with μEmacs for a text-based chat interface.

The primary goal is to create a practical AI assistant accessible from vintage X68000 hardware, supporting both Japanese and English languages. A future extension will provide a game-style graphical interface (256x256, English-only) using the same communication core.

### Project Scope

- **In Scope**: Communication library, CLI tool, bridge software, μEmacs integration
- **Out of Scope**: Game-style graphical frontend implementation (design only for extensibility)

### Development Environment

- **Host**: Windows with xdev68k cross-compilation toolchain
- **Target**: X68000 running Human68k
- **Bridge**: Raspberry Pi or ESP32
- **Repository**: https://github.com/yosshin4004/xdev68k

---

## Requirements

### Requirement 1: Serial Communication Library

**User Story:** As a developer, I want a reusable serial communication library, so that I can build multiple applications (CLI, μEmacs integration, graphical frontend) using the same communication core.

#### Acceptance Criteria

1. WHEN the library is initialized THEN the system SHALL configure RS-232C with specified baud rate (default: 9600bps)
2. WHEN sending text data THEN the system SHALL transmit bytes through RS-232C using IOCS `_RS_PUTC` call
3. WHEN receiving data THEN the system SHALL read bytes from RS-232C using IOCS `_RS_GETC` call
4. WHEN a transmission completes THEN the system SHALL send an end-of-message delimiter (`\x04` ETX)
5. WHEN receiving a response THEN the system SHALL buffer incoming data until end-of-message delimiter is detected
6. WHEN a timeout occurs during receive THEN the system SHALL return an error code after configurable timeout period (default: 60 seconds)
7. IF the serial port is unavailable THEN the system SHALL return an appropriate error code without crashing

---

### Requirement 2: Communication Protocol

**User Story:** As a developer, I want a simple text-based protocol between X68000 and the bridge, so that debugging and extending the system is straightforward.

#### Acceptance Criteria

1. WHEN sending a chat message THEN the system SHALL encode the message as UTF-8 text followed by ETX (`\x04`)
2. WHEN receiving a response THEN the system SHALL decode UTF-8 text terminated by ETX
3. WHEN an error occurs on the bridge THEN the system SHALL receive an error message prefixed with `ERROR:`
4. WHEN the bridge is processing THEN it MAY send keepalive bytes (`\x00`) to prevent timeout
5. IF the message contains multi-byte characters (Japanese) THEN the system SHALL preserve UTF-8 encoding throughout transmission

---

### Requirement 3: Bridge Software

**User Story:** As a user, I want a bridge device that translates my X68000 serial communication to OpenAI API calls, so that I can chat with AI from my vintage computer.

#### Acceptance Criteria

1. WHEN the bridge receives text via serial THEN the system SHALL send it to OpenAI Chat Completions API
2. WHEN the OpenAI API responds THEN the system SHALL transmit the response text back via serial
3. WHEN the OpenAI API returns an error THEN the system SHALL send `ERROR: <message>` via serial
4. WHEN processing a request THEN the system SHALL send periodic keepalive bytes if response takes longer than 5 seconds
5. WHEN starting up THEN the bridge SHALL read API key from configuration file or environment variable
6. IF the API key is missing or invalid THEN the system SHALL log an error and send `ERROR: Invalid API key` via serial
7. WHEN configured THEN the bridge SHALL support customizable system prompts for different use cases

---

### Requirement 4: CLI Tool for X68000

**User Story:** As a user, I want a command-line tool to chat with AI from the Human68k command prompt, so that I can quickly ask questions without launching an editor.

#### Acceptance Criteria

1. WHEN executing `chat "message"` THEN the system SHALL send the message and display the response on stdout
2. WHEN executing `chat -i` THEN the system SHALL enter interactive mode with prompt-response loop
3. WHEN in interactive mode and user types `exit` or `quit` THEN the system SHALL terminate gracefully
4. WHEN a communication error occurs THEN the system SHALL display an error message on stderr and exit with non-zero code
5. WHEN the `-h` or `--help` flag is provided THEN the system SHALL display usage information
6. IF no message is provided and not in interactive mode THEN the system SHALL display usage information and exit

---

### Requirement 5: μEmacs Integration

**User Story:** As a μEmacs user, I want to send text from my editor buffer to the AI and receive responses inline, so that I can have a seamless chat experience within my editing workflow.

#### Acceptance Criteria

1. WHEN user invokes send command (e.g., `Ctrl+C Ctrl+S`) THEN the system SHALL send the current paragraph or marked region to the AI
2. WHEN a response is received THEN the system SHALL insert a separator line followed by the response text into the buffer
3. WHEN waiting for response THEN the system SHALL display a status indicator (e.g., `...` or `Thinking...`)
4. WHEN response is complete THEN the system SHALL emit an audible bell or visual notification
5. WHEN an error occurs THEN the system SHALL insert the error message into the buffer with `[ERROR]` prefix
6. IF the external chat tool is not found THEN the system SHALL display an appropriate error message

---

### Requirement 6: Character Encoding Support

**User Story:** As a Japanese user, I want to communicate in Japanese through μEmacs, so that I can use my native language with the AI assistant.

#### Acceptance Criteria

1. WHEN sending Japanese text from X68000 THEN the system SHALL convert Shift_JIS to UTF-8 before transmission
2. WHEN receiving Japanese text THEN the system SHALL convert UTF-8 to Shift_JIS for display
3. WHEN encoding conversion fails THEN the system SHALL replace unconvertible characters with `?` and continue
4. IF the graphical frontend is used THEN the system SHALL only support ASCII characters (English-only mode)

---

### Requirement 7: Extensibility for Graphical Frontend

**User Story:** As a developer, I want the communication library to be independent of display logic, so that I can create alternative frontends (like a game-style UI) without modifying the core.

#### Acceptance Criteria

1. WHEN building applications THEN the communication library SHALL be linkable as a static library (`libchat.a`)
2. WHEN designing the library API THEN it SHALL not depend on any display or input method
3. WHEN the graphical frontend is implemented THEN it SHALL use the same `libchat.a` for communication
4. IF additional protocol features are needed THEN the library SHALL provide extension points without breaking existing clients

---

## Future Considerations (Out of Scope)

### Graphical Frontend Concept

A future game-style graphical application will provide:
- 256x256 pixel display mode
- English-only interface (ASCII)
- Shadow-effect bitmap font rendering
- PCG or GVRAM-based text display
- Game-like UI with status bar and key hints
- Same communication library (`libchat.a`)

This frontend is documented here for architectural planning but will not be implemented in the initial release.
