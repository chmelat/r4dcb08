/*
 *  Date and time in ISO 8601 format
 */
#ifndef NOW_H
#define NOW_H

/**
 * Maximum length of timestamp buffer
 * Must accommodate "YYYY-MM-DD HH:MM:SS.CC" plus null terminator
 */
#define DBUF 64

/**
 * Returns current date and time in ISO 8601 format (YYYY-MM-DD HH:MM:SS.CC)
 * 
 * WARNING: This function is not thread-safe as it returns a pointer
 * to a static buffer. Copy the result if needed for later use.
 *
 * @return Pointer to a static buffer containing formatted timestamp,
 *         or NULL if an error occurred
 */
extern char *now(void);

/**
 * Thread-safe version that writes to a caller-provided buffer
 *
 * @param buffer     Output buffer for timestamp
 * @param buffer_len Size of the output buffer
 * @return           0 on success, -1 on error
 */
extern int now_r(char *buffer, size_t buffer_len);

#endif /* NOW_H */
