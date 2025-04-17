/*
 *  V1.5/2025-04-17
 *  Temperature sensor module R4DCB08 communication utility
 */

#include <stdlib.h>  /* Standard lib */
#include <stdio.h>   /* Standard input/output definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <stdint.h>  /* Integer types */
#include <time.h>    /* Time function */
#include <libgen.h>  /* Required for basename in newer C versions */
#include <string.h>

#include "typedef.h"
#include "serial.h"
#include "now.h"
#include "packet.h"
#include "monada.h"
#include "revision.h"
#include "define_error_resp.h"
#include "median_filter.h"
#include "error.h"
#include "config.h"
#include "signal_handler.h"

/* Global variables */
char *progname;

/* Function declarations moved to header files */

int main(int argc, char *argv[])
{
    ProgramConfig config;
    AppStatus status;
    
    /* Initialize configuration */
    init_config(&config);
    
    /* Initialize signal handlers */
    init_signal_handlers();
    
    /* Process command line arguments */
    status = parse_arguments(argc, argv, &config);
    handle_error(status, "parse_arguments", 1);
    
    /* Execute command according to configuration */
    status = execute_command(&config);
    handle_error(status, "execute_command", 1);
    
    return 0;
}