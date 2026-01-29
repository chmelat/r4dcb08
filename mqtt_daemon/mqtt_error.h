/*
 * MQTT daemon error handling and logging
 * V1.0/2026-01-29
 */
#ifndef MQTT_ERROR_H
#define MQTT_ERROR_H

#include <syslog.h>

/* MQTT daemon specific error codes */
typedef enum {
    MQTT_OK = 0,

    /* Configuration errors */
    MQTT_ERR_CONFIG_FILE = -100,
    MQTT_ERR_CONFIG_PARSE = -101,
    MQTT_ERR_CONFIG_VALUE = -102,

    /* Connection errors */
    MQTT_ERR_CONNECT = -110,
    MQTT_ERR_DISCONNECT = -111,
    MQTT_ERR_RECONNECT = -112,

    /* Publishing errors */
    MQTT_ERR_PUBLISH = -120,
    MQTT_ERR_TOPIC = -121,

    /* Serial/Modbus errors */
    MQTT_ERR_SERIAL = -130,
    MQTT_ERR_MODBUS = -131,
    MQTT_ERR_READ_TEMP = -132,

    /* Daemon errors */
    MQTT_ERR_DAEMON = -140,
    MQTT_ERR_FORK = -141,
    MQTT_ERR_PID_FILE = -142,

    /* Library errors */
    MQTT_ERR_MOSQUITTO_INIT = -150,
    MQTT_ERR_MOSQUITTO_LIB = -151
} MqttStatus;

/**
 * Initialize logging
 * Opens syslog for daemon mode or stderr for foreground
 *
 * @param daemon_mode 1 for syslog, 0 for stderr
 * @param program_name Name for syslog identification
 */
void mqtt_log_init(int daemon_mode, const char *program_name);

/**
 * Close logging
 */
void mqtt_log_close(void);

/**
 * Log an informational message
 *
 * @param format printf-style format string
 */
void mqtt_log_info(const char *format, ...);

/**
 * Log a warning message
 *
 * @param format printf-style format string
 */
void mqtt_log_warning(const char *format, ...);

/**
 * Log an error message
 *
 * @param format printf-style format string
 */
void mqtt_log_error(const char *format, ...);

/**
 * Log a debug message (only if verbose mode is enabled)
 *
 * @param format printf-style format string
 */
void mqtt_log_debug(const char *format, ...);

/**
 * Set verbose/debug mode
 *
 * @param verbose 1 to enable debug messages, 0 to disable
 */
void mqtt_log_set_verbose(int verbose);

/**
 * Get text description of MQTT status code
 *
 * @param status Status code
 * @return Text description
 */
const char *mqtt_status_str(MqttStatus status);

#endif /* MQTT_ERROR_H */
