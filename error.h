/* error.h */
#ifndef ERROR_H
#define ERROR_H

/**
 * Status codes for the entire application
 */
typedef enum {
    /* General codes */
    STATUS_OK = 0,               /* Operation successful */
    
    /* Command line argument errors */
    ERROR_INVALID_PORT = -10,    /* Invalid port */
    ERROR_INVALID_ADDRESS = -11, /* Invalid device address */
    ERROR_INVALID_BAUDRATE = -12,/* Invalid baudrate */
    ERROR_INVALID_CHANNEL = -13, /* Invalid channel number */
    ERROR_INVALID_TIME = -14,    /* Invalid time step */
    ERROR_TOO_MANY_ARGS = -15,   /* Too many arguments */
    
    /* Communication errors */
    ERROR_PORT_INIT = -20,       /* Port initialization failure */
    ERROR_SEND_PACKET = -21,     /* Failed to send packet */
    ERROR_RECEIVE_PACKET = -22,  /* Failed to receive packet */
    
    /* Operation errors */
    ERROR_WRITE_ADDRESS = -30,   /* Failed to write address */
    ERROR_WRITE_BAUDRATE = -31,  /* Failed to write baudrate */
    ERROR_WRITE_CORRECTION = -32,/* Failed to write correction */
    ERROR_READ_TEMPERATURE = -33,/* Failed to read temperature */
    ERROR_READ_CORRECTION = -34, /* Failed to read correction */
    ERROR_MEDIAN_FILTER = -35    /* Median filter failure */
} AppStatus;

/**
 * Returns a text description of the error state
 * @param status Status code
 * @return Text description
 */
const char* get_error_message(AppStatus status);

/**
 * Processes an error state
 * @param status Status code
 * @param function_name Name of the function where the error occurred
 * @param exit_on_error 1 if the program should exit on error, 0 otherwise
 * @return status (for chaining calls)
 */
AppStatus handle_error(AppStatus status, const char* function_name, int exit_on_error);

#endif /* ERROR_H */