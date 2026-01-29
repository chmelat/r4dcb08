/*
 * MQTT daemon configuration
 * V1.0/2026-01-29
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include "mqtt_config.h"
#include "mqtt_error.h"

/* Long options for getopt */
static struct option long_options[] = {
    {"port",          required_argument, 0, 'p'},
    {"address",       required_argument, 0, 'a'},
    {"baudrate",      required_argument, 0, 'b'},
    {"channels",      required_argument, 0, 'n'},
    {"mqtt-host",     required_argument, 0, 'H'},
    {"mqtt-port",     required_argument, 0, 'P'},
    {"mqtt-user",     required_argument, 0, 'u'},
    {"mqtt-pass",     required_argument, 0, 'w'},
    {"topic",         required_argument, 0, 't'},
    {"client-id",     required_argument, 0, 'i'},
    {"interval",      required_argument, 0, 'I'},
    {"config",        required_argument, 0, 'c'},
    {"daemon",        no_argument,       0, 'd'},
    {"verbose",       no_argument,       0, 'v'},
    {"median-filter", no_argument,       0, 'm'},
    {"maf-filter",    required_argument, 0, 'M'},
    {"help",          no_argument,       0, 'h'},
    {"version",       no_argument,       0, 'V'},
    {0, 0, 0, 0}
};

void mqtt_config_init(MqttConfig *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(MqttConfig));

    /* Serial defaults */
    strncpy(config->serial_port, MQTT_DEFAULT_PORT, MQTT_MAX_PATH - 1);
    config->device_address = MQTT_DEFAULT_ADDRESS;
    config->baudrate = MQTT_DEFAULT_BAUDRATE;
    config->num_channels = MQTT_DEFAULT_CHANNELS;

    /* MQTT defaults */
    strncpy(config->mqtt_host, MQTT_DEFAULT_HOST, MQTT_MAX_HOST - 1);
    config->mqtt_port = MQTT_DEFAULT_MQTT_PORT;
    strncpy(config->topic_prefix, MQTT_DEFAULT_TOPIC, MQTT_MAX_TOPIC - 1);
    config->qos = MQTT_DEFAULT_QOS;
    config->retain = 1;
    config->keepalive = MQTT_DEFAULT_KEEPALIVE;

    /* Generate default client ID */
    snprintf(config->client_id, MQTT_MAX_CLIENT_ID, "r4dcb08-mqtt-%d", getpid());

    /* Daemon defaults */
    config->interval = MQTT_DEFAULT_INTERVAL;
    config->daemon_mode = 0;
    config->verbose = 0;

    /* Filter defaults */
    config->enable_median_filter = 0;
    config->enable_maf_filter = 0;
    config->maf_window_size = 5;
}

static char *trim(char *str)
{
    char *end;

    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) {
        return str;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    /* Write new null terminator */
    end[1] = '\0';

    return str;
}

