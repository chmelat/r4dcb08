/*
 *  Functions for packet handling in Modbus RTU protocol
 *  V1.2/2025-04-16
 */
#ifndef PACKET_H
#define PACKET_H

//#include <stdint.h>  /* For uint8_t, uint16_t */
//#include "typedef.h" /* For PACKET definition */

/**
 * Receive mode definitions
 */
typedef enum {
    RECEIVE_MODE_TEMPERATURE = 0,  /* Packet from temperature read */
    RECEIVE_MODE_ACKNOWLEDGE = 1,  /* Acknowledge packet from setting operations */
    RECEIVE_MODE_MAX
} ReceiveMode;

/**
 * Receive a packet from the device
 *
 * @param fd    File descriptor of the serial port
 * @param p_RP  Pointer to PACKET structure to store received data
 * @param mode  Receiving mode (RECEIVE_MODE_TEMPERATURE or RECEIVE_MODE_ACKNOWLEDGE)
 * @return      0 on success, negative value on error
 *              -1: General error
 *              -2: Invalid mode
 */
extern int received_packet(int fd, PACKET *p_RP, int mode);

/**
 * Send a packet to the device
 *
 * @param fd    File descriptor of the serial port
 * @param p_SP  Pointer to PACKET structure with data to send
 * @return      Number of data bytes sent on success, negative value on error
 */
extern int send_packet(int fd, PACKET *p_SP);

/**
 * Form a packet with given parameters
 *
 * @param addr      Device address
 * @param inst      Instruction code
 * @param p_data    Pointer to data buffer
 * @param len       Length of data
 * @param p_Packet  Pointer to PACKET structure to store the formed packet
 */
extern void form_packet(uint8_t addr, uint8_t inst, uint8_t *p_data,
                        int len, PACKET *p_Packet);

/**
 * Print packet contents to stdout
 *
 * @param p_P  Pointer to PACKET structure to print
 */
extern void print_packet(PACKET *p_P);

#endif /* PACKET_H */
