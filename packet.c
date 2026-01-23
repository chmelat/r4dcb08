/*
 *  Functions for packet handling in Modbus RTU protocol
 *  V1.0/2025-01-30
 *  V1.1/2025-03-28 Adaptation for R4DCB08 device
 *  V1.2/2025-04-16 Code improvements and robustness enhancements
 */
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>  /* Standard library */
#include <unistd.h>  /* UNIX standard function definitions */
#include <stdint.h>  /* Specific width integer types */
#include <string.h>  /* String functions */
#include <errno.h>   /* Error numbers */

#include "typedef.h" /* Data type definitions */
#include "packet.h"  /* Global function declarations */

/* Configuration constants */
#define READ_TIMEOUT_MS    500   /* Timeout for reading operations in milliseconds */
#define MIN_PACKET_SIZE    4     /* Minimum valid packet size: ADDR + FUNC + CRC(2) */
#define MAX_PACKET_SIZE    (DMAX + 4) /* Maximum valid packet size */

/* Error messages */
#define ERR_INVALID_LENGTH "Invalid data length"
#define ERR_INVALID_MODE   "Invalid receive mode"
#define ERR_CRC_MISMATCH   "CRC verification failed"
#define ERR_DATA_OVERFLOW  "Data length exceeds maximum"
#define ERR_WRITE_FAILED   "Failed to write packet to port"

/*
 *  Local function prototypes
 */
static uint16_t CRC16_2(const uint8_t *buf, int len);
static int read_with_timeout(int fd, uint8_t *buffer, int size, int timeout_ms);
static AppStatus received_packet_internal(int fd, PACKET *p_RP, int mode,
                                          int timeout_ms, int quiet);

/**********************************************************************/

/*
 *  Receive a packet from the device (internal implementation)
 */
