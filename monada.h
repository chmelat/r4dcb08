/*
 *  Wrapper function for device communication
 *  V1.4/2025-04-16
 */
#ifndef MONADA_H
#define MONADA_H

#include <stdint.h>  /* For uint8_t */
//#include "typedef.h" /* For PACKET definition */
#include "packet.h"  /* For ReceiveMode enum */
#include "error.h"   /* For AppStatus */


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
 * @param data_out Pointer to store received data pointer (optional, can be NULL)
 *
 * @return        STATUS_OK on success, AppStatus error code on failure
 */
extern AppStatus monada(int fd, uint8_t adr, uint8_t inst, int in_len, 
                       uint8_t *arg, PACKET *p_r, int verb, 
                       const char *msg, int mode, uint8_t **data_out);


#endif /* MONADA_H */
