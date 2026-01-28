/*
 * Help and usage display functions
 * V1.0/2025-04-17
 */
#include <stdio.h>
#include "help_functions.h"
#include "revision.h"

/* External global variables */
extern char *progname;

/* Local define from main.c */
#define PORT "/dev/ttyUSB0"  /* Linux */

/*
 *  Help
 */
void help(void)
{
    static char *msg[] = {
        "-h or -?\tHelp",
        "-p [name]\tSelect port (default: "PORT")",
        "-a [address]\tSelect address (default: '01H')",
        "-b [n]\t\tSet baud rate on serial port {1200, 2400, 4800, 9600, 19200}, def. 9600",
        "-t [time]\tTime step [s], (default 1 s)",
        "-n [num]\tNumber of channels to read (1-8), def. 1",
        "-c\t\tRead correction temperature [C]",
        "-w [address]\tWrite new device address (1..254)",
        "-x [n]\t\tSet baud rate on R4DCB08 device {0:1200, 1:2400, 2:4800, 3:9600, 4:19200}",
        "-s [ch,Tc]\tSet temperature correction Tc for channel ch",
        "-m\t\tEnable three point median filter",
        "-M [n]\t\tEnable MAF filter with window size n (odd, 3-15)",
        "-f\t\tEnable one shot measure without timestamp",
        "-r\t\tFactory reset (resets address to 1, baudrate to 9600, corrections to 0)",
        "-S\t\tScan RS485 bus for devices (addresses 1-254)",
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
void usage(void)
{
    fprintf(stderr, "usage: %s [OPTIONS]\nFor help, type: '%s -h or -?'\n", progname, progname);
}
