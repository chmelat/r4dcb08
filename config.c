/* config.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include "config.h"
#include "error.h"
#include "serial.h"
#include "packet.h"
#include "monada.h"
#include "now.h"
#include "median_filter.h"
#include "read_functions.h"
#include "write_functions.h"
#include "help_functions.h"
#include "constants.h"

/* External global variables */
extern char *progname;


/* Initialize configuration */
void init_config(ProgramConfig *config) {
    config->port = NULL;
    config->address = DEFAULT_ADDRESS;
    config->baudrate = 9600;
    config->time_step = 1;
    config->num_channels = 1;
    config->read_correction = 0;
    config->new_address = 0;
    config->baudrate_code = BAUD_INVALID;
    config->channel = -1;
    config->correction_temp = 0.0;
    config->enable_median_filter = 0;
    config->one_shot = 0;
}

/* Validate device address */
static AppStatus validate_device_address(uint8_t address) {
    if (address < MIN_DEVICE_ADDRESS || address > MAX_DEVICE_ADDRESS) {
        fprintf(stderr, "Device address %d is not %d..%d!\n", 
                address, MIN_DEVICE_ADDRESS, MAX_DEVICE_ADDRESS);
        return ERROR_INVALID_ADDRESS;
    }
    return STATUS_OK;
}

/* Process command line arguments */
AppStatus parse_arguments(int argc, char *argv[], ProgramConfig *config) {
    int c;

    while ((c = getopt(argc, argv, "p:a:b:t:n:cw:s:x:mfh?")) != -1) {
        switch (c) {
            case 'p':  /* Port name */
                config->port = optarg;
                break;
            case 'a':  /* Device address */
                config->address = (uint8_t)atoi(optarg);
                if (validate_device_address(config->address) != STATUS_OK) {
                    return ERROR_INVALID_ADDRESS;
                }
                break;
            case 'b':  /* Baudrate */
                config->baudrate = (int)atoi(optarg);
                break;
            case 't':  /* Time step */
                config->time_step = atoi(optarg);
                if (config->time_step < 0) {
                    fprintf(stderr, "Time step must be positive or zero!\n");
                    return ERROR_INVALID_TIME;
                }
                break;
            case 'n': /* Number of channels */
                config->num_channels = atoi(optarg);
                if (config->num_channels < 1 || config->num_channels > MAX_CHANNELS) {
                    fprintf(stderr, "Number of channels %d is not 1..%d!\n", 
                            config->num_channels, MAX_CHANNELS);
                    return ERROR_INVALID_CHANNEL;
                }
                break;
            case 'c':  /* Read correction */
                config->read_correction = 1;
                break;
            case 'w':  /* Write address */
                config->new_address = (uint8_t)(atoi(optarg));
                if (validate_device_address(config->new_address) != STATUS_OK) {
                    return ERROR_INVALID_ADDRESS;
                }
                break;
            case 's':  /* Set correction */
                if (sscanf(optarg, "%d,%f", &config->channel, &config->correction_temp) != 2) {
                    fprintf(stderr, "Invalid format for -s parameter, expected ch,value\n");
                    return ERROR_INVALID_CHANNEL;
                }
                if (config->channel < 1 || config->channel > MAX_CHANNELS) {
                    fprintf(stderr, "Invalid channel number (%d) in -s option!\n", 
                            config->channel);
                    return ERROR_INVALID_CHANNEL;
                }
                break;
            case 'x':  /* Set device baudrate */
                config->baudrate_code = (uint8_t)(atoi(optarg));
                if (config->baudrate_code > BAUD_19200) {
                    fprintf(stderr, "Invalid code baudrate (%d) in -x option!\n", 
                            config->baudrate_code);
                    return ERROR_INVALID_BAUDRATE;
                }
                break;
            case 'm':  /* Enable filter */
                config->enable_median_filter = 1;
                break;
            case 'f':  /* Enable one shot */
                config->one_shot = 1;
                break;
            case 'h':  /* Help */
            case '?':  /* Help */
                help();
                exit(EXIT_SUCCESS);
            default:
                break;
        }
    }

    if (argc > optind) {  /* Too many arguments */
        fprintf(stderr, "Too many arguments!\n");
        usage();
        return ERROR_TOO_MANY_ARGS;
    }

    return STATUS_OK;
}

/* Initialize port (from main.c) */
static AppStatus init_port(char *device, int baud, int *fd) {
    int rc;

    *fd = open_port(device);
    if (*fd < 0) {
        return ERROR_PORT_INIT;
    }
    
    rc = set_port(*fd, baud);
    if (rc < 0) {
        close(*fd);
        *fd = -1;
        return ERROR_PORT_INIT;
    }
    
    return STATUS_OK;
}

/* Execute command according to configuration */
AppStatus execute_command(ProgramConfig *config) {
    int fd;
    AppStatus status;
    char *device = config->port ? config->port : DEFAULT_PORT;
    
    /* Initialize port */
    status = init_port(device, config->baudrate, &fd);
    if (status != STATUS_OK) {
        return status;
    }
    
    /* Process commands in priority order */
    if (config->baudrate_code != BAUD_INVALID) {
        status = write_baudrate(fd, config->address, config->baudrate_code);
        close(fd);
        return status;
    }
    
    if (config->new_address) {
        status = write_address(fd, config->address, config->new_address);
        close(fd);
        return status;
    }

    if (config->channel >= 0) {
        status = write_correction(fd, config->address, config->channel, 
                                 config->correction_temp);
        close(fd);
        return status;
    }

    if (config->read_correction) {
        status = read_correction(fd, config->address);
        close(fd);
        return status;
    }

    if (config->enable_median_filter) 
        printf("# Active three-point median filter for all data ...\n#\n");

    /* Default action - read temperature */
    status = read_temp(fd, config->address, config->num_channels, 
                     config->time_step, config->enable_median_filter, config->one_shot);
    
    fflush(stdout);
    close(fd);
    return status;
}
