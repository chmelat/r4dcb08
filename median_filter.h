/*
 *  Three-point median filter header
 *  V0.0/2023-02-01
 *  V0.1/2024-11-25 add ERRRESP
 *  V0.2/2025-03-09 enhanced documentation and safety
 *  V0.3/2025-03-12 changed to return status code
 */

#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

/* Return codes */
#define MF_SUCCESS      0   /* Operation completed successfully */
#define MF_ERR_PARAM   -1   /* Invalid parameter */
#define MF_ERR_RANGE   -2   /* Value out of range */

/*
 * Apply a three-point median filter to a series of values
 *
 * The filter keeps the last three values for each channel and outputs
 * the median value, providing effective spike removal while preserving
 * trends in the data.
 *
 * Parameters:
 *   sample          - Input timestamp or identifier string
 *   nch             - Number of channels to process
 *   val             - Array of input values for each channel
 *   sample_filtered - Output timestamp (from the middle sample in the window)
 *   val_filtered    - Array of filtered output values
 *
 * Return value:
 *   MF_SUCCESS      - Filter applied successfully
 *   MF_ERR_PARAM    - NULL pointers or other invalid parameters
 *   MF_ERR_RANGE    - Channel count out of valid range
 *
 * Note: This function maintains state between calls.
 *       ERRRESP values are handled specially.
 */
extern int median_filter(char *sample, int nch, const float val[], 
                        char *sample_filtered, float val_filtered[]);

#endif /* MEDIAN_FILTER_H */
