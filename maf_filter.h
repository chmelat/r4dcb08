/*
 *  Moving Average Filter (MAF) with trapezoidal weights header
 *  Centered trapezoidal weighted moving average on odd window size
 *  V0.1/2025-01-28
 */

#ifndef MAF_FILTER_H
#define MAF_FILTER_H

/* Return codes */
#define MAF_SUCCESS      0   /* Operation completed successfully */
#define MAF_ERR_PARAM   -1   /* Invalid parameter (NULL pointer) */
#define MAF_ERR_RANGE   -2   /* Value out of range */
#define MAF_ERR_WINDOW  -3   /* Invalid window size */

/* Window size constraints */
#define MAF_MIN_WINDOW      3   /* Minimum window size */
#define MAF_MAX_WINDOW     15   /* Maximum window size */
#define MAF_DEFAULT_WINDOW  5   /* Default window size */

/**
 * Initialize the MAF filter with specified window size
 *
 * @param window_size Size of the filter window (must be odd, 3-15)
 * @return MAF_SUCCESS on success, MAF_ERR_WINDOW if invalid window size
 */
int maf_init(int window_size);

/**
 * Reset the MAF filter state
 *
 * Clears the internal buffer and resets to initial state.
 * Must call maf_init() again before using the filter.
 */
void maf_reset(void);

/**
 * Apply trapezoidal weighted moving average filter to a series of values
 *
 * The filter uses trapezoidal weights: [0.5, 1, 1, ..., 1, 0.5]
 * Formula: MAF = (0.5*x[0] + x[1] + ... + x[n-2] + 0.5*x[n-1]) / (n-1)
 *
 * Parameters:
 *   sample          - Input timestamp or identifier string
 *   nch             - Number of channels to process
 *   val             - Array of input values for each channel
 *   sample_filtered - Output timestamp (from the middle sample in the window)
 *   val_filtered    - Array of filtered output values
 *
 * Return value:
 *   MAF_SUCCESS     - Filter applied successfully
 *   MAF_ERR_PARAM   - NULL pointers or other invalid parameters
 *   MAF_ERR_RANGE   - Channel count out of valid range
 *   MAF_ERR_WINDOW  - Filter not initialized
 *
 * Note: This function maintains state between calls.
 *       Call maf_init() before first use.
 *       ERRRESP values are handled specially.
 */
int maf_filter(char *sample, int nch, const float val[],
               char *sample_filtered, float val_filtered[]);

/**
 * Get current window size
 *
 * @return Current window size, or 0 if not initialized
 */
int maf_get_window_size(void);

#endif /* MAF_FILTER_H */
