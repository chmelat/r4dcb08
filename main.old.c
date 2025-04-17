/*
 *  V1.4/2025-04-14
 *  Temperature sensor module R4DCB08 communication utility
 */

#include <stdlib.h>  /* Standard lib */
#include <stdio.h>   /* Standard input/output definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <stdint.h>  /* Integer types */
#include <time.h>    /* Time function */
#include <libgen.h>  /* Required for basename in newer C versions */
#include <string.h>
#include <signal.h>  /* Signal handling */

#include "typedef.h"
#include "serial.h"
#include "now.h"
#include "packet.h"
#include "monada.h"
#include "revision.h"
#include "define_error_resp.h"
#include "median_filter.h"

/* Local define */
#define PORT "/dev/ttyUSB0"  /* Linux */
// #define PORT "/dev/ttyU0"  /* OpenBSD */
#define ADR '\x01'           /* Default address */
#define MAX_ARG 20           /* Maximum number of main() arguments, excess will be cut off */

/* Constants to replace magic values */
#define MAX_CHANNELS 8
#define MIN_DEVICE_ADDRESS 1
#define MAX_DEVICE_ADDRESS 254
#define MIN_TEMPERATURE -55.0
#define MAX_TEMPERATURE 125.0

/* Baudrate codes enumeration */
typedef enum {
    BAUD_1200 = 0,
    BAUD_2400 = 1,
    BAUD_4800 = 2,
    BAUD_9600 = 3,
    BAUD_19200 = 4,
    BAUD_INVALID = 9  /* Original default value */
} BaudrateCode;

/* Signal handling for clean termination */
volatile sig_atomic_t running = 1;

/* Local function declarations */
static void read_temp(int fd, uint8_t adr, int n, int dt, int m_f);
static void read_correction(int fd, uint8_t adr);
static int write_address(int fd, uint8_t adr, uint8_t n_adr);
static int write_baudrate(int fd, uint8_t adr, uint8_t cbr);
static int write_correction(int fd, uint8_t adr, uint8_t ch, float T);
static int init(char *device, int baud);
static void help(void);
static void usage(void);
static int validate_device_address(uint8_t address);
static int get_baudrate_value(uint8_t code);
static void handle_signal(int sig);

/* Global variables */
char *progname;

