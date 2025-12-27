/*
 * main.c - X68000 Chat CLI Tool
 *
 * Command-line interface for chatting with AI via the bridge.
 *
 * Usage:
 *   chat "message"      - Send a single message
 *   chat -i             - Interactive mode
 *   chat -h             - Show help
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iocslib.h>
#include <doslib.h>
#include "chat.h"

/* Buffer sizes */
#define INPUT_BUFSIZE   1024
#define RESPONSE_BUFSIZE 4096

/* Global response buffer */
static char response_buffer[RESPONSE_BUFSIZE];

/*
 * Print usage information
 */
static void print_usage(void)
{
    printf("X68000 OpenAI Chat Client\n");
    printf("\n");
    printf("Usage: chat [options] [message]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -i, --interactive  Enter interactive mode\n");
    printf("  -b, --baud <rate>  Set baud rate (9600/19200/38400)\n");
    printf("  -t, --timeout <s>  Set timeout in seconds (default: 60)\n");
    printf("  -h, --help         Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  chat \"What is X68000?\"\n");
    printf("  chat -i\n");
    printf("  chat -b 38400 \"Hello\"\n");
}

/*
 * Send a message and print the response
 */
static int send_and_print(const char *message)
{
    int result;
    size_t resp_len;

    result = chat_query(message, response_buffer, RESPONSE_BUFSIZE, &resp_len);
    if (result != CHAT_OK) {
        fprintf(stderr, "Error: %s\n", chat_error_string(result));
        return result;
    }

    printf("%s\n", response_buffer);
    return CHAT_OK;
}

/*
 * Interactive mode - prompt-response loop
 */
static int interactive_mode(void)
{
    char input[INPUT_BUFSIZE];
    int result;

    printf("X68000 Chat - Interactive Mode\n");
    printf("Type 'exit' or 'quit' to end.\n");
    printf("\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        /* Read input line */
        if (fgets(input, INPUT_BUFSIZE, stdin) == NULL) {
            break;
        }

        /* Remove trailing newline */
        {
            size_t len = strlen(input);
            if (len > 0 && input[len - 1] == '\n') {
                input[len - 1] = '\0';
                len--;
            }
            if (len > 0 && input[len - 1] == '\r') {
                input[len - 1] = '\0';
                len--;
            }
        }

        /* Skip empty lines */
        if (input[0] == '\0') {
            continue;
        }

        /* Check for exit commands */
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        /* Send and display response */
        printf("...\n");
        result = send_and_print(input);
        if (result != CHAT_OK) {
            /* Continue despite errors in interactive mode */
        }
        printf("\n");
    }

    return CHAT_OK;
}

/*
 * Parse command line arguments
 */
int main(int argc, char *argv[])
{
    int i;
    int interactive = 0;
    int baud_rate = CHAT_DEFAULT_BAUD;
    int timeout = CHAT_DEFAULT_TIMEOUT;
    const char *message = NULL;
    chat_config_t config;
    int result;

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive = 1;
        }
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--baud") == 0) {
            if (i + 1 < argc) {
                baud_rate = atoi(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
            if (i + 1 < argc) {
                timeout = atoi(argv[++i]);
            }
        }
        else if (argv[i][0] != '-') {
            message = argv[i];
        }
    }

    /* Check if we have something to do */
    if (!interactive && message == NULL) {
        print_usage();
        return 1;
    }

    /* Initialize chat library */
    config.baud_rate = baud_rate;
    config.timeout_sec = timeout;
    config.buffer_size = RESPONSE_BUFSIZE;

    result = chat_init(&config);
    if (result != CHAT_OK) {
        fprintf(stderr, "Failed to initialize: %s\n", chat_error_string(result));
        return 1;
    }

    /* Execute requested mode */
    if (interactive) {
        result = interactive_mode();
    } else {
        result = send_and_print(message);
    }

    /* Cleanup */
    chat_cleanup();

    return (result == CHAT_OK) ? 0 : 1;
}
