/*
 * chat_internal.h - Internal structures and constants
 *
 * Not part of public API - for library internal use only.
 */

#ifndef CHAT_INTERNAL_H
#define CHAT_INTERNAL_H

#include "chat.h"

/* Protocol constants */
#define PROTO_ETX           0x04    /* End of transmission */
#define PROTO_KEEPALIVE     0x00    /* Keepalive byte (NUL) */
#define PROTO_ERROR_PREFIX  "ERROR:"

/* RS-232C baud rate codes for IOCS */
#define RS_BAUD_9600        7
#define RS_BAUD_19200       8
#define RS_BAUD_38400       9

/* Serial port state */
typedef struct {
    int initialized;
    int baud_rate;
    int timeout_sec;
    char *recv_buffer;
    size_t buffer_size;
    size_t buffer_pos;
} chat_state_t;

/* Global state (defined in chat.c) */
extern chat_state_t g_chat_state;

/* Internal serial functions (serial.c) */
int serial_init(int baud_rate);
void serial_cleanup(void);
int serial_putc(int c);
int serial_getc(void);
int serial_available(void);

/* Internal protocol functions (protocol.c) */
int proto_send_message(const char *msg, size_t len);
int proto_recv_message(char *buffer, size_t bufsize, size_t *received, int timeout_sec);

#endif /* CHAT_INTERNAL_H */
