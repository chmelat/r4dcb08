/*
 * Device settings modification functions
 * V1.0/2025-04-17
 */
#ifndef WRITE_FUNCTIONS_H
#define WRITE_FUNCTIONS_H

#include <stdint.h>
#include "error.h"

/**
 * Write a new device address
 * 
 * @param fd File descriptor for the serial port
 * @param adr Current device address
 * @param n_adr New device address to be set
 * 
 * @return STATUS_OK on success, otherwise an error code from AppStatus enum
 */
AppStatus write_address(int fd, uint8_t adr, uint8_t n_adr);

/**
 * Write a new baudrate to the device
 * 
 * @param fd File descriptor for the serial port
 * @param adr Device address
 * @param cbr Baudrate code (0:1200, 1:2400, 2:4800, 3:9600, 4:19200)
 * 
 * @return STATUS_OK on success, otherwise an error code from AppStatus enum
 */
AppStatus write_baudrate(int fd, uint8_t adr, uint8_t cbr);

/**
 * Write temperature correction value for a channel
 * 
 * @param fd File descriptor for the serial port
 * @param adr Device address
 * @param ch Channel number (1-8)
 * @param T_c Correction temperature value
 * 
 * @return STATUS_OK on success, otherwise an error code from AppStatus enum
 */
AppStatus write_correction(int fd, uint8_t adr, uint8_t ch, float T_c);

/**
 * Get baudrate value from code
 * 
 * @param code Baudrate code (0:1200, 1:2400, 2:4800, 3:9600, 4:19200)
 * @param baudrate Pointer to store the baudrate value
 * 
 * @return STATUS_OK on success, ERROR_INVALID_BAUDRATE on error
 */
AppStatus get_baudrate_value(uint8_t code, int *baudrate);

#endif /* WRITE_FUNCTIONS_H */