/*
 * chat.c - Main chat library implementation
 *
 * Provides the public API for serial communication with the bridge.
 */

#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include "chat_internal.h"

/* Global state */
chat_state_t g_chat_state = {0};

/* Error message strings */
static const char *error_strings[] = {
    "OK",                           /* CHAT_OK */
    "Initialization failed",        /* CHAT_ERR_INIT */
    "Timeout waiting for response", /* CHAT_ERR_TIMEOUT */
    "Send failed",                  /* CHAT_ERR_SEND */
    "Receive failed",               /* CHAT_ERR_RECV */
    "Protocol error",               /* CHAT_ERR_PROTOCOL */
    "Buffer overflow"               /* CHAT_ERR_BUFFER */
};

/*
 * Initialize the chat library
 */
int chat_init(const chat_config_t *config)
{
    int result;
    int baud_rate;
    int timeout_sec;
    size_t buffer_size;

    /* Use defaults if no config provided */
    if (config != NULL) {
        baud_rate = config->baud_rate;
        timeout_sec = config->timeout_sec;
        buffer_size = config->buffer_size;
    } else {
        baud_rate = CHAT_DEFAULT_BAUD;
        timeout_sec = CHAT_DEFAULT_TIMEOUT;
        buffer_size = CHAT_DEFAULT_BUFSIZE;
    }

    /* Validate baud rate: 0 means use SWITCH.X settings */
    if (baud_rate != 0 && baud_rate != 4800 && baud_rate != 9600 && baud_rate != 19200 && baud_rate != 38400) {
        baud_rate = CHAT_DEFAULT_BAUD;
    }
    if (timeout_sec <= 0) {
        timeout_sec = CHAT_DEFAULT_TIMEOUT;
    }
    if (buffer_size < 256) {
        buffer_size = CHAT_DEFAULT_BUFSIZE;
    }

    /* Allocate receive buffer */
    g_chat_state.recv_buffer = (char *)malloc(buffer_size);
    if (g_chat_state.recv_buffer == NULL) {
        return CHAT_ERR_INIT;
    }

    /* Initialize serial port */
    result = serial_init(baud_rate);
    if (result != CHAT_OK) {
        free(g_chat_state.recv_buffer);
        g_chat_state.recv_buffer = NULL;
        return result;
    }

    /* Save state */
    g_chat_state.initialized = 1;
    g_chat_state.baud_rate = baud_rate;
    g_chat_state.timeout_sec = timeout_sec;
    g_chat_state.buffer_size = buffer_size;
    g_chat_state.buffer_pos = 0;

    return CHAT_OK;
}

/*
 * Cleanup the chat library
 */
void chat_cleanup(void)
{
    if (!g_chat_state.initialized) {
        return;
    }

    serial_cleanup();

    if (g_chat_state.recv_buffer != NULL) {
        free(g_chat_state.recv_buffer);
        g_chat_state.recv_buffer = NULL;
    }

    g_chat_state.initialized = 0;
}

/*
 * Send a message
 */
int chat_send(const char *message, size_t len)
{
    if (!g_chat_state.initialized) {
        return CHAT_ERR_INIT;
    }

    if (message == NULL || len == 0) {
        return CHAT_ERR_SEND;
    }

    return proto_send_message(message, len);
}

/*
 * Receive a response
 */
int chat_recv(char *buffer, size_t bufsize, size_t *received)
{
    if (!g_chat_state.initialized) {
        return CHAT_ERR_INIT;
    }

    if (buffer == NULL || bufsize == 0) {
        return CHAT_ERR_RECV;
    }

    return proto_recv_message(buffer, bufsize, received, g_chat_state.timeout_sec);
}

/*
 * Send message and receive response in one operation
 */
int chat_query(const char *message, char *response, size_t resp_size, size_t *resp_len)
{
    int result;
    size_t msg_len;

    if (!g_chat_state.initialized) {
        return CHAT_ERR_INIT;
    }

    /* Send the message */
    msg_len = strlen(message);
    result = chat_send(message, msg_len);
    if (result != CHAT_OK) {
        return result;
    }

    /* Receive the response */
    result = chat_recv(response, resp_size, resp_len);
    return result;
}

/*
 * Check if data is available
 */
int chat_available(void)
{
    if (!g_chat_state.initialized) {
        return 0;
    }

    return serial_available();
}

/*
 * Get error string
 */
const char *chat_error_string(int error_code)
{
    int index = -error_code;

    if (index >= 0 && index <= 6) {
        return error_strings[index];
    }

    return "Unknown error";
}
