/*
 * RS485 bus scan functions
 * V1.0/2025-01-23
 */
#ifndef SCAN_H
#define SCAN_H

#include <stdint.h>
#include "error.h"

/* Scan timeout in milliseconds */
#define SCAN_TIMEOUT_MS 100

/**
 * Scan RS485 bus for Modbus RTU devices
 *
 * @param fd         File descriptor of the serial port
 * @param start_addr Starting address for scan (typically 1)
 * @param end_addr   Ending address for scan (typically 254)
 * @return           STATUS_OK on success (regardless of devices found)
 */
AppStatus scan_bus(int fd, uint8_t start_addr, uint8_t end_addr);

#endif /* SCAN_H */
