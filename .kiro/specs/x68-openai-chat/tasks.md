# X68000 OpenAI Chat System - Implementation Plan

## Overview

This implementation plan covers the development of the X68000 OpenAI Chat System from initial setup through μEmacs integration. Tasks are organized in phases with dependencies clearly marked.

---

## Phase 1: Project Setup and Infrastructure

- [ ] 1. Initialize project repository and directory structure
  - Create project at `D:\dev\x68k-openai-chat\` (parallel to xdev68k)
  - Directory structure:
    ```
    D:\dev\
    ├── xdev68k\           # Existing toolchain
    └── x68k-openai-chat\  # This project
        ├── .kiro\specs\
        ├── src\libchat\
        ├── src\cli\
        ├── bridge\
        └── Makefile
    ```
  - Initialize git repository
  - Create README.md with project overview
  - Add .gitignore for build artifacts (*.o, *.a, *.x, *.elf, build/)
  - _Requirements: 1.1, 7.1_

- [ ] 2. Configure xdev68k build environment
  - Verify xdev68k installation at `D:\dev\xdev68k`
  - Create base Makefile with relative path to toolchain:
    ```makefile
    XDEV68K_DIR = ../xdev68k
    CC = $(XDEV68K_DIR)/bin/m68k-elf-gcc
    ```
  - Configure MSYS2 to access project via `/d/dev/x68k-openai-chat`
  - Add build targets for library and CLI
  - Test compilation with minimal "Hello World" program
  - Verify elf2x68k conversion produces valid .x executable
  - _Requirements: 1.1_

- [ ] 3. Set up emulator environment with virtual serial port
  - Install com0com (Null-modem emulator) on Windows
    - Download from https://sourceforge.net/projects/com0com/
    - Create virtual COM port pair: COM10 ⇔ COM11
  - Configure XM6 TypeG emulator
    - Set RS-232C port to COM10
    - Verify serial port settings (9600bps, 8N1)
  - Test virtual serial connection
    - Use terminal software (TeraTerm) on COM11 to verify communication
    - Send test data from XM6 and confirm receipt
  - Document emulator serial setup for other developers
  - _Requirements: 1.1, 1.2, 1.3_

- [ ] 4. Set up bridge development environment
  - Create Python virtual environment (Windows native, not WSL)
    ```cmd
    cd D:\dev\x68k-openai-chat\bridge
    python -m venv venv
    venv\Scripts\activate
    ```
  - Create requirements.txt:
    ```
    pyserial>=3.5
    openai>=1.0.0
    pyyaml>=6.0
    ```
  - Create config.yaml template with COM11 for emulator testing:
    ```yaml
    serial:
      port: COM11        # Virtual COM port (com0com pair)
      baud_rate: 9600
    openai:
      api_key: ${OPENAI_API_KEY}
      model: gpt-4o-mini
    ```
  - Document bridge setup instructions
  - _Requirements: 3.5_

---

## Phase 2: Communication Library Core (libchat.a)

- [ ] 5. Implement serial port initialization
  - Create chat.h with public API definitions
  - Create chat_internal.h with internal structures
  - Implement `chat_init()` with IOCS `_RS_INIT` call
  - Implement `chat_cleanup()` to release serial port
  - Add error handling for port unavailable
  - Test: Serial port opens/closes without error
  - _Requirements: 1.1, 1.7_

- [ ] 6. Implement serial send functionality
  - Create serial.c with low-level IOCS wrappers
  - Implement byte-by-byte transmission via `_RS_PUTC`
  - Add ETX delimiter appending
  - Test: Send "Hello" + ETX via loopback
  - _Requirements: 1.2, 1.4_

- [ ] 7. Implement serial receive functionality
  - Implement receive buffer management
  - Implement byte-by-byte reception via `_RS_GETC`
  - Add ETX detection for message termination
  - Implement timeout handling with configurable duration
  - Handle keepalive bytes (NUL) during receive
  - Test: Receive response terminated by ETX
  - _Requirements: 1.3, 1.5, 1.6_

- [ ] 8. Implement protocol encoding/decoding
  - Create protocol.c for message framing
  - Implement `chat_send()` function
  - Implement `chat_recv()` function
  - Implement `chat_query()` combined operation
  - Add error prefix detection (`ERROR:`)
  - Test: Round-trip message with bridge simulator
  - _Requirements: 2.1, 2.2, 2.3, 2.4_

- [ ] 9. Build static library
  - Update Makefile to build libchat.a
  - Verify all object files link correctly
  - Test: Library links with minimal test program
  - _Requirements: 7.1, 7.2_

---

## Phase 3: Bridge Software

- [ ] 10. Implement bridge serial handler
  - Create bridge.py main module
  - Implement serial port initialization
  - Implement message receive (read until ETX)
  - Implement message send (append ETX)
  - Test: Echo received messages back
  - _Requirements: 3.1_

- [ ] 11. Implement OpenAI API client
  - Implement OpenAI client initialization
  - Implement chat completions API call
  - Add system prompt configuration
  - Add error handling for API errors
  - Test: Successful API call with test message
  - _Requirements: 3.1, 3.2, 3.3, 3.7_

- [ ] 12. Implement keepalive mechanism
  - Create background thread for keepalive
  - Send NUL byte every 5 seconds during API call
  - Stop keepalive when response received
  - Test: Long API response doesn't timeout X68000
  - _Requirements: 3.4_

- [ ] 13. Implement configuration loading
  - Parse config.yaml for settings
  - Support environment variable substitution for API key
  - Validate required configuration fields
  - Add helpful error messages for missing config
  - Test: Bridge starts with valid config, fails gracefully with invalid
  - _Requirements: 3.5, 3.6_

- [ ] 14. Implement character encoding conversion
  - Add Shift_JIS to UTF-8 conversion for received messages
  - Add UTF-8 to Shift_JIS conversion for responses
  - Handle conversion errors with replacement character
  - Test: Japanese text round-trip
  - _Requirements: 6.1, 6.2, 6.3_

- [ ] 15. Integration test: Library + Bridge
  - Connect X68000 (or emulator) to bridge
  - Send test message from X68000
  - Verify response received correctly
  - Test Japanese text if applicable
  - Document any issues and fixes
  - _Requirements: 1.1-1.7, 2.1-2.5, 3.1-3.7_

---

## Phase 4: CLI Tool (chat.x)

- [ ] 16. Implement CLI argument parsing
  - Create main.c for CLI tool
  - Parse command line arguments (-i, -b, -t, -h)
  - Extract message from arguments
  - Display usage on -h or invalid args
  - _Requirements: 4.5, 4.6_

- [ ] 17. Implement single-query mode
  - Initialize chat library
  - Send message from command line
  - Display response on stdout
  - Handle errors with stderr output and exit code
  - Test: `chat "Hello"` returns response
  - _Requirements: 4.1, 4.4_

- [ ] 18. Implement interactive mode
  - Add prompt display loop
  - Read user input line by line
  - Send each line as query
  - Display responses
  - Handle "exit" and "quit" commands
  - Test: Interactive session works correctly
  - _Requirements: 4.2, 4.3_

- [ ] 19. Build and test CLI tool
  - Update Makefile for chat.x target
  - Compile and link with libchat.a
  - Convert ELF to X68000 executable
  - Test on real X68000 or emulator
  - _Requirements: 4.1-4.6_

---

## Phase 5: μEmacs Integration

- [ ] 20. Research μEmacs capabilities on X68000
  - Investigate macro/extension capabilities
  - Determine shell command execution support
  - Identify file I/O functions available
  - Document findings and limitations
  - _Requirements: 5.1_

- [ ] 21. Implement file-based integration approach
  - Modify chat.x to support stdin/stdout for piping
  - Test: `echo "Hello" | chat` works correctly
  - Test: `chat < input.txt > output.txt` works correctly
  - _Requirements: 5.1, 5.2_

- [ ] 22. Create μEmacs macro for chat
  - Write macro to save region to temp file
  - Add shell command invocation
  - Implement response file reading
  - Add separator line insertion
  - Add bell/notification on completion
  - Add error handling for missing chat tool
  - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_

- [ ] 23. Test and refine μEmacs integration
  - Test with various text selections
  - Test with Japanese text
  - Verify status indicator displays
  - Verify notification works
  - Document usage instructions
  - _Requirements: 5.1-5.6, 6.1-6.3_

---

## Phase 6: Documentation and Polish

- [ ] 24. Write user documentation
  - Create installation guide
  - Document CLI usage with examples
  - Document μEmacs integration setup
  - Add troubleshooting section
  - Create quick start guide
  - _Requirements: All_

- [ ] 25. Write developer documentation
  - Document library API with examples
  - Document protocol specification
  - Document build process
  - Add contribution guidelines
  - _Requirements: 7.1-7.4_

- [ ] 26. Final testing and release preparation
  - Perform end-to-end testing
  - Test on real X68000 hardware
  - Fix any remaining issues
  - Tag release version
  - Create release notes
  - _Requirements: All_

---

## Task Dependencies

```
Phase 1: [1] → [2] → [3] → [4]
                      ↓
