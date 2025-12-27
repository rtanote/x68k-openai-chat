/*
 * protocol.c - Protocol encoding/decoding for chat messages
 *
 * Handles message framing with ETX delimiter and keepalive handling.
 */

#include <string.h>
#include <iocslib.h>
#include "chat_internal.h"

/*
 * Send a message with ETX terminator
 */
int proto_send_message(const char *msg, size_t len)
{
    size_t i;
    int result;

    /* Send message bytes */
    for (i = 0; i < len; i++) {
        result = serial_putc((unsigned char)msg[i]);
        if (result != CHAT_OK) {
            return CHAT_ERR_SEND;
        }
    }

    /* Send ETX terminator */
    result = serial_putc(PROTO_ETX);
    if (result != CHAT_OK) {
        return CHAT_ERR_SEND;
    }

    return CHAT_OK;
}

/*
 * Get current time in seconds (from IOCS ONTIME)
 * ONTIME returns 1/100 seconds since midnight
 */
static unsigned long get_time_sec(void)
{
    return (unsigned long)ONTIME() / 100;
}

/*
 * Receive a message until ETX terminator
 * Handles keepalive bytes (NUL) by ignoring them
 */
int proto_recv_message(char *buffer, size_t bufsize, size_t *received, int timeout_sec)
{
    size_t pos = 0;
    unsigned long start_time;
    unsigned long current_time;
    int c;

    start_time = get_time_sec();

    while (pos < bufsize - 1) {  /* Leave room for null terminator */
        /* Check timeout */
        current_time = get_time_sec();

        /* Handle midnight rollover (86400 seconds in a day) */
        if (current_time < start_time) {
            current_time += 8640000 / 100;  /* Add a day's worth */
        }

        if ((current_time - start_time) >= (unsigned long)timeout_sec) {
            return CHAT_ERR_TIMEOUT;
        }

        /* Try to read a byte */
        c = serial_getc();
        if (c < 0) {
            /* No data available, continue waiting */
            continue;
        }

        /* Handle keepalive bytes (NUL) */
        if (c == PROTO_KEEPALIVE) {
            /* Reset timeout on keepalive */
            start_time = get_time_sec();
            continue;
        }

        /* Check for end of message */
        if (c == PROTO_ETX) {
            break;
        }

        /* Store the byte */
        buffer[pos++] = (char)c;
    }

    /* Null-terminate the buffer */
    buffer[pos] = '\0';
    if (received != NULL) {
        *received = pos;
    }

    /* Check if response is an error message */
    if (pos >= 6 && strncmp(buffer, PROTO_ERROR_PREFIX, 6) == 0) {
        return CHAT_ERR_PROTOCOL;
    }

    return CHAT_OK;
}
