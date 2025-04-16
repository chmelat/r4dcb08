/*
 *  Wrapper function for device communication
 *  V1.0/2016-02-08
 *  V1.1/2019-10-28
 *  V1.2/2019-10-30  Autonomous file
 *  V1.3/2019-10-31  Add p_r (pointer to received packet) and verb as parameter
 *  V1.4/2025-04-16  Improved error handling and robustness
 */

#include <stdlib.h>  /* Standard lib */
#include <stdio.h>   /* Standard input/output definitions */
#include <stdint.h>  /* Standard integer types */
#include <string.h>  /* String functions */

#include "typedef.h"  /* Data type definitions */
#include "packet.h"   /* Packet functions declarations */
#include "monada.h"   /* Function declarations */

/*
 *  Global variables
 */
static MonadaStatus last_status = MONADA_OK;  /* Status of the last operation */

/*
 *  Send an instruction to the device and receive the response
 */
uint8_t *monada(int fd, uint8_t adr, uint8_t inst, int in_len, 
               uint8_t *arg, PACKET *p_r, int verb, 
               const char *msg, int mode)
{
    static uint8_t data[DMAX];  /* Array for data (not thread-safe) */
    uint8_t *p_data = data;     /* Pointer to data array */
    static PACKET tx_packet;    /* Send packet */
    static PACKET rx_packet;    /* Received packet */
    PACKET *p_tx = &tx_packet;  /* Pointer to send packet */
    PACKET *p_rx = &rx_packet;  /* Pointer to received packet */
    int i, result;
    const char *function_name = "monada";

    /* Reset status */
    last_status = MONADA_OK;

    /* Validate parameters */
    if (p_r == NULL || msg == NULL || fd < 0 || 
        mode < 0 || mode >= RECEIVE_MODE_MAX) {
        fprintf(stderr, "%s: Invalid parameter(s)\n", function_name);
        last_status = MONADA_ERROR_INVALID_PARAM;
        return NULL;
    }

    /* Check input data length */
    if (in_len > DMAX || in_len < 0) {
        fprintf(stderr, "%s: In %s - data too long: %d > %d!\n", 
                function_name, msg, in_len, DMAX);
        last_status = MONADA_ERROR_DATA_TOO_LONG;
        return NULL;
    }

    /* Copy input data to local buffer */
    if (arg != NULL && in_len > 0) {
        for (i = 0; i < in_len; i++) {
            data[i] = *(arg++);  /* Using the original style of copying */
        }
    }

    /* Form packet to send */
    form_packet(adr, inst, p_data, in_len, p_tx);
 
    #ifdef DEBUG
        printf("%s: Send packet:\n", function_name);
        print_packet(p_tx);
    #endif

    /* Send packet */
    result = send_packet(fd, p_tx);
    if (result < 0) {
        fprintf(stderr, "%s: In %s - send error!\n", function_name, msg);
        last_status = MONADA_ERROR_SEND_FAILED;
        return NULL;
    }

    /* Receive response */
    result = received_packet(fd, p_rx, mode);
    if (result < 0) {
        fprintf(stderr, "%s: In %s - receive error (mode %d)!\n", 
                function_name, msg, mode);
        last_status = MONADA_ERROR_RECEIVE_FAILED;
        return NULL;
    }

    #ifdef DEBUG
        printf("%s: Received packet:\n", function_name);
        print_packet(p_rx);
    #endif 

    /* Print success message if verbose mode is on */
    if (verb) {
        printf("%s ... OK\n", msg);
    }

    /* Copy received packet to output parameter */
    *p_r = rx_packet;
    
    /* Return pointer to received data */
    return p_rx->data;
}

/*
 *  Get the status of the last monada operation
 */
MonadaStatus monada_status(void)
{
    return last_status;
}