Phase 2: [5] → [6] → [7] → [8] → [9]
                                  ↓
Phase 3: [10] → [11] → [12]       ↓
           ↓                      ↓
         [13] → [14] → [15] ←─────┘
                        ↓
Phase 4: [16] → [17] → [18] → [19]
                               ↓
Phase 5: [20] → [21] → [22] → [23]
                               ↓
Phase 6: [24] → [25] → [26]
```

---

## Milestone Checkpoints

### Milestone 0: "Dev Environment Ready" (Tasks 1-4)
- Project structure created
- xdev68k build working
- Virtual COM ports configured (com0com)
- XM6 TypeG serial communication verified
- Bridge Python environment ready

### Milestone 1: "Serial Echo" (Tasks 5-9)
- X68000 can send/receive data via serial
- libchat.a builds successfully

### Milestone 2: "Bridge Online" (Tasks 10-15)
- Bridge connects X68000 to OpenAI
- Full round-trip communication working

### Milestone 3: "CLI Complete" (Tasks 16-19)
- chat.x works from Human68k command line
- Both single-query and interactive modes functional

### Milestone 4: "Editor Integration" (Tasks 20-23)
- μEmacs integration complete
- Japanese text support working

### Milestone 5: "Release Ready" (Tasks 24-26)
- Documentation complete
- Tested on real hardware
- Ready for public release

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| com0com installation issues | Use alternative: TCP-COM bridge or hardware loopback for testing |
| XM6 TypeG serial incompatibility | Test with XM6 Pro-68k as fallback; document working emulator versions |
| μEmacs macro limitations | Fall back to file-based workflow with manual invocation |
| Serial timing issues | Implement robust timeout and retry logic |
| Japanese encoding edge cases | Use replacement characters; document limitations |
| xdev68k compatibility | Test incrementally; maintain compatibility notes |
| Bridge reliability | Add watchdog and auto-restart capability |

---

## Notes

- All X68000 code uses C89 for xdev68k compatibility
- Bridge uses Python 3.8+ for modern async support
- Task estimates assume familiarity with X68000 development
- Hardware testing requires real X68000 or accurate emulator (XM6 TypeG recommended)
