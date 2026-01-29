/*
 * MQTT daemon configuration
 * V1.0/2026-01-29
 */
#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <stdint.h>
#include "mqtt_error.h"

/* Default values */
#define MQTT_DEFAULT_PORT "/dev/ttyUSB0"
#define MQTT_DEFAULT_ADDRESS 1
#define MQTT_DEFAULT_BAUDRATE 9600
#define MQTT_DEFAULT_CHANNELS 8
#define MQTT_DEFAULT_HOST "localhost"
#define MQTT_DEFAULT_MQTT_PORT 1883
#define MQTT_DEFAULT_MQTT_PORT_TLS 8883
#define MQTT_DEFAULT_TOPIC "sensors/r4dcb08"
#define MQTT_DEFAULT_INTERVAL 10
#define MQTT_DEFAULT_QOS 1
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_DEFAULT_DIAGNOSTICS_INTERVAL 6

/* Environment variable for password */
#define MQTT_PASSWORD_ENV "MQTT_PASSWORD"

/* Maximum string lengths */
#define MQTT_MAX_PATH 256
#define MQTT_MAX_HOST 128
#define MQTT_MAX_TOPIC 256
#define MQTT_MAX_CRED 128
#define MQTT_MAX_CLIENT_ID 64

/* Configuration structure */
typedef struct {
    /* Serial port settings */
    char serial_port[MQTT_MAX_PATH];
    uint8_t device_address;
    int baudrate;
    int num_channels;

    /* MQTT settings */
    char mqtt_host[MQTT_MAX_HOST];
    int mqtt_port;
    char mqtt_user[MQTT_MAX_CRED];
    char mqtt_pass[MQTT_MAX_CRED];
    char topic_prefix[MQTT_MAX_TOPIC];
    char client_id[MQTT_MAX_CLIENT_ID];
    int qos;
    int retain;
    int keepalive;

    /* Daemon settings */
    int interval;
    int daemon_mode;
    int verbose;
    char config_file[MQTT_MAX_PATH];
    char pid_file[MQTT_MAX_PATH];

    /* Filter settings */
    int enable_median_filter;
    int enable_maf_filter;
    int maf_window_size;

    /* TLS settings */
    int use_tls;
    char tls_ca_file[MQTT_MAX_PATH];
    char tls_cert_file[MQTT_MAX_PATH];
    char tls_key_file[MQTT_MAX_PATH];
    int tls_insecure;  /* Skip certificate verification (testing only) */

    /* Password file (more secure than command line) */
    char password_file[MQTT_MAX_PATH];

    /* Diagnostics settings */
    int diagnostics_interval;  /* Publish diagnostics every N intervals (0=disable) */
} MqttConfig;

/**
 * Initialize configuration with default values
 *
 * @param config Pointer to configuration structure
 */
void mqtt_config_init(MqttConfig *config);

/**
 * Parse command line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param config Pointer to configuration structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_config_parse_args(int argc, char *argv[], MqttConfig *config);

/**
 * Parse configuration file (INI format)
 *
 * @param filename Path to configuration file
 * @param config Pointer to configuration structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_config_parse_file(const char *filename, MqttConfig *config);

/**
 * Validate configuration values
 *
 * @param config Pointer to configuration structure
 * @return MQTT_OK if valid, error code on invalid values
 */
MqttStatus mqtt_config_validate(const MqttConfig *config);

/**
 * Print configuration (for debugging)
 *
 * @param config Pointer to configuration structure
 */
void mqtt_config_print(const MqttConfig *config);

/**
 * Print usage information
 *
 * @param program_name Name of the program
 */
void mqtt_config_usage(const char *program_name);

/**
 * Load password from file or environment variable
 * Priority: 1) password_file, 2) MQTT_PASSWORD env, 3) config file value
 *
 * @param config Pointer to configuration structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_config_load_password(MqttConfig *config);

/**
 * Clear sensitive data from configuration (password)
 * Call after credentials have been passed to MQTT library
 *
 * @param config Pointer to configuration structure
 */
void mqtt_config_clear_sensitive(MqttConfig *config);

/**
 * Safe string to integer conversion with validation
 *
 * @param str String to convert
 * @param result Pointer to store result
 * @param min Minimum valid value
 * @param max Maximum valid value
 * @return 0 on success, -1 on error
 */
int mqtt_config_parse_int(const char *str, int *result, int min, int max);

#endif /* MQTT_CONFIG_H */
