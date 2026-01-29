/*
 * MQTT daemon configuration
 * V1.1/2026-01-29
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
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
    {"password-file", required_argument, 0, 'W'},
    {"topic",         required_argument, 0, 't'},
    {"client-id",     required_argument, 0, 'i'},
    {"interval",      required_argument, 0, 'I'},
    {"config",        required_argument, 0, 'c'},
    {"pid-file",      required_argument, 0, 'F'},
    {"daemon",        no_argument,       0, 'd'},
    {"verbose",       no_argument,       0, 'v'},
    {"median-filter", no_argument,       0, 'm'},
    {"maf-filter",    required_argument, 0, 'M'},
    {"tls",           no_argument,       0, 'S'},
    {"tls-ca",        required_argument, 0, 1001},
    {"tls-cert",      required_argument, 0, 1002},
    {"tls-key",       required_argument, 0, 1003},
    {"tls-insecure",  no_argument,       0, 1004},
    {"help",          no_argument,       0, 'h'},
    {"version",       no_argument,       0, 'V'},
    {0, 0, 0, 0}
};

/* Safe string to integer conversion */
int mqtt_config_parse_int(const char *str, int *result, int min, int max)
{
    char *endptr;
    long val;

    if (str == NULL || result == NULL) {
        return -1;
    }

    errno = 0;
    val = strtol(str, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
        return -1;
    }

    /* Check for no digits */
    if (endptr == str) {
        return -1;
    }

    /* Check for trailing non-whitespace */
    while (isspace((unsigned char)*endptr)) endptr++;
    if (*endptr != '\0') {
        return -1;
    }

    /* Check range */
    if (val < min || val > max) {
        return -1;
    }

    *result = (int)val;
    return 0;
}

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

    /* TLS defaults */
    config->use_tls = 0;
    config->tls_insecure = 0;

    /* Default PID file */
    strncpy(config->pid_file, "/var/run/r4dcb08-mqtt.pid", MQTT_MAX_PATH - 1);
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

        /* Helper macro for boolean parsing */
        #define PARSE_BOOL(v) (strcmp(v, "true") == 0 || strcmp(v, "1") == 0 || strcmp(v, "yes") == 0)

        /* Parse key-value pairs */
        if (strcmp(key, "port") == 0 || strcmp(key, "serial_port") == 0) {
            strncpy(config->serial_port, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "address") == 0 || strcmp(key, "device_address") == 0) {
            int addr;
            if (mqtt_config_parse_int(value, &addr, 1, 254) == 0) {
                config->device_address = (uint8_t)addr;
            } else {
                mqtt_log_warning("Config line %d: invalid address '%s'", line_num, value);
            }
        } else if (strcmp(key, "baudrate") == 0) {
            if (mqtt_config_parse_int(value, &config->baudrate, 1200, 115200) != 0) {
                mqtt_log_warning("Config line %d: invalid baudrate '%s'", line_num, value);
            }
        } else if (strcmp(key, "channels") == 0 || strcmp(key, "num_channels") == 0) {
            if (mqtt_config_parse_int(value, &config->num_channels, 1, 8) != 0) {
                mqtt_log_warning("Config line %d: invalid channels '%s'", line_num, value);
            }
        } else if (strcmp(key, "mqtt_host") == 0 || strcmp(key, "host") == 0) {
            strncpy(config->mqtt_host, value, MQTT_MAX_HOST - 1);
        } else if (strcmp(key, "mqtt_port") == 0) {
            if (mqtt_config_parse_int(value, &config->mqtt_port, 1, 65535) != 0) {
                mqtt_log_warning("Config line %d: invalid mqtt_port '%s'", line_num, value);
            }
        } else if (strcmp(key, "mqtt_user") == 0 || strcmp(key, "user") == 0 ||
                   strcmp(key, "username") == 0) {
            strncpy(config->mqtt_user, value, MQTT_MAX_CRED - 1);
        } else if (strcmp(key, "mqtt_pass") == 0 || strcmp(key, "pass") == 0 ||
                   strcmp(key, "password") == 0) {
            strncpy(config->mqtt_pass, value, MQTT_MAX_CRED - 1);
        } else if (strcmp(key, "password_file") == 0) {
            strncpy(config->password_file, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "topic") == 0 || strcmp(key, "topic_prefix") == 0) {
            strncpy(config->topic_prefix, value, MQTT_MAX_TOPIC - 1);
        } else if (strcmp(key, "client_id") == 0) {
            strncpy(config->client_id, value, MQTT_MAX_CLIENT_ID - 1);
        } else if (strcmp(key, "qos") == 0) {
            if (mqtt_config_parse_int(value, &config->qos, 0, 2) != 0) {
                mqtt_log_warning("Config line %d: invalid qos '%s'", line_num, value);
            }
        } else if (strcmp(key, "retain") == 0) {
            config->retain = PARSE_BOOL(value);
        } else if (strcmp(key, "keepalive") == 0) {
            if (mqtt_config_parse_int(value, &config->keepalive, 1, 3600) != 0) {
                mqtt_log_warning("Config line %d: invalid keepalive '%s'", line_num, value);
            }
        } else if (strcmp(key, "interval") == 0) {
            if (mqtt_config_parse_int(value, &config->interval, 1, 86400) != 0) {
                mqtt_log_warning("Config line %d: invalid interval '%s'", line_num, value);
            }
        } else if (strcmp(key, "pid_file") == 0) {
            strncpy(config->pid_file, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "median_filter") == 0) {
            config->enable_median_filter = PARSE_BOOL(value);
        } else if (strcmp(key, "maf_filter") == 0) {
            config->enable_maf_filter = PARSE_BOOL(value);
        } else if (strcmp(key, "maf_window") == 0 || strcmp(key, "maf_window_size") == 0) {
            if (mqtt_config_parse_int(value, &config->maf_window_size, 3, 15) == 0) {
                config->enable_maf_filter = 1;
            } else {
                mqtt_log_warning("Config line %d: invalid maf_window '%s'", line_num, value);
            }
        } else if (strcmp(key, "verbose") == 0) {
            config->verbose = PARSE_BOOL(value);
        /* TLS options */
        } else if (strcmp(key, "tls") == 0 || strcmp(key, "use_tls") == 0) {
            config->use_tls = PARSE_BOOL(value);
        } else if (strcmp(key, "tls_ca") == 0 || strcmp(key, "tls_ca_file") == 0) {
            strncpy(config->tls_ca_file, value, MQTT_MAX_PATH - 1);
            config->use_tls = 1;
        } else if (strcmp(key, "tls_cert") == 0 || strcmp(key, "tls_cert_file") == 0) {
            strncpy(config->tls_cert_file, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "tls_key") == 0 || strcmp(key, "tls_key_file") == 0) {
            strncpy(config->tls_key_file, value, MQTT_MAX_PATH - 1);
        } else if (strcmp(key, "tls_insecure") == 0) {
            config->tls_insecure = PARSE_BOOL(value);
        } else {
            mqtt_log_warning("Config line %d: unknown key '%s'", line_num, key);
        }

        #undef PARSE_BOOL
    }

    fclose(fp);
    return MQTT_OK;
}

