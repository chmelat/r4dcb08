/*
 *  Function for open and set of parameter communication on serial port
 *  V1.0/121107
 *  V1.1/250109  Add select baud rate of serial port
 *  V1.2/250308 Optimized version (AI)
 *  V1.3/250313 Updated to return error codes instead of exiting
 */
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>  /* Standard lib */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include "serial.h"

/*
 *  Converts numeric baud rate to system constant
 *  Returns speed_t value or B0 for unsupported baud rates
 */
static speed_t get_baud(int baud)
{
    static const struct {
        int rate;
        speed_t speed;
    } baud_table[] = {
        { 1200, B2400 },
        { 2400, B2400 },
        { 4800, B4800 },
        { 9600, B9600 },
        { 19200, B19200 },
        { 38400, B38400 },
        { 57600, B57600 },
        { 115200, B115200 },
        { 0, 0 }
    };

    for (int i = 0; baud_table[i].rate != 0; i++) {
        if (baud_table[i].rate == baud)
            return baud_table[i].speed;
    }

    fprintf(stderr, "Incorrect baud rate %d!\n", baud);
    return B0;  /* 0 baud rate signifies error */
}

/*
 *  Opens the serial port
 *  Returns file descriptor or negative error code
 */
int open_port(const char *device)
{
    int fd; /* File descriptor for the port */
    
    /* Validate input */
    if (device == NULL) {
        fprintf(stderr, "open_port: NULL device name\n");
        return SERIAL_ERROR_OPEN;
    }

    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        /*
         *  O_NOCTTY - terminal won't interfere with communication
         *  O_NDELAY - ignore DCD signal state
         */
        fprintf(stderr, "open_port: Unable to open %s - %s\n", 
                device, strerror(errno));
        return SERIAL_ERROR_OPEN;
    }

    if (fcntl(fd, F_SETFL, 0) == -1) {
        /*
         *  Setting port to blocking mode (0 instead of FNDELAY)
         */
        fprintf(stderr, "open_port: fcntl error - %s\n", strerror(errno));
        close(fd);
        return SERIAL_ERROR_FCNTL;
    }

    return fd;
}

/*
 *  Configures the serial port with specified baud rate
 *  Uses blocking mode with 1.5 second timeout
 *  Returns 0 on success, negative error code on failure
 */
int set_port(int fd, int baud)
{
    struct termios options;  /* Terminal configuration struct */
    speed_t baud_rate;
    
    /* Validate file descriptor */
    if (fd < 0) {
        fprintf(stderr, "set_port: Invalid file descriptor\n");
        return SERIAL_ERROR_ATTR;
    }
    
    /* Get baud rate constant */
    baud_rate = get_baud(baud);
    if (baud_rate == B0) {
        return SERIAL_ERROR_BAUD;
    }

    /* Get current port settings */
    if (tcgetattr(fd, &options) == -1) {
        fprintf(stderr, "set_port: tcgetattr failed - %s\n", strerror(errno));
        return SERIAL_ERROR_ATTR;
    }

    /* Set input and output baud rates */
    if (cfsetispeed(&options, baud_rate) == -1 || 
        cfsetospeed(&options, baud_rate) == -1) {
        fprintf(stderr, "set_port: invalid baud rate - %s\n", strerror(errno));
        return SERIAL_ERROR_BAUD;
    }

    /* Set raw mode - no control characters, just raw data */
    cfmakeraw(&options);

    /* Configure read timeout behavior:
     * VMIN=0: Return immediately with any data received
     * VTIME=15: Wait up to 1.5 seconds for data
     */
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 15;

    /* Apply the new settings */
    if (tcsetattr(fd, TCSAFLUSH, &options) == -1) {
        fprintf(stderr, "set_port: tcsetattr failed - %s\n", strerror(errno));
        return SERIAL_ERROR_CONFIG;
    }

//    fprintf(stderr, "Serial port configured at %d baud\n", baud);
    return SERIAL_SUCCESS;
}
