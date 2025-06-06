/* constants.h */
#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Device constants */
#define MAX_CHANNELS 8
#define MIN_DEVICE_ADDRESS 1
#define MAX_DEVICE_ADDRESS 254
#define MIN_TEMPERATURE -55.0
#define MAX_TEMPERATURE 125.0

/* Serial port constants */
#define DEFAULT_PORT "/dev/ttyUSB0"      /* Linux */
#define DEFAULT_ADDRESS '\x01'           /* Default device address */

/* Baudrate codes enumeration */
typedef enum {
    BAUD_1200 = 0,
    BAUD_2400 = 1,
    BAUD_4800 = 2,
    BAUD_9600 = 3,
    BAUD_19200 = 4,
    BAUD_INVALID = 9
} BaudrateCode;

#endif /* CONSTANTS_H */