MqttStatus mqtt_config_parse_file(const char *filename, MqttConfig *config)
{
    FILE *fp;
    char line[512];
    char *key, *value;
    int line_num = 0;

    if (filename == NULL || config == NULL) {
        return MQTT_ERR_CONFIG_FILE;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        mqtt_log_error("Cannot open config file: %s", filename);
        return MQTT_ERR_CONFIG_FILE;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_num++;

        /* Remove newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* Skip comments and empty lines */
        char *trimmed = trim(line);
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
            continue;
        }

        /* Skip section headers */
        if (*trimmed == '[') {
            continue;
        }

        /* Find '=' separator */
        value = strchr(trimmed, '=');
        if (value == NULL) {
            mqtt_log_warning("Config line %d: missing '='", line_num);
            continue;
        }

        *value = '\0';
        value++;

        key = trim(trimmed);
        value = trim(value);

        /* Remove quotes from value */
        size_t len = strlen(value);
        if (len >= 2 && ((value[0] == '"' && value[len-1] == '"') ||
                         (value[0] == '\'' && value[len-1] == '\''))) {
            value[len-1] = '\0';
            value++;
        }

        /* Parse key-value pairs */
        if (strcmp(key, "port") == 0 || strcmp(key, "serial_port") == 0) {
            strncpy(config->serial_port, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "address") == 0 || strcmp(key, "device_address") == 0) {
            config->device_address = (uint8_t)atoi(value);
        } else if (strcmp(key, "baudrate") == 0) {
            config->baudrate = atoi(value);
        } else if (strcmp(key, "channels") == 0 || strcmp(key, "num_channels") == 0) {
            config->num_channels = atoi(value);
        } else if (strcmp(key, "mqtt_host") == 0 || strcmp(key, "host") == 0) {
            strncpy(config->mqtt_host, value, MQTT_MAX_HOST - 1);
        } else if (strcmp(key, "mqtt_port") == 0) {
            config->mqtt_port = atoi(value);
        } else if (strcmp(key, "mqtt_user") == 0 || strcmp(key, "user") == 0 ||
                   strcmp(key, "username") == 0) {
            strncpy(config->mqtt_user, value, MQTT_MAX_CRED - 1);
        } else if (strcmp(key, "mqtt_pass") == 0 || strcmp(key, "pass") == 0 ||
                   strcmp(key, "password") == 0) {
            strncpy(config->mqtt_pass, value, MQTT_MAX_CRED - 1);
        } else if (strcmp(key, "topic") == 0 || strcmp(key, "topic_prefix") == 0) {
            strncpy(config->topic_prefix, value, MQTT_MAX_TOPIC - 1);
        } else if (strcmp(key, "client_id") == 0) {
            strncpy(config->client_id, value, MQTT_MAX_CLIENT_ID - 1);
        } else if (strcmp(key, "qos") == 0) {
            config->qos = atoi(value);
        } else if (strcmp(key, "retain") == 0) {
            config->retain = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                             strcmp(value, "yes") == 0);
        } else if (strcmp(key, "keepalive") == 0) {
            config->keepalive = atoi(value);
        } else if (strcmp(key, "interval") == 0) {
            config->interval = atoi(value);
        } else if (strcmp(key, "median_filter") == 0) {
            config->enable_median_filter = (strcmp(value, "true") == 0 ||
                                           strcmp(value, "1") == 0 ||
                                           strcmp(value, "yes") == 0);
        } else if (strcmp(key, "maf_filter") == 0) {
            config->enable_maf_filter = (strcmp(value, "true") == 0 ||
                                        strcmp(value, "1") == 0 ||
                                        strcmp(value, "yes") == 0);
        } else if (strcmp(key, "maf_window") == 0 || strcmp(key, "maf_window_size") == 0) {
            config->maf_window_size = atoi(value);
            if (config->maf_window_size > 0) {
                config->enable_maf_filter = 1;
            }
        } else if (strcmp(key, "verbose") == 0) {
            config->verbose = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                              strcmp(value, "yes") == 0);
        } else {
            mqtt_log_warning("Config line %d: unknown key '%s'", line_num, key);
        }
    }

    fclose(fp);
    return MQTT_OK;
}

