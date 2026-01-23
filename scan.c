/*
 * RS485 bus scan functions
 * V1.0/2025-01-23
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "scan.h"
#include "typedef.h"
#include "packet.h"
#include "constants.h"

/*
 * Scan RS485 bus for Modbus RTU devices
 *
 * Sends a read register 0x0000 request to each address and collects responses.
 * Uses a short timeout (100ms) to keep total scan time reasonable.
 */
AppStatus scan_bus(int fd, uint8_t start_addr, uint8_t end_addr)
{
    uint8_t found[MAX_DEVICE_ADDRESS + 1];
    int found_count = 0;
    uint8_t input_data[4] = {0x00, 0x00, 0x00, 0x01}; /* Read register 0x0000, count 1 */
    PACKET tx_packet;
    PACKET rx_packet;
    AppStatus status;

    /* Scan all addresses in range */
    for (uint8_t addr = start_addr; addr <= end_addr; addr++) {
        /* Form read holding registers packet (function 0x03) */
        form_packet(addr, 0x03, input_data, 4, &tx_packet);

        /* Send packet */
        status = send_packet(fd, &tx_packet, NULL);
        if (status != STATUS_OK) {
            continue; /* Skip to next address on send error */
        }

        /* Try to receive response with short timeout, quiet mode */
        status = received_packet_timeout(fd, &rx_packet, RECEIVE_MODE_TEMPERATURE,
                                         SCAN_TIMEOUT_MS, 1);
        if (status == STATUS_OK) {
            /* Device responded - save address */
            found[found_count++] = addr;
        }

        /* Small inter-frame delay */
        usleep(5000);
    }

    /* Print results */
    if (found_count == 0) {
        printf("No devices found.\n");
    } else {
        printf("Found %d device(s):\n", found_count);
        for (int i = 0; i < found_count; i++) {
            printf("  %d\n", found[i]);
        }
    }

    return STATUS_OK;
}
