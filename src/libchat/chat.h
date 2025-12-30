/*
 * chat.h - X68000 OpenAI Chat Communication Library
 *
 * Public API for serial communication with the bridge device.
 */

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

/* Configuration structure */
typedef struct {
    int baud_rate;          /* 0=use SWITCH.X, or 4800/9600/19200/38400 */
    int timeout_sec;        /* Receive timeout in seconds */
    size_t buffer_size;     /* Internal buffer size */
} chat_config_t;

/* Default configuration values */
#define CHAT_DEFAULT_BAUD       9600
#define CHAT_DEFAULT_TIMEOUT    60
#define CHAT_DEFAULT_BUFSIZE    4096

/*
 * Initialize communication library.
 * If config is NULL, default values are used.
 * Returns CHAT_OK on success, error code on failure.
 */
int chat_init(const chat_config_t *config);

/*
 * Cleanup and close serial port.
 */
void chat_cleanup(void);

/*
 * Send a message to the bridge.
 * message: null-terminated string to send
 * len: length of message (excluding null terminator)
 * Returns CHAT_OK on success, error code on failure.
 */
int chat_send(const char *message, size_t len);

/*
 * Receive a response from the bridge.
 * buffer: buffer to store received data
 * bufsize: size of buffer
 * received: pointer to store actual received length
 * Returns CHAT_OK on success, error code on failure.
 */
int chat_recv(char *buffer, size_t bufsize, size_t *received);

/*
 * Send a message and receive response (combined operation).
 * message: null-terminated string to send
 * response: buffer to store response
 * resp_size: size of response buffer
 * resp_len: pointer to store actual response length (can be NULL)
 * Returns CHAT_OK on success, error code on failure.
 */
int chat_query(const char *message, char *response, size_t resp_size, size_t *resp_len);

/*
 * Check if data is available to read.
 * Returns 1 if data available, 0 if not.
 */
int chat_available(void);

/*
 * Get human-readable error message for error code.
 */
const char *chat_error_string(int error_code);

#endif /* CHAT_H */