MqttStatus mqtt_config_parse_args(int argc, char *argv[], MqttConfig *config)
{
    int opt;
    int option_index = 0;

    if (config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    while ((opt = getopt_long(argc, argv, "p:a:b:n:H:P:u:w:t:i:I:c:dvmM:hV",
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                strncpy(config->serial_port, optarg, MQTT_MAX_PATH - 1);
                break;
            case 'a':
                config->device_address = (uint8_t)atoi(optarg);
                break;
            case 'b':
                config->baudrate = atoi(optarg);
                break;
            case 'n':
                config->num_channels = atoi(optarg);
                break;
            case 'H':
                strncpy(config->mqtt_host, optarg, MQTT_MAX_HOST - 1);
                break;
            case 'P':
                config->mqtt_port = atoi(optarg);
                break;
            case 'u':
                strncpy(config->mqtt_user, optarg, MQTT_MAX_CRED - 1);
                break;
            case 'w':
                strncpy(config->mqtt_pass, optarg, MQTT_MAX_CRED - 1);
                break;
            case 't':
                strncpy(config->topic_prefix, optarg, MQTT_MAX_TOPIC - 1);
                break;
            case 'i':
                strncpy(config->client_id, optarg, MQTT_MAX_CLIENT_ID - 1);
                break;
            case 'I':
                config->interval = atoi(optarg);
                break;
            case 'c':
                strncpy(config->config_file, optarg, MQTT_MAX_PATH - 1);
                break;
            case 'd':
                config->daemon_mode = 1;
                break;
            case 'v':
                config->verbose = 1;
                break;
            case 'm':
                config->enable_median_filter = 1;
                break;
            case 'M':
                config->enable_maf_filter = 1;
                config->maf_window_size = atoi(optarg);
                break;
            case 'V':
                printf("r4dcb08-mqtt version 1.0\n");
                exit(0);
            case 'h':
            default:
                mqtt_config_usage(argv[0]);
                exit(opt == 'h' ? 0 : 1);
        }
    }

    return MQTT_OK;
}

MqttStatus mqtt_config_validate(const MqttConfig *config)
{
    if (config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate device address */
    if (config->device_address < 1 || config->device_address > 254) {
        mqtt_log_error("Invalid device address: %d (must be 1-254)",
                      config->device_address);
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate channel count */
    if (config->num_channels < 1 || config->num_channels > 8) {
        mqtt_log_error("Invalid channel count: %d (must be 1-8)",
                      config->num_channels);
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate interval */
    if (config->interval < 1) {
        mqtt_log_error("Invalid interval: %d (must be >= 1)", config->interval);
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate MQTT port */
    if (config->mqtt_port < 1 || config->mqtt_port > 65535) {
        mqtt_log_error("Invalid MQTT port: %d", config->mqtt_port);
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate QoS */
    if (config->qos < 0 || config->qos > 2) {
        mqtt_log_error("Invalid QoS: %d (must be 0-2)", config->qos);
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Validate MAF window size if enabled */
    if (config->enable_maf_filter) {
        if (config->maf_window_size < 3 || config->maf_window_size > 15 ||
            (config->maf_window_size % 2) == 0) {
            mqtt_log_error("Invalid MAF window size: %d (must be odd, 3-15)",
                          config->maf_window_size);
            return MQTT_ERR_CONFIG_VALUE;
        }
    }

    /* Validate baudrate */
    int valid_bauds[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 0};
    int valid = 0;
    for (int i = 0; valid_bauds[i] != 0; i++) {
        if (config->baudrate == valid_bauds[i]) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        mqtt_log_error("Invalid baudrate: %d", config->baudrate);
        return MQTT_ERR_CONFIG_VALUE;
    }

    return MQTT_OK;
}

void mqtt_config_print(const MqttConfig *config)
{
    if (config == NULL) {
        return;
    }

    mqtt_log_info("Configuration:");
    mqtt_log_info("  Serial port: %s", config->serial_port);
    mqtt_log_info("  Device address: %d", config->device_address);
    mqtt_log_info("  Baudrate: %d", config->baudrate);
    mqtt_log_info("  Channels: %d", config->num_channels);
    mqtt_log_info("  MQTT host: %s:%d", config->mqtt_host, config->mqtt_port);
    mqtt_log_info("  Topic prefix: %s", config->topic_prefix);
    mqtt_log_info("  Client ID: %s", config->client_id);
    mqtt_log_info("  Interval: %d s", config->interval);
    mqtt_log_info("  QoS: %d, Retain: %s", config->qos,
                 config->retain ? "yes" : "no");
    if (config->enable_median_filter) {
        mqtt_log_info("  Median filter: enabled");
    }
    if (config->enable_maf_filter) {
        mqtt_log_info("  MAF filter: enabled (window=%d)", config->maf_window_size);
    }
}

void mqtt_config_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("R4DCB08 temperature sensor MQTT publisher daemon\n\n");
    printf("Serial options:\n");
    printf("  -p, --port <device>      Serial port (default: %s)\n", MQTT_DEFAULT_PORT);
    printf("  -a, --address <addr>     Modbus address 1-254 (default: %d)\n", MQTT_DEFAULT_ADDRESS);
    printf("  -b, --baudrate <baud>    Baudrate (default: %d)\n", MQTT_DEFAULT_BAUDRATE);
    printf("  -n, --channels <num>     Number of channels 1-8 (default: %d)\n", MQTT_DEFAULT_CHANNELS);
    printf("\nMQTT options:\n");
    printf("  -H, --mqtt-host <host>   MQTT broker host (default: %s)\n", MQTT_DEFAULT_HOST);
    printf("  -P, --mqtt-port <port>   MQTT broker port (default: %d)\n", MQTT_DEFAULT_MQTT_PORT);
    printf("  -u, --mqtt-user <user>   MQTT username\n");
    printf("  -w, --mqtt-pass <pass>   MQTT password\n");
    printf("  -t, --topic <prefix>     Topic prefix (default: %s)\n", MQTT_DEFAULT_TOPIC);
    printf("  -i, --client-id <id>     MQTT client ID\n");
    printf("\nDaemon options:\n");
    printf("  -I, --interval <sec>     Measurement interval in seconds (default: %d)\n", MQTT_DEFAULT_INTERVAL);
    printf("  -c, --config <file>      Configuration file path\n");
    printf("  -d, --daemon             Run as daemon\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("\nFilter options:\n");
    printf("  -m, --median-filter      Enable median filter\n");
    printf("  -M, --maf-filter <size>  Enable MAF filter with window size (odd, 3-15)\n");
    printf("\nOther options:\n");
    printf("  -h, --help               Show this help\n");
    printf("  -V, --version            Show version\n");
    printf("\nExample:\n");
    printf("  %s -p /dev/ttyUSB0 -a 1 -H localhost -I 10\n", program_name);
    printf("  %s -c /etc/r4dcb08-mqtt.conf -d\n", program_name);
}
