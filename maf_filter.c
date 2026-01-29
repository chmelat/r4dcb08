/*
 *  Moving Average Filter (MAF) with trapezoidal weights
 *  Centered trapezoidal weighted moving average on odd window size
 *  Weights: [0.5, 1, 1, ..., 1, 0.5]
 *  Formula: MAF = (0.5*x[0] + x[1] + ... + x[n-2] + 0.5*x[n-1]) / (n-1)
 *  V0.1/2025-01-28
 */

#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String operations */
#include <stdlib.h>  /* For NULL definition */

#include "now.h"            /* Define DBUF */
#include "define_error_resp.h"
#include "maf_filter.h"
#include "constants.h"

/* Static state for the filter */
static int window_size = 0;          /* Current window size (0 = not initialized) */
static int buffer_index = 0;         /* Current position in circular buffer */
static int samples_count = 0;        /* Number of samples collected */
static float val_buffer[MAF_MAX_WINDOW][MAX_CHANNELS];
static char s_buffer[MAF_MAX_WINDOW][DBUF];

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
 * Initialize the MAF filter
 */
int maf_init(int win_size)
{
    /* Validate window size */
    if (win_size < MAF_MIN_WINDOW || win_size > MAF_MAX_WINDOW) {
        fprintf(stderr, "maf_init: Window size %d is not %d..%d\n",
                win_size, MAF_MIN_WINDOW, MAF_MAX_WINDOW);
        return MAF_ERR_WINDOW;
    }

    /* Window size must be odd */
    if (win_size % 2 == 0) {
        fprintf(stderr, "maf_init: Window size %d must be odd\n", win_size);
        return MAF_ERR_WINDOW;
    }

    window_size = win_size;
    buffer_index = 0;
    samples_count = 0;

    return MAF_SUCCESS;
}

/*
 * Reset the MAF filter state
 */
void maf_reset(void)
{
    window_size = 0;
    buffer_index = 0;
    samples_count = 0;
}

/*
 * Get current window size
 */
int maf_get_window_size(void)
{
    return window_size;
}

/*
 * Apply trapezoidal weighted moving average filter
 */
int maf_filter(char *sample, int nch, const float val[],
               char *sample_filtered, float val_filtered[])
{
    int i, m;
    int center_idx;
    float sum;
    float weight;

    /* Check if initialized */
    if (window_size == 0) {
        fprintf(stderr, "maf_filter: Filter not initialized\n");
        return MAF_ERR_WINDOW;
    }

    /* Validate inputs */
    if (sample == NULL || val == NULL || sample_filtered == NULL || val_filtered == NULL) {
        fprintf(stderr, "maf_filter: NULL pointer provided\n");
        return MAF_ERR_PARAM;
    }

    if (nch <= 0 || nch > MAX_CHANNELS) {
        fprintf(stderr, "maf_filter: Invalid channel count: %d\n", nch);
        return MAF_ERR_RANGE;
    }

    /* Store current sample in circular buffer */
    strncpy(s_buffer[buffer_index], sample, DBUF - 1);
    s_buffer[buffer_index][DBUF - 1] = '\0';

    for (m = 0; m < nch; m++) {
        val_buffer[buffer_index][m] = val[m];
    }

    /* Increment samples count (up to window_size) */
    if (samples_count < window_size) {
        samples_count++;
    }

    /* Calculate center index for timestamp */
    /* Center is at (window_size - 1) / 2 positions back */
    center_idx = mod(buffer_index - (window_size - 1) / 2, window_size);

    /* Copy timestamp from center sample */
    strncpy(sample_filtered, s_buffer[center_idx], DBUF - 1);
    sample_filtered[DBUF - 1] = '\0';

    /* Process each channel */
    for (m = 0; m < nch; m++) {
        float total_weight = 0.0f;
        sum = 0.0f;

        /* If we don't have enough samples yet, use simple average */
        if (samples_count < window_size) {
            for (i = 0; i < samples_count; i++) {
                if (val_buffer[i][m] != ERRRESP) {
                    sum += val_buffer[i][m];
                    total_weight += 1.0f;
                }
            }
            if (total_weight > 0.0f) {
                val_filtered[m] = sum / total_weight;
            } else {
                val_filtered[m] = ERRRESP;
            }
            continue;
        }

        /* Full window - apply trapezoidal weighted average */
        for (i = 0; i < window_size; i++) {
            int idx = mod(buffer_index - window_size + 1 + i, window_size);

            if (val_buffer[idx][m] == ERRRESP) {
                continue;
            }

            /* Trapezoidal weights: 0.5 at edges, 1.0 in the middle */
            weight = (i == 0 || i == window_size - 1) ? 0.5f : 1.0f;

            sum += weight * val_buffer[idx][m];
            total_weight += weight;
        }

        if (total_weight > 0.0f) {
            val_filtered[m] = sum / total_weight;
        } else {
            val_filtered[m] = ERRRESP;
        }
    }

    /* Advance buffer index */
    buffer_index = mod(buffer_index + 1, window_size);

    return MAF_SUCCESS;
}
