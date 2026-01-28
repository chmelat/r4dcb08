/*
 * Temperature reading functions
 * V1.0/2025-04-17
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "read_functions.h"
#include "error.h"
#include "monada.h"
#include "median_filter.h"
#include "now.h"
#include "typedef.h"
#include "define_error_resp.h"
#include "signal_handler.h"
#include "constants.h"


/**
 * Read and print correction temperature for all channels
 */
AppStatus read_correction(int fd, uint8_t adr)
{
    int verb = 0;
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int i;
    float T;
    AppStatus status;

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

    status = monada(fd, adr, '\x03', 4, input_data, p_pr, verb, "read_correction", 0, &p_data);
    if (status != STATUS_OK) {
        return ERROR_READ_CORRECTION;
    }

    for (i=0; i<MAX_CHANNELS; i++) {
        T = (float)INT16(p_data[2*i+1], p_data[2*i])/10; /* Temperature [C] */
        printf(" %.1f", T);
    }
    printf("\n\n");
    
    return STATUS_OK;
}

/**
 * Read and print temperature from 1..n channels
 */
AppStatus read_temp(int fd, uint8_t adr, int n, int dt, int m_f, int one_shot)
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
    AppStatus status;

    /* Input validation */
    if (n < 1 || n > MAX_CHANNELS) {
        return ERROR_INVALID_CHANNEL;
    }

    if (dt < 0) {
        return ERROR_INVALID_TIME;
    }

    /* Set up signal handlers for clean termination */
    init_signal_handlers();

    /* Register address (2 byte) + Read number (2 byte) */       
    input_data[0] = 0x00; 
    input_data[1] = 0x00; 
    input_data[2] = 0x00; 
    input_data[3] = n;

    if (!one_shot) {
      printf("# Date                ");
      for (i=1; i<=n; i++) {
        printf("  Ch%d",i);
      }  
      printf("\n");
    }

    /* Modified loop to allow termination with Ctrl+C */
    while (running) {
        status = monada(fd, adr, '\x03', 4, input_data, p_pr, verb, "read_temp", 0, &p_data);
        if (status != STATUS_OK) {
            return ERROR_READ_TEMPERATURE;
        }

        sample_time = now();
        if (sample_time == NULL) {
            fprintf(stderr, "read_temp: Failed to get current time\n");
            return ERROR_READ_TEMPERATURE;
        }

        for (i=0; i<n; i++) {
            T[i] = (float)INT16(p_data[2*i+1], p_data[2*i])/10; /* Temperature [C] */
            if (T[i] < MIN_TEMPERATURE || T[i] > MAX_TEMPERATURE)
              T[i] = ERRRESP;
        }

        if (m_f) {
          rc = median_filter(sample_time, n, T, sample_t_f,T_f);
          if (rc != MF_SUCCESS) {
            fprintf(stderr, "Median filter failed with code %d\n", rc);
            return ERROR_MEDIAN_FILTER;
          }
          strncpy(sample_time, sample_t_f, DBUF - 1);
          sample_time[DBUF - 1] = '\0';
          for (i=0; i<n; i++) {
            T[i] = T_f[i];
          }
        }

        if (!one_shot) {
          printf("%s ", sample_time);
        }

        for (i=0; i<n; i++) {
          if (T[i] != ERRRESP) 
            printf(" %.1f", T[i]);
          else
            printf("  NaN");
        }
        printf("\n");
        
        if (!one_shot && dt > 0) {
          struct timespec ts = { .tv_sec = dt, .tv_nsec = 0 };
          nanosleep(&ts, NULL);
        }
        if (one_shot) {
          break;
        }
    }
    if (!one_shot) {
      int sig = get_received_signal();
      if (sig == SIGINT) {
          printf("\nReceived SIGINT (Ctrl+C), measurement stopped\n");
      } else if (sig == SIGTERM) {
          printf("\nReceived SIGTERM, measurement stopped\n");
      } else {
          printf("\nMeasurement stopped\n");
      }
    }
    return STATUS_OK;
}
