/*
 *  Three-point median filter
 *  Z posloupnosti  (........, a_k, a_j, a_i) je vystupem filtru hodnota m_j = median(a_k, a_j, a_i)
 *  V0.0/2023-02-01
 *  V0.1/2024-11-25 add ERRRESP
 *  V0.2/2025-03-09 enhanced safety and performance (AI)
 *  V0.3/2025-03-12 changed to return status code
 */

#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String operations */
#include <stdlib.h>  /* For NULL definition */
#include <assert.h>  /* For assertions */

#include "now.h"            /* Define DBUF */
#include "define_error_resp.h"
#include "median_filter.h"
#include "constants.h"

/* Constants */
#define MF_WINDOW_SIZE 3     /* Size of the sliding window */

/*
 *  Declare local functions
 */
static int mod(int a, int b);
static inline double median(float a, float b, float c);

/*
 *  Apply three-point median filter
 */
int median_filter(char *sample, int nch, const float val[], 
                 char *sample_filtered, float val_filtered[])
{
    /* Static state for the filter */
    static int i = 0;
    static float val_vec[MF_WINDOW_SIZE][MAX_CHANNELS];
    static char s_vec[MF_WINDOW_SIZE][DBUF];
    static int start = 1;
    
    int j, k, m;
    
    /* Validate inputs */
    if (sample == NULL || val == NULL || sample_filtered == NULL || val_filtered == NULL) {
        fprintf(stderr, "median_filter: NULL pointer provided\n");
        return MF_ERR_PARAM;
    }
    
    if (nch <= 0 || nch > MAX_CHANNELS) {
        fprintf(stderr, "median_filter: Invalid channel count: %d\n", nch);
        return MF_ERR_RANGE;
    }
    
    /* Calculate indices for the circular buffer */
    i = mod(++i, MF_WINDOW_SIZE);  /* Actual index */
    j = mod(i-1, MF_WINDOW_SIZE);  /* Prev. index */
    k = mod(i-2, MF_WINDOW_SIZE);  /* Prev. prev. index */
    
    /* Initialize the filter with the first value */
    if (start) {
        for (m = 0; m < nch; m++) {
            val_vec[0][m] = val[m];
            val_vec[1][m] = val[m];
            val_vec[2][m] = val[m];
        }
        
        /* Safe string copy with bound checking */
        strncpy(s_vec[0], sample, DBUF-1);
        s_vec[0][DBUF-1] = '\0';
        
        strncpy(s_vec[1], sample, DBUF-1);
        s_vec[1][DBUF-1] = '\0';
        
        strncpy(s_vec[2], sample, DBUF-1);
        s_vec[2][DBUF-1] = '\0';
        
        start = 0;
    }
    
    /* Safe string copy for current sample */
    strncpy(s_vec[i], sample, DBUF-1);
    s_vec[i][DBUF-1] = '\0';
    
    /* Copy the timestamp from the middle sample */
    strncpy(sample_filtered, s_vec[j], DBUF-1);
    sample_filtered[DBUF-1] = '\0';
    
    /* Process each channel */
    for (m = 0; m < nch; m++) {
        /* Store the current value */
        val_vec[i][m] = val[m];
        
        /* Apply the median filter */
        if (val_vec[i][m] == ERRRESP) {
            /* Pass through error values */
            val_filtered[m] = ERRRESP;
        } else if ((val_vec[j][m] == ERRRESP) || (val_vec[k][m] == ERRRESP)) {
            /* If any value in the window is an error, use current value */
            val_filtered[m] = val[m];
        } else {
            /* Calculate median of three values */
            val_filtered[m] = median(val_vec[k][m], val_vec[j][m], val_vec[i][m]);
        }
    }
    
    return MF_SUCCESS;
}

/*
 *  Operator modulo to properly handle negative values:
 *  a mod n = a - n(floor(a/n))
 */
static int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

/*
 *  Median function - finds median of three values
 *  Optimized for three values using direct comparisons
 */
static inline double median(float a, float b, float c)
{
    if (a > b) {
        /* Swap a and b */
        float temp = a;
        a = b;
        b = temp;
    }
    
    /* At this point, a <= b */
    if (b > c) {
        /* Now we need to find middle value between a, b, and c */
        /* We know b > c, so either a or c is the median */
        return (a > c) ? a : c;
    } else {
        /* We know a <= b <= c, so b is the median */
        return b;
    }
}