MqttStatus mqtt_config_parse_args(int argc, char *argv[], MqttConfig *config)
{
    int opt;
    int option_index = 0;
    int tmp;

    if (config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    while ((opt = getopt_long(argc, argv, "p:a:b:n:H:P:u:W:t:i:I:c:F:dvmM:ShV",
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                strncpy(config->serial_port, optarg, MQTT_MAX_PATH - 1);
                break;
            case 'a':
                if (mqtt_config_parse_int(optarg, &tmp, 1, 254) == 0) {
                    config->device_address = (uint8_t)tmp;
                } else {
                    fprintf(stderr, "Error: invalid address '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'b':
                if (mqtt_config_parse_int(optarg, &config->baudrate, 1200, 115200) != 0) {
                    fprintf(stderr, "Error: invalid baudrate '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'n':
                if (mqtt_config_parse_int(optarg, &config->num_channels, 1, 8) != 0) {
                    fprintf(stderr, "Error: invalid channels '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'H':
                strncpy(config->mqtt_host, optarg, MQTT_MAX_HOST - 1);
                break;
            case 'P':
                if (mqtt_config_parse_int(optarg, &config->mqtt_port, 1, 65535) != 0) {
                    fprintf(stderr, "Error: invalid port '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'u':
                strncpy(config->mqtt_user, optarg, MQTT_MAX_CRED - 1);
                break;
            case 'W':
                strncpy(config->password_file, optarg, MQTT_MAX_PATH - 1);
                break;
            case 't':
                strncpy(config->topic_prefix, optarg, MQTT_MAX_TOPIC - 1);
                break;
            case 'i':
                strncpy(config->client_id, optarg, MQTT_MAX_CLIENT_ID - 1);
                break;
            case 'I':
                if (mqtt_config_parse_int(optarg, &config->interval, 1, 86400) != 0) {
                    fprintf(stderr, "Error: invalid interval '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'c':
                strncpy(config->config_file, optarg, MQTT_MAX_PATH - 1);
                break;
            case 'F':
                strncpy(config->pid_file, optarg, MQTT_MAX_PATH - 1);
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
                if (mqtt_config_parse_int(optarg, &config->maf_window_size, 3, 15) != 0) {
                    fprintf(stderr, "Error: invalid MAF window size '%s'\n", optarg);
                    return MQTT_ERR_CONFIG_VALUE;
                }
                break;
            case 'S':
                config->use_tls = 1;
                break;
            case 1001:  /* --tls-ca */
                strncpy(config->tls_ca_file, optarg, MQTT_MAX_PATH - 1);
                config->use_tls = 1;
                break;
            case 1002:  /* --tls-cert */
                strncpy(config->tls_cert_file, optarg, MQTT_MAX_PATH - 1);
                break;
            case 1003:  /* --tls-key */
                strncpy(config->tls_key_file, optarg, MQTT_MAX_PATH - 1);
                break;
            case 1004:  /* --tls-insecure */
                config->tls_insecure = 1;
                break;
            case 'V':
                printf("r4dcb08-mqtt version 1.1\n");
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

    /* Validate TLS settings */
    if (config->use_tls) {
        /* CA file is required unless tls_insecure is set */
        if (config->tls_ca_file[0] == '\0' && !config->tls_insecure) {
            mqtt_log_error("TLS enabled but no CA file specified (use --tls-ca or --tls-insecure)");
            return MQTT_ERR_CONFIG_VALUE;
        }
        /* If cert is specified, key must also be specified */
        if (config->tls_cert_file[0] != '\0' && config->tls_key_file[0] == '\0') {
            mqtt_log_error("TLS client certificate specified but no key file");
            return MQTT_ERR_CONFIG_VALUE;
        }
        if (config->tls_key_file[0] != '\0' && config->tls_cert_file[0] == '\0') {
            mqtt_log_error("TLS key file specified but no certificate");
            return MQTT_ERR_CONFIG_VALUE;
        }
        /* Warn about insecure mode */
        if (config->tls_insecure) {
            mqtt_log_warning("TLS insecure mode enabled - certificate verification disabled!");
        }
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
    mqtt_log_info("  MQTT host: %s:%d%s", config->mqtt_host, config->mqtt_port,
                 config->use_tls ? " (TLS)" : "");
    mqtt_log_info("  Topic prefix: %s", config->topic_prefix);
    mqtt_log_info("  Client ID: %s", config->client_id);
    mqtt_log_info("  Interval: %d s", config->interval);
    mqtt_log_info("  QoS: %d, Retain: %s", config->qos,
                 config->retain ? "yes" : "no");
    if (config->mqtt_user[0] != '\0') {
        mqtt_log_info("  Auth: user=%s, password=%s", config->mqtt_user,
                     config->mqtt_pass[0] != '\0' ? "***" : "(none)");
    }
    if (config->use_tls) {
        mqtt_log_info("  TLS: enabled");
        if (config->tls_ca_file[0] != '\0') {
            mqtt_log_info("    CA file: %s", config->tls_ca_file);
        }
        if (config->tls_cert_file[0] != '\0') {
            mqtt_log_info("    Client cert: %s", config->tls_cert_file);
        }
        if (config->tls_insecure) {
            mqtt_log_info("    WARNING: Insecure mode (no cert verification)");
        }
    }
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
    printf("  -P, --mqtt-port <port>   MQTT broker port (default: %d, TLS: %d)\n",
           MQTT_DEFAULT_MQTT_PORT, MQTT_DEFAULT_MQTT_PORT_TLS);
    printf("  -u, --mqtt-user <user>   MQTT username\n");
    printf("  -W, --password-file <f>  File containing MQTT password\n");
    printf("  -t, --topic <prefix>     Topic prefix (default: %s)\n", MQTT_DEFAULT_TOPIC);
    printf("  -i, --client-id <id>     MQTT client ID\n");
    printf("\nTLS options:\n");
    printf("  -S, --tls                Enable TLS/SSL encryption\n");
    printf("      --tls-ca <file>      CA certificate file (enables TLS)\n");
    printf("      --tls-cert <file>    Client certificate file\n");
    printf("      --tls-key <file>     Client private key file\n");
    printf("      --tls-insecure       Skip certificate verification (testing only)\n");
    printf("\nDaemon options:\n");
    printf("  -I, --interval <sec>     Measurement interval in seconds (default: %d)\n", MQTT_DEFAULT_INTERVAL);
    printf("  -c, --config <file>      Configuration file path\n");
    printf("  -F, --pid-file <file>    PID file path (default: /var/run/r4dcb08-mqtt.pid)\n");
    printf("  -d, --daemon             Run as daemon\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("\nFilter options:\n");
    printf("  -m, --median-filter      Enable median filter\n");
    printf("  -M, --maf-filter <size>  Enable MAF filter with window size (odd, 3-15)\n");
    printf("\nOther options:\n");
    printf("  -h, --help               Show this help\n");
    printf("  -V, --version            Show version\n");
    printf("\nPassword can also be set via %s environment variable.\n", MQTT_PASSWORD_ENV);
    printf("\nExample:\n");
    printf("  %s -p /dev/ttyUSB0 -a 1 -H localhost -I 10\n", program_name);
    printf("  %s -c /etc/r4dcb08-mqtt.conf -d\n", program_name);
    printf("  %s -H broker.example.com --tls --tls-ca /etc/ssl/ca.crt -u user -W /etc/mqtt.pass\n", program_name);
}

MqttStatus mqtt_config_load_password(MqttConfig *config)
{
    FILE *fp;
    char *env_pass;
    char line[MQTT_MAX_CRED];

    if (config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    /* Priority 1: Password file */
    if (config->password_file[0] != '\0') {
        fp = fopen(config->password_file, "r");
        if (fp == NULL) {
            mqtt_log_error("Cannot open password file: %s", config->password_file);
            return MQTT_ERR_CONFIG_FILE;
        }

        if (fgets(line, sizeof(line), fp) != NULL) {
            /* Remove trailing newline */
            line[strcspn(line, "\r\n")] = '\0';
            strncpy(config->mqtt_pass, line, MQTT_MAX_CRED - 1);
            config->mqtt_pass[MQTT_MAX_CRED - 1] = '\0';

            /* Clear local buffer */
            explicit_bzero(line, sizeof(line));
        }

        fclose(fp);
        mqtt_log_debug("Password loaded from file");
        return MQTT_OK;
    }

    /* Priority 2: Environment variable (if password not already set) */
    if (config->mqtt_pass[0] == '\0') {
        env_pass = getenv(MQTT_PASSWORD_ENV);
        if (env_pass != NULL && env_pass[0] != '\0') {
            strncpy(config->mqtt_pass, env_pass, MQTT_MAX_CRED - 1);
            config->mqtt_pass[MQTT_MAX_CRED - 1] = '\0';
            mqtt_log_debug("Password loaded from environment");
        }
    }

    /* Priority 3: Config file value (already loaded) */
    return MQTT_OK;
}

void mqtt_config_clear_sensitive(MqttConfig *config)
{
    if (config == NULL) {
        return;
    }

    /* Securely clear password from memory */
    explicit_bzero(config->mqtt_pass, sizeof(config->mqtt_pass));
}
