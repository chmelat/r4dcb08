/* config.h */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "error.h"

/* Structure for storing program configuration */
typedef struct {
    char *port;              /* Port name */
    uint8_t address;         /* Device address */
    int baudrate;            /* Port baudrate */
    int time_step;           /* Time step between measurements [s] */
    int num_channels;        /* Number of channels (1..8) */
    int read_correction;     /* 1 to read correction temperature, 0 otherwise */
    uint8_t new_address;     /* New device address */
    uint8_t baudrate_code;   /* Device baudrate code */
    int channel;             /* Channel number for correction */
    float correction_temp;   /* Correction temperature */
    int enable_median_filter;/* 1 to enable median filter, 0 otherwise */
    int enable_maf_filter;   /* 1 to enable MAF filter, 0 otherwise */
    int maf_window_size;     /* MAF window size (odd, 3-15) */
    int one_shot;            /* 1 enable one shot measure, 0 othervise */
    int factory_reset;       /* 1 to perform factory reset, 0 otherwise */
    int scan_mode;           /* 1 to scan bus for devices, 0 otherwise */
} ProgramConfig;

/**
 * Initializes configuration with default values
 * @param config Pointer to the configuration structure
 */
void init_config(ProgramConfig *config);

/**
 * Processes command line arguments
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param config Pointer to the configuration structure
 * @return STATUS_OK on success, otherwise an error code
 */
AppStatus parse_arguments(int argc, char *argv[], ProgramConfig *config);

/**
 * Executes actions according to the configuration
 * @param config Configuration structure
 * @return STATUS_OK on success, otherwise an error code
 */
AppStatus execute_command(ProgramConfig *config);

#endif /* CONFIG_H */
