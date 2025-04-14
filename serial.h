/*
 *  serial.h - Interface for serial port operations
 *  V1.0/8.11.2012/TCh
 *  V1.1/250109 Add baud rate selection
 *  V1.2/250313 Updated API with error codes
 */

#ifndef SERIAL_H
#define SERIAL_H

/* Error codes for serial port operations */
#define SERIAL_SUCCESS       0
#define SERIAL_ERROR_OPEN   -1
#define SERIAL_ERROR_FCNTL  -2
#define SERIAL_ERROR_ATTR   -3
#define SERIAL_ERROR_BAUD   -4
#define SERIAL_ERROR_CONFIG -5

/**
 * Opens a serial port with the specified device name.
 *
 * @param device Path to the serial device
 * @return File descriptor on success, negative error code on failure
 */
extern int open_port(const char *device);

/**
 * Configures the serial port with the specified baud rate and settings.
 * Uses blocking mode with 1.5 second timeout.
 *
 * @param fd File descriptor for the serial port
 * @param baud Baud rate (2400, 4800, 9600, 19200, 38400, 57600, 115200)
 * @return 0 on success, negative error code on failure
 */
extern int set_port(int fd, int baud);

#endif /* SERIAL_H */
