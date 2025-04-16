/*
 *  Current date and time in ISO 8601 format
 */
#include <stdlib.h>   /* Standard lib */
#include <stdio.h>    /* Standard input/output definitions */
#include <time.h>     /* Time function */
#include <sys/time.h> /* Time function */
#include <string.h>   /* String functions */
#include <errno.h>    /* Error numbers and messages */

#include "now.h" /* DBUF definition */

/**
 * Returns current date and time in ISO 8601 format.
 * Not thread-safe due to static buffer usage.
 *
 * @return Pointer to static buffer with timestamp or NULL on error
 */
char *now(void)
{
    struct tm ts_buf;        /* Time structure */
    struct tm *ts = &ts_buf; /* Pointer to time structure */
    static char buf[DBUF];   /* Text variable for timestamp */
    struct timeval now_val;  /* Argument for gettimeofday */
    size_t len;              /* Length of formatted string */

    /* Clear buffer to ensure null termination */
    memset(buf, 0, sizeof(buf));

    /* Get current time with error handling */
    if (gettimeofday(&now_val, NULL) == -1) {
        fprintf(stderr, "now: Error calling gettimeofday: %s\n", strerror(errno));
        return NULL;
    }

    /* Convert seconds to time structure with thread-safe function */
    ts = localtime_r(&now_val.tv_sec, &ts_buf);
    if (ts == NULL) {
        fprintf(stderr, "now: Error in localtime_r: %s\n", strerror(errno));
        return NULL;
    }

    /* Format time with error checking */
    len = strftime(buf, sizeof(buf) - 5, "%Y-%m-%d %H:%M:%S", ts);
    if (len == 0 || len >= sizeof(buf) - 5) {
        fprintf(stderr, "now: Error formatting time, buffer too small or format error\n");
        return NULL;
    }

    /* Add centiseconds with boundary checking */
    unsigned int centisec = (unsigned int)(now_val.tv_usec / 10000);
    int result = snprintf(buf + len, sizeof(buf) - len, ".%02u", centisec);
    
    if (result < 0 || result >= (int)(sizeof(buf) - len)) {
        fprintf(stderr, "now: Error adding centiseconds, buffer too small\n");
        return NULL;
    }

    return buf;
}
