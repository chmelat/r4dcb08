/*
 * Device settings modification functions
 * V1.0/2025-04-17
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "write_functions.h"
#include "monada.h"
#include "typedef.h"
#include "constants.h"

/* 
 * Write a new device address
 */
AppStatus write_address(int fd, uint8_t adr, uint8_t n_adr) 
{
    int verb = 1; 
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t input_data[DMAX];
    AppStatus status;

    /* Validate input */
    if (n_adr < MIN_DEVICE_ADDRESS || n_adr > MAX_DEVICE_ADDRESS) {
        fprintf(stderr, "Device address %d is not %d..%d!\n", 
                n_adr, MIN_DEVICE_ADDRESS, MAX_DEVICE_ADDRESS);
        return ERROR_INVALID_ADDRESS;
    }

    /* Register address (2 byte) + Setting address (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0xFE; 
    input_data[2] = 0x00; 
    input_data[3] = n_adr;

    printf("Old address %d, new address %d\n", adr, n_adr);

    status = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "write_address", 1, NULL);
    if (status != STATUS_OK) {
        return ERROR_WRITE_ADDRESS;
    }
    
    return STATUS_OK;
}

/*
 * Get baudrate value from code
 */
AppStatus get_baudrate_value(uint8_t code, int *baudrate)
{
    if (baudrate == NULL) {
        return ERROR_INVALID_BAUDRATE;
    }
    
    switch (code) {
        case BAUD_1200:  *baudrate = 1200; break;
        case BAUD_2400:  *baudrate = 2400; break;
        case BAUD_4800:  *baudrate = 4800; break;
        case BAUD_9600:  *baudrate = 9600; break;
        case BAUD_19200: *baudrate = 19200; break;
        default:
            fprintf(stderr, "Invalid baudrate code: %d\n", code);
            return ERROR_INVALID_BAUDRATE;
    }
    
    return STATUS_OK;
}

/*
 * Write a new baudrate to the device
 */
AppStatus write_baudrate(int fd, uint8_t adr, uint8_t cbr)
{
    int verb = 1; 
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t input_data[DMAX];
    int baud;
    AppStatus status;

    /* Get baudrate value from code */
    status = get_baudrate_value(cbr, &baud);
    if (status != STATUS_OK) {
        return status;
    }

    /* Register address (2 byte) + Setting content (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0xFF; 
    input_data[2] = 0x00; 
    input_data[3] = cbr;

    printf("Set baudrate to %d, will be updated when module is powered on again!\n", baud);

    status = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "write_baudrate", 1, NULL);
    if (status != STATUS_OK) {
        return ERROR_WRITE_BAUDRATE;
    }
    
    return STATUS_OK;
}

/*
 * Write temperature correction value for a channel
 */
AppStatus write_correction(int fd, uint8_t adr, uint8_t ch, float T_c)
{
    int verb = 1;
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t input_data[DMAX];
    int16_t T;
    AppStatus status;

    /* Validate channel number */
    if (ch < 1 || ch > MAX_CHANNELS) {
        fprintf(stderr, "Invalid channel number (%d) - must be 1-%d\n", ch, MAX_CHANNELS);
        return ERROR_INVALID_CHANNEL;
    }

    T = (int16_t)(10*T_c);

    /* Register address (2 byte) + Setting content (2 byte) */
    input_data[0] = 0x00;
    input_data[1] = ch + 7; /* 0x08..0x0F */
    input_data[2] = (T >> 8) & 0xFF;  /* high byte */
    input_data[3] = T & 0xFF;         /* low byte */

    status = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "correction_temperature", 1, NULL);
    if (status != STATUS_OK) {
        return ERROR_WRITE_CORRECTION;
    }

    printf("Write temperature correction %.1f to channel %d\n", T_c, ch);
    return STATUS_OK;
}

/*
 * Perform factory reset on the device
 * Writes value 5 to register 0x00FF
 */
AppStatus factory_reset(int fd, uint8_t adr)
{
    int verb = 1;
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t input_data[DMAX];
    AppStatus status;

    /* Register address 0x00FF + Factory reset code 0x0005 */
    input_data[0] = 0x00;
    input_data[1] = 0xFF;
    input_data[2] = 0x00;
    input_data[3] = 0x05;  /* Factory reset command */

    printf("Performing factory reset on device at address %d...\n", adr);
    printf("WARNING: Device will reset to:\n");
    printf("  - Address: 1\n");
    printf("  - Baudrate: 9600\n");
    printf("  - All temperature corrections: 0\n");

    status = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "factory_reset", 1, NULL);

    /* Note: Device may not respond after reset, so timeout is acceptable */
    if (status == ERROR_PACKET_TIMEOUT) {
        printf("Factory reset sent (device may not respond - this is normal)\n");
        printf("IMPORTANT: Power cycle the device to load new settings from EEPROM!\n");
        return STATUS_OK;
    }

    if (status != STATUS_OK) {
        return ERROR_FACTORY_RESET;
    }

    printf("Factory reset command sent successfully.\n");
    printf("IMPORTANT: Power cycle the device to load new settings from EEPROM!\n");
    return STATUS_OK;
}