static AppStatus received_packet_internal(int fd, PACKET *p_RP, int mode,
                                          int timeout_ms, int quiet)
{
    const char *msg = "received_packet";
    uint8_t buf[MAX_PACKET_SIZE];
    int bytes_read = 0;
    int total_length = 0;
    uint16_t crc_calculated;
    int i;

    /* Validate input parameters */
    if (p_RP == NULL) {
        if (!quiet) fprintf(stderr, "%s: NULL packet pointer\n", msg);
        return ERROR_PACKET_NULL;
    }

    if (mode < 0 || mode >= RECEIVE_MODE_MAX) {
        if (!quiet) fprintf(stderr, "%s: %s (%d)\n", msg, ERR_INVALID_MODE, mode);
        return ERROR_PACKET_MODE;
    }

    /* Read first byte (address) */
    bytes_read = read_with_timeout(fd, buf, 1, timeout_ms);
    if (bytes_read != 1) {
        if (!quiet) fprintf(stderr, "%s: Timeout waiting for address byte\n", msg);
        return ERROR_PACKET_TIMEOUT;
    }

    /* Determine expected packet length based on mode */
    if (mode == RECEIVE_MODE_TEMPERATURE) {
        /* Read function code + length */
        bytes_read = read_with_timeout(fd, buf + 1, 2, timeout_ms);
        if (bytes_read != 2) {
            if (!quiet) fprintf(stderr, "%s: Timeout waiting for function and length bytes\n", msg);
            return ERROR_PACKET_TIMEOUT;
        }

        /* Calculate total expected length */
        total_length = buf[2] + 5; /* ADDR + FUNC + LEN + DATA + CRC(2) */

        /* Validate expected length */
        if (total_length < MIN_PACKET_SIZE || total_length > MAX_PACKET_SIZE) {
            if (!quiet) fprintf(stderr, "%s: %s, invalid total length: %d\n",
                    msg, ERR_INVALID_LENGTH, total_length);
            return ERROR_PACKET_OVERFLOW;
        }

        /* Read remaining data and CRC */
        bytes_read = read_with_timeout(fd, buf + 3, total_length - 3, timeout_ms);
        if (bytes_read != total_length - 3) {
            if (!quiet) fprintf(stderr, "%s: %s, expected %d bytes, got %d\n",
                    msg, ERR_INVALID_LENGTH, total_length - 3, bytes_read);
            return ERROR_PACKET_TIMEOUT;
        }
    }
    else if (mode == RECEIVE_MODE_ACKNOWLEDGE) {
        /* Read remaining 7 bytes for acknowledge packet */
        bytes_read = read_with_timeout(fd, buf + 1, 7, timeout_ms);
        if (bytes_read != 7) {
            if (!quiet) fprintf(stderr, "%s: %s, expected 7 bytes, got %d\n",
                    msg, ERR_INVALID_LENGTH, bytes_read);
            return ERROR_PACKET_TIMEOUT;
        }
        total_length = 8;  /* Fixed length for acknowledge packet */
    }

    /* Extract packet fields */
    p_RP->addr = buf[0];
    p_RP->inst = buf[1];

    if (mode == RECEIVE_MODE_TEMPERATURE) {
        p_RP->len = buf[2];
        /* Copy data bytes */
        for (i = 0; i < p_RP->len && i < DMAX; i++) {
            p_RP->data[i] = buf[i + 3];
        }
        /* Extract CRC */
        p_RP->CRC = UINT16(buf[total_length - 2], buf[total_length - 1]);

        /* Verify CRC */
        crc_calculated = CRC16_2(buf, p_RP->len + 3);
    }
    else { /* RECEIVE_MODE_ACKNOWLEDGE */
        p_RP->len = 2;  /* Fixed data length for acknowledge */
        /* Copy data bytes - position is different for acknowledge packets */
        for (i = 0; i < p_RP->len && i < DMAX; i++) {
            p_RP->data[i] = buf[i + 4];
        }
        /* Extract CRC */
        p_RP->CRC = UINT16(buf[total_length - 2], buf[total_length - 1]);

        /* Verify CRC */
        crc_calculated = CRC16_2(buf, p_RP->len + 4);
    }

    /* Check if calculated CRC matches received CRC */
    if (crc_calculated != p_RP->CRC) {
        if (!quiet) {
            fprintf(stderr, "%s: %s, calculated: 0x%04X, received: 0x%04X\n",
                    msg, ERR_CRC_MISMATCH, crc_calculated, p_RP->CRC);
            print_packet(p_RP);
        }
        return ERROR_PACKET_CRC;
    }

    /* Small delay to ensure stable operation */
    usleep((useconds_t)(8000));

    return STATUS_OK;
}

/*
 *  Receive a packet from the device
 */
AppStatus received_packet(int fd, PACKET *p_RP, int mode)
{
    return received_packet_internal(fd, p_RP, mode, READ_TIMEOUT_MS, 0);
}

/*
 *  Receive a packet from the device with custom timeout
 */
AppStatus received_packet_timeout(int fd, PACKET *p_RP, int mode,
                                  int timeout_ms, int quiet)
{
    return received_packet_internal(fd, p_RP, mode, timeout_ms, quiet);
}

/*
 *  Send a packet to the device
 */
AppStatus send_packet(int fd, PACKET *p_SP, int *bytes_sent)
{
    const char *msg = "send_packet";
    uint8_t buf[MAX_PACKET_SIZE];
    int i;

    /* Validate input parameters */
    if (p_SP == NULL) {
        fprintf(stderr, "%s: NULL packet pointer\n", msg);
        return ERROR_PACKET_NULL;
    }

    if (p_SP->len > DMAX) {
        fprintf(stderr, "%s: %s (%u > %d)\n", msg, ERR_DATA_OVERFLOW, p_SP->len, DMAX);
        return ERROR_PACKET_OVERFLOW;
    }

    /* Assemble packet */
    buf[0] = p_SP->addr;  /* Address */
    buf[1] = p_SP->inst;  /* Instruction */
    
    /* Copy data */
    for (i = 0; i < p_SP->len && i < DMAX; i++) {
        buf[i + 2] = p_SP->data[i];
    }
    
    /* Add CRC */
    buf[p_SP->len + 2] = (p_SP->CRC & 0xFF);     /* CRC_lo */
    buf[p_SP->len + 3] = ((p_SP->CRC >> 8) & 0xFF); /* CRC_hi */

    /* Write packet to device */
    if (write(fd, buf, p_SP->len + 4) < 0) {
        fprintf(stderr, "%s: %s (errno: %d, %s)\n", 
                msg, ERR_WRITE_FAILED, errno, strerror(errno));
        return ERROR_PACKET_WRITE;
    }
    
    /* Store bytes sent if requested */
    if (bytes_sent != NULL) {
        *bytes_sent = p_SP->len;
    }
    
    return STATUS_OK;
}

