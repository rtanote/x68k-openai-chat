/*
 * serial.c - Low-level RS-232C serial communication for X68000
 *
 * Uses IOCS calls for serial port access.
 */

#include <iocslib.h>
#include "chat_internal.h"

/* SET232C mode bits */
/* Mode format: 0x4C_SBBB (4=stop bits, C=char len, S=stop bits, BBB=baud) */
/* We use 8N1 format: 8 data bits, no parity, 1 stop bit */
#define RS_MODE_8N1     0x4C00  /* 8 data bits, no parity, 1 stop bit */

/*
 * Convert baud rate to IOCS code
 */
static int baud_to_code(int baud_rate)
{
    switch (baud_rate) {
        case 300:   return 0;
        case 600:   return 1;
        case 1200:  return 2;
        case 2400:  return 3;
        case 4800:  return 4;
        case 9600:  return 5;
        case 19200: return 6;
        case 38400: return 8;
        default:    return 5;  /* Default to 9600 */
    }
}

/*
 * Initialize serial port
 *
 * If baud_rate is 0, use existing SWITCH.X settings.
 * Otherwise, call SET232C to configure the port.
 */
int serial_init(int baud_rate)
{
    int mode;
    int result;

    /* If baud_rate is 0, skip SET232C and use SWITCH.X settings */
    if (baud_rate == 0) {
        return CHAT_OK;
    }

    /* Build mode word: 8N1 + baud rate */
    mode = RS_MODE_8N1 | baud_to_code(baud_rate);

    /* Initialize RS-232C */
    result = SET232C(mode);
    if (result < 0) {
        return CHAT_ERR_INIT;
    }

    return CHAT_OK;
}

/*
 * Cleanup serial port
 */
void serial_cleanup(void)
{
    /* Nothing special needed - the port remains available */
}

/*
 * Send one byte
 */
int serial_putc(int c)
{
    /* Wait for transmit buffer to be ready */
    while (OSNS232C() == 0) {
        /* Busy wait */
    }

    /* Send the byte */
    OUT232C(c);
    return CHAT_OK;
}

/*
 * Receive one byte (blocking with timeout handled by caller)
 * Returns the byte value (0-255), or -1 if no data available
 */
int serial_getc(void)
{
    if (ISNS232C() == 0) {
        return -1;  /* No data available */
    }
    return INP232C() & 0xFF;
}

/*
 * Check if data is available
 */
int serial_available(void)
{
    return ISNS232C() != 0;
}
