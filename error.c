/* error.c */
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

const char* get_error_message(AppStatus status) {
    switch (status) {
        case STATUS_OK:
            return "Operation successful";
        case ERROR_INVALID_PORT:
            return "Invalid port";
        case ERROR_INVALID_ADDRESS:
            return "Invalid device address";
        case ERROR_INVALID_BAUDRATE:
            return "Invalid baudrate";
        case ERROR_INVALID_CHANNEL:
            return "Invalid channel number";
        case ERROR_INVALID_TIME:
            return "Invalid time step";
        case ERROR_TOO_MANY_ARGS:
            return "Too many arguments";
        case ERROR_PORT_INIT:
            return "Port initialization failure";
        case ERROR_SEND_PACKET:
            return "Failed to send packet";
        case ERROR_RECEIVE_PACKET:
            return "Failed to receive packet";
        case ERROR_WRITE_ADDRESS:
            return "Failed to write address";
        case ERROR_WRITE_BAUDRATE:
            return "Failed to write baudrate";
        case ERROR_WRITE_CORRECTION:
            return "Failed to write correction";
        case ERROR_READ_TEMPERATURE:
            return "Failed to read temperature";
        case ERROR_READ_CORRECTION:
            return "Failed to read correction";
        case ERROR_MEDIAN_FILTER:
            return "Median filter failure";
        default:
            return "Unknown error";
    }
}

AppStatus handle_error(AppStatus status, const char* function_name, int exit_on_error) {
    if (status != STATUS_OK) {
        fprintf(stderr, "Error in %s: %s (code %d)\n", 
                function_name, get_error_message(status), status);
        
        if (exit_on_error) {
            exit(EXIT_FAILURE);
        }
    }
    return status;
}