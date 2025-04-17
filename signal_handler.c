/*
 * Signal handling utilities
 * V1.0/2025-04-17
 */
#include <stdio.h>
#include "signal_handler.h"

/* 
 * Global flag for running state
 * Initialized to 1 (running)
 */
volatile sig_atomic_t running = 1;

/*
 * Signal handler for clean termination
 */
void handle_signal(int sig)
{
    /* Set running flag to 0 to stop any running loops */
    running = 0;
    
    /* Optional: Print a message indicating which signal was received */
    if (sig == SIGINT) {
        printf("\nReceived SIGINT (Ctrl+C), shutting down...\n");
    } else if (sig == SIGTERM) {
        printf("\nReceived SIGTERM, shutting down...\n");
    }
}

/*
 * Initialize signal handlers
 */
void init_signal_handlers(void)
{
    /* Set up handler for SIGINT (Ctrl+C) */
    signal(SIGINT, handle_signal);
    
    /* Set up handler for SIGTERM */
    signal(SIGTERM, handle_signal);
}