int main(int argc, char *argv[])
{
    int fd;            /* file descriptor */
    int c;             /* Option for getopt */
    char *port = NULL; /* port name from command line */
    uint8_t adr = ADR; /* Device address */
    int baud = 9600;   /* Baud rate of serial port, default value = 9600 */
    int dt = 1;        /* Approx. delay between measurements [s] */
    int n = 1;         /* Number of channels (1..8), default 1 */
    int ct = 0;        /* Read and print correction temperature */
    uint8_t n_adr = 0; /* Write new device address */
    uint8_t cbr = BAUD_INVALID; /* Code for baudrate */
    int m_f = 0;        /* Disable/Enable three point median filter */

    int ch = -1;       /* Channel */
    float T_c = 0.0;   /* Correction temperature */

    progname = basename(argv[0]);

    /* Command line options */
    while ((c = getopt(argc, argv, "p:a:b:t:n:cw:s:x:mh?")) != -1) {
        switch (c) {
            case 'p':  /* Port name */
                port = optarg;
                break;
            case 'a':  /* Device address specification (default is universal address ADR) */
                adr = (uint8_t)atoi(optarg);
                if (validate_device_address(adr) != 0) {
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':  /* Set baudrate on serial port */
                baud = (int)atoi(optarg);
                break;
            case 't':  /* delta time */
                dt = atoi(optarg);
                if (dt < 0) {
                    fprintf(stderr, "Time step must be positive or zero!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'n': /* Number of channels */
                n = atoi(optarg);
                if (n < 1 || n > MAX_CHANNELS) {
                    fprintf(stderr, "Number of channels %d is not 1..%d!\n", n, MAX_CHANNELS);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':  /* Read correction temperature */
                ct = 1;
                break;
            case 'w':  /* Write device address */
                n_adr = (uint8_t)(atoi(optarg));
                if (validate_device_address(n_adr) != 0) {
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':  /* Set correction value {channel, value} */
                if (sscanf(optarg, "%d,%f", &ch, &T_c) != 2) {
                    fprintf(stderr, "Invalid format for -s parameter, expected ch,value\n");
                    exit(EXIT_FAILURE);
                }
                if (ch < 1 || ch > MAX_CHANNELS) {
                    fprintf(stderr, "Invalid channel number (%d) in -s option!\n", ch);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'x':  /* Set baudrate of device */
                cbr = (uint8_t)(atoi(optarg));
                if (cbr > BAUD_19200) {
                    fprintf(stderr, "Invalid code baudrate (%d) in -x option!\n", cbr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':  /* Enable three point median filter */
                m_f = 1;
                break;
            case 'h':  /* Help */
                help();
                exit(EXIT_SUCCESS);
            case '?':  /* Help */
                help();
                exit(EXIT_SUCCESS);
            default:
                break;
        }
    }

    if (argc > optind) {  /* More arguments than processed options */
        fprintf(stderr, "Too many arguments!\n");
        usage();
        exit(EXIT_FAILURE);
    }

    /* Initialize port */
    if (port == NULL)
        fd = init(PORT, baud);  /* Default port name */
    else
        fd = init(port, baud);  /* Specified port name */
   
    if (fd < 0) {
        fprintf(stderr, "Failed to initialize the serial port\n");
        exit(EXIT_FAILURE);
    }
   
    /* Process commands in priority order */
    if (cbr != BAUD_INVALID) {
        if (write_baudrate(fd, adr, cbr) != 0) {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    
    if (n_adr) {
        if (write_address(fd, adr, n_adr) != 0) {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    if (ch >= 0) {
        if (write_correction(fd, adr, ch, T_c) != 0) {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    if (ct) {
        read_correction(fd, adr);
        exit(EXIT_SUCCESS);
    }

    if (m_f) printf("# Active three-point median filter for all data ...\n#\n");

    /* Default action - read temperature */
    read_temp(fd, adr, n, dt, m_f);

    fflush(stdout);  /* Flush stdout */
    close(fd);       /* Close port */

    return 0;
}

/*
 *  Read and print temperature 1..n channel
 */
static void read_temp(int fd, uint8_t adr, int n, int dt, int m_f)
{
    int verb = 0;
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int i;
    int rc;
    float T[MAX_CHANNELS];
    float T_f[MAX_CHANNELS];
    char *sample_time = NULL;
    char sample_t_f[DBUF];  /* Sample time after filtering */

    /* Set up signal handler for clean termination */
    signal(SIGINT, handle_signal);

    /* Register address (2 byte) + Read number (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0x00; 
    input_data[2] = 0x00; 
    input_data[3] = n;

    printf("# Date                ");
    for (i=1; i<=n; i++) {
        printf("  Ch%d",i);
    }  
    printf("\n");

    /* Modified loop to allow termination with Ctrl+C */
    while (running) {
        p_data = monada(fd, adr, '\x03', 4, input_data, p_pr, verb, "read_temp", 0);

        sample_time = now();

        for (i=0; i<n; i++) {
            T[i] = (float)INT16(p_data[2*i+1], p_data[2*i])/10; /* Temperature [C] */
            if (T[i] < MIN_TEMPERATURE || T[i] > MAX_TEMPERATURE)
              T[i] = ERRRESP;
        }

        if (m_f) {
          rc = median_filter(sample_time, n, T, sample_t_f,T_f);
          if (rc != MF_SUCCESS) {
            fprintf(stderr, "Median filter failed with code %d\n", rc);
            exit (EXIT_FAILURE);
          }
          strcpy(sample_time, sample_t_f);
          for (i=0; i<n; i++) {
            T[i] = T_f[i];
          }
        }

        
        printf("%s ", sample_time);
        for (i=0; i<n; i++) {
          if (T[i] != ERRRESP) 
            printf(" %.1f", T[i]);
          else
            printf("  NaN");
        }
        printf("\n");
        usleep((useconds_t)(dt*1E6));
    }
    
    printf("\nMeasurement stopped\n");
}

/*
 *  Read and print correction temperature 1..8 channel
 */
static void read_correction(int fd, uint8_t adr)
{
    int verb = 0;
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int i;
    float T;

    /* Register address (2 byte) + Read number (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0x08; 
    input_data[2] = 0x00; 
    input_data[3] = 8;

    printf("Temperature correction [C]\n");
    for (i=1; i<=MAX_CHANNELS; i++) {
        printf("  Ch%d",i);
    }  
    printf("\n");

    p_data = monada(fd, adr, '\x03', 4, input_data, p_pr, verb, "read_correction", 0);
    if (p_data == NULL) {
        fprintf(stderr, "Failed to read correction values\n");
        return;
    }

    for (i=0; i<MAX_CHANNELS; i++) {
        T = (float)INT16(p_data[2*i+1], p_data[2*i])/10; /* Temperature [C] */
        printf(" %.1f", T);
    }
    printf("\n\n");
}

static int write_address(int fd, uint8_t adr, uint8_t n_adr)
{
    int verb = 1; 
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];

    /* Validate input */
    if (validate_device_address(n_adr) != 0) {
        return -1;
    }

    /* Register address (2 byte) + Setting address (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0xFE; 
    input_data[2] = 0x00; 
    input_data[3] = n_adr;

    printf("Old address %d, new address %d\n", adr, n_adr);

    p_data = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "write_address", 1);
    
    /* Check for errors */
    if (p_data == NULL) {
        fprintf(stderr, "Failed to write new address\n");
        return -1;
    }
    
    return 0;
}

static int write_baudrate(int fd, uint8_t adr, uint8_t cbr)
{
    int verb = 1; 
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int baud;

    /* Get baudrate value from code */
    baud = get_baudrate_value(cbr);
    if (baud < 0) {
        return -1;
    }

    /* Register address (2 byte) + Setting content (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0xFF; 
    input_data[2] = 0x00; 
    input_data[3] = cbr;

    printf("Set baudrate to %d, will be updated when module is powered on again!\n", baud);

    p_data = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "write_baudrate", 1);
    
    /* Check for errors */
    if (p_data == NULL) {
        fprintf(stderr, "Failed to write baudrate\n");
        return -1;
    }
    
    return 0;
}

static int write_correction(int fd, uint8_t adr, uint8_t ch, float T_c)
{
    int verb = 1;	
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int16_t T;
    char *p_char;

    /* Validate channel number */
    if (ch < 1 || ch > MAX_CHANNELS) {
        fprintf(stderr, "Invalid channel number (%d) - must be 1-%d\n", ch, MAX_CHANNELS);
        return -1;
    }

    T = (int16_t)(10*T_c);
    p_char = (char *)(&T);

    /* Register address (2 byte) + Setting content (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = ch + 7; /* 0x08..0x0F */ 
    input_data[2] = *(p_char+1); 
    input_data[3] = *p_char;

    p_data = monada(fd, adr, '\x06', 4, input_data, p_pr, verb, "correction_temperature", 1);
    
    /* Check for errors */
    if (p_data == NULL) {
        fprintf(stderr, "Failed to write temperature correction\n");
        return -1;
    }
    
    printf("Write temperature correction %.1f to channel %d\n", T_c, ch);
    return 0;
}

/*
 *  Signal handler for clean termination
 */
static void handle_signal(int sig)
{
    running = 0;
}

/*
 *  Helper function to validate device address
 */
static int validate_device_address(uint8_t address)
{
    if (address < MIN_DEVICE_ADDRESS || address > MAX_DEVICE_ADDRESS) {
        fprintf(stderr, "Device address %d is not %d..%d!\n", 
                address, MIN_DEVICE_ADDRESS, MAX_DEVICE_ADDRESS);
        return -1;
    }
    return 0;
}

/*
 *  Helper function to get baudrate value from code
 */
static int get_baudrate_value(uint8_t code)
{
    switch (code) {
        case BAUD_1200:  return 1200;
        case BAUD_2400:  return 2400;
        case BAUD_4800:  return 4800;
        case BAUD_9600:  return 9600;
        case BAUD_19200: return 19200;
        default:
            fprintf(stderr, "Invalid baudrate code: %d\n", code);
            return -1;
    }
}

/*
 *  Initialize serial port
 */
static int init(char *device, int baud)
{
    int fd;  /* File descriptor */
    int rc;

    fd = open_port(device);
    rc = set_port(fd, baud);
    if (rc < 0) {
        fprintf(stderr, "Problem with set_port(), rc = %d\n", rc);
        exit(EXIT_FAILURE);
    }
    return fd;
}

/*
 *  Help
 */
static void help(void)
{
    static char *msg[] = {
        "-h or -?\tHelp",
        "-p [name]\tSelect port (default: "PORT")",
        "-a [address]\tSelect address (default: '01H')",
        "-b [n]\t\tSet baud rate on serial port {1200, 2400, 4800, 9600, 19200}, def. 9600",
        "-t [time]\tTime step [s], (default 1 s)",
        "-c\t\tRead correction temperature [C]",
        "-w [address]\tWrite new device address (1..254)",
        "-b [n]\t\tSet baud rate serial port {1200, 2400, 4800, 9600, 19200}, def. 9600",
        "-x [n]\t\tSet baud rate on R4DCB08 device {0:1200, 1:2400, 2:4800, 3:9600, 4:19200}, def. 9600",
        "-s [ch,Tc]\tSet temperature correction Tc for channel ch",
        "-m\t\tEnable three point median filter",
        0
    };
  
    char **p = msg;
    fprintf(stderr, "\n%s V%s (%s)\n", progname, VERSION, REVDATE);
    fprintf(stderr, "Use Ctrl+C to stop continuous temperature readings\n");
    while (*p) fprintf(stderr, "%s\n", *p++);
}

/*
 *  Usage
 */
static void usage(void)
{
    fprintf(stderr, "usage: %s [OPTIONS]\nFor help, type: '%s -h or -?'\n", progname, progname);
}