/*
 *  Form a packet with given parameters
 */
void form_packet(uint8_t addr, uint8_t inst, uint8_t *p_data,
                 int len, PACKET *p_Packet)
{
    uint8_t buf[MAX_PACKET_SIZE];
    uint16_t crc_calculated;
    int i;
    
    /* Validate input parameters */
    if (p_Packet == NULL || p_data == NULL || len < 0 || len > DMAX) {
        fprintf(stderr, "form_packet: Invalid parameters\n");
        return;
    }
    
    /* Fill packet structure */
    p_Packet->addr = addr;
    p_Packet->inst = inst;
    p_Packet->len = len;
    
    /* Assemble buffer for CRC calculation */
    buf[0] = addr;
    buf[1] = inst;
    
    /* Copy data */
    for (i = 0; i < len; i++) {
        p_Packet->data[i] = p_data[i];
        buf[i + 2] = p_data[i];
    }
    
    /* Calculate and set CRC */
    crc_calculated = CRC16_2(buf, len + 2);
    p_Packet->CRC = crc_calculated;
}

/*
 *  Print packet contents to stdout
 */
void print_packet(PACKET *p_P)
{
    int i;

    /* Validate input parameter */
    if (p_P == NULL) {
        fprintf(stderr, "print_packet: NULL packet pointer\n");
        return;
    }

    printf("ADR INS DATA... CRC CRC\n");
    printf("%02X %02X ", p_P->addr, p_P->inst);
    
    for (i = 0; i < p_P->len; i++) {
        printf("%02X ", p_P->data[i]);
    }
    
    printf("%02X %02X\n", (p_P->CRC) & 0xFF, (p_P->CRC) >> 8);
}

/* Local functions */

/*
 *  Calculate Modbus CRC16
 */
static uint16_t CRC16_2(const uint8_t *buf, int len)
{ 
    uint16_t crc = 0xFFFF;
    
    /* Validate input parameters */
    if (buf == NULL || len <= 0) {
        return 0;
    }
    
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];    /* XOR byte into least sig. byte of crc */

        for (int i = 8; i != 0; i--) {    /* Loop over each bit */
            if ((crc & 0x0001) != 0) {    /* If the LSB is set */
                crc >>= 1;                /* Shift right and XOR 0xA001 */
                crc ^= 0xA001;
            }
            else {                        /* Else LSB is not set */
                crc >>= 1;                /* Just shift right */
            }
        }
    }
    
    return crc;
}

/*
 *  Read data with timeout
 */
static int read_with_timeout(int fd, uint8_t *buffer, int size, int timeout_ms)
{
    fd_set readfds;
    struct timeval tv;
    int bytes_read = 0;
    int result;
    
    /* Validate input parameters */
    if (buffer == NULL || size <= 0) {
        return 0;
    }
    
    while (bytes_read < size) {
        /* Set up for select */
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        /* Set timeout */
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        
        /* Wait for data or timeout */
        result = select(fd + 1, &readfds, NULL, NULL, &tv);
        
        if (result < 0) {
            /* Error in select */
            fprintf(stderr, "read_with_timeout: select error: %s\n", strerror(errno));
            return bytes_read;
        } 
        else if (result == 0) {
            /* Timeout */
            return bytes_read;
        }
        
        /* Data available, read it */
        result = read(fd, buffer + bytes_read, size - bytes_read);
        
        if (result < 0) {
            /* Read error */
            fprintf(stderr, "read_with_timeout: read error: %s\n", strerror(errno));
            return bytes_read;
        } 
        else if (result == 0) {
            /* End of file */
            return bytes_read;
        }
        
        bytes_read += result;
    }
    
    return bytes_read;
}
