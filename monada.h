/*
 *  Wrapper function for device communication
 *  V1.4/2025-04-16
 */
#ifndef MONADA_H
#define MONADA_H

#include <stdint.h>  /* For uint8_t */
//#include "typedef.h" /* For PACKET definition */
#include "packet.h"  /* For ReceiveMode enum */

/**
 * Status codes for monada function
 */
typedef enum {
    MONADA_OK = 0,               /* Operation successful */
    MONADA_ERROR_INVALID_PARAM = -1,  /* Invalid parameter */
    MONADA_ERROR_DATA_TOO_LONG = -2,  /* Input data too long */
    MONADA_ERROR_SEND_FAILED = -3,    /* Failed to send packet */
    MONADA_ERROR_RECEIVE_FAILED = -4  /* Failed to receive packet */
} MonadaStatus;

/**
 * Send an instruction to the device and receive the response
 *
 * This function is a wrapper that handles the entire communication sequence:
 * forming a packet, sending it, receiving the response, and extracting the data.
 *
 * @param fd      File descriptor of the serial port
 * @param adr     Device address
 * @param inst    Instruction code
 * @param in_len  Length of input data
 * @param arg     Pointer to input data buffer (can be NULL for zero-length data)
 * @param p_r     Pointer to PACKET structure to store the received packet
 * @param verb    Verbosity flag: 1 = print "OK" message, 0 = silent
 * @param msg     Operation name for debugging messages
 * @param mode    Receive mode (RECEIVE_MODE_TEMPERATURE or RECEIVE_MODE_ACKNOWLEDGE)
 *
 * @return        Pointer to received data on success, NULL on error. 
 *                Check return value with monada_status() for error details.
 */
extern uint8_t *monada(int fd, uint8_t adr, uint8_t inst, int in_len, 
                      uint8_t *arg, PACKET *p_r, int verb, 
                      const char *msg, int mode);

/**
 * Get the status of the last monada operation
 *
 * @return        Status code from the MonadaStatus enum
 */
extern MonadaStatus monada_status(void);

#endif /* MONADA_H */
