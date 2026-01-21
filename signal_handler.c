/*
 * Signal handling utilities
 * V1.0/2025-04-17
 * V1.1/2025-01-21 Use sigaction() instead of signal(), remove printf from handler
 */
#include <string.h>
#include "signal_handler.h"

/*
 * Global flag for running state
 * Initialized to 1 (running)
 */
volatile sig_atomic_t running = 1;

/* Store received signal number for later reporting */
static volatile sig_atomic_t received_signal = 0;

/*
 * Signal handler for clean termination
 * Note: Only async-signal-safe operations are allowed here
 */
void handle_signal(int sig)
{
    received_signal = sig;
    running = 0;
}

/*
 * Get the signal that caused termination (for reporting after handler returns)
 */
int get_received_signal(void)
{
    return received_signal;
}

/*
 * Initialize signal handlers using sigaction()
 * More reliable and portable than signal()
 */
void init_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /* Set up handler for SIGINT (Ctrl+C) */
    sigaction(SIGINT, &sa, NULL);

    /* Set up handler for SIGTERM */
    sigaction(SIGTERM, &sa, NULL);
}
