/*
 * Temperature reading functions
 * V1.0/2025-04-17
 */
#ifndef READ_FUNCTIONS_H
#define READ_FUNCTIONS_H

#include <stdint.h>
#include "error.h"

/**
 * Read and print temperature from 1..n channels
 * 
 * @param fd File descriptor for the serial port
 * @param adr Device address
 * @param n Number of channels to read (1-8)
 * @param dt Time step between measurements in seconds
 * @param m_f Flag to enable (1) or disable (0) three-point median filter
 * 
 * @return STATUS_OK on success, otherwise an error code from AppStatus enum
 */
AppStatus read_temp(int fd, uint8_t adr, int n, int dt, int m_f);

/**
 * Read and print correction temperature for all channels
 * 
 * @param fd File descriptor for the serial port
 * @param adr Device address
 * 
 * @return STATUS_OK on success, otherwise an error code from AppStatus enum
 */
AppStatus read_correction(int fd, uint8_t adr);

#endif /* READ_FUNCTIONS_H */