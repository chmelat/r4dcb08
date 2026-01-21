/*
 * Signal handling utilities
 * V1.0/2025-04-17
 */
#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

/*
 * Flag indicating whether the program should continue running
 * Volatile is required as it's modified in the signal handler
 */
extern volatile sig_atomic_t running;

/*
 * Signal handler for clean termination
 * Sets the running flag to 0 to allow graceful shutdown
 */
void handle_signal(int sig);

/*
 * Initialize signal handlers
 * Sets up handlers for SIGINT and SIGTERM using sigaction()
 */
void init_signal_handlers(void);

/*
 * Get the signal number that caused termination
 * Returns 0 if no signal was received
 */
int get_received_signal(void);

#endif /* SIGNAL_HANDLER_H */