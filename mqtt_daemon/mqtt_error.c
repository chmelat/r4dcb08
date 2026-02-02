/*
 * MQTT daemon error handling and logging
 * V1.1/2026-02-02
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "mqtt_error.h"

static int use_syslog = 0;
static int verbose_mode = 0;

void mqtt_log_init(int daemon_mode, const char *program_name)
{
    use_syslog = daemon_mode;
    if (use_syslog) {
        openlog(program_name, LOG_PID | LOG_CONS, LOG_DAEMON);
    }
}

void mqtt_log_close(void)
{
    if (use_syslog) {
        closelog();
    }
}

void mqtt_log_set_verbose(int verbose)
{
    verbose_mode = verbose;
}

static void log_message(int priority, const char *prefix, const char *format, va_list args)
{
    if (use_syslog) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        syslog(priority, "%s", buffer);
    } else {
        time_t now = time(NULL);
        struct tm tm_buf;
        struct tm *tm_info = localtime_r(&now, &tm_buf);
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

        fprintf(stderr, "%s [%s] ", time_buf, prefix);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
}

void mqtt_log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, "INFO", format, args);
    va_end(args);
}

void mqtt_log_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(LOG_WARNING, "WARN", format, args);
    va_end(args);
}

void mqtt_log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(LOG_ERR, "ERROR", format, args);
    va_end(args);
}

void mqtt_log_debug(const char *format, ...)
{
    if (!verbose_mode) {
        return;
    }
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, "DEBUG", format, args);
    va_end(args);
}

const char *mqtt_status_str(MqttStatus status)
{
    switch (status) {
        case MQTT_OK:
            return "Success";
        case MQTT_ERR_CONFIG_FILE:
            return "Cannot open configuration file";
        case MQTT_ERR_CONFIG_PARSE:
            return "Configuration parse error";
        case MQTT_ERR_CONFIG_VALUE:
            return "Invalid configuration value";
        case MQTT_ERR_CONNECT:
            return "MQTT connection failed";
        case MQTT_ERR_DISCONNECT:
            return "MQTT disconnection error";
        case MQTT_ERR_RECONNECT:
            return "MQTT reconnection failed";
        case MQTT_ERR_PUBLISH:
            return "MQTT publish failed";
        case MQTT_ERR_TOPIC:
            return "Invalid MQTT topic";
        case MQTT_ERR_SERIAL:
            return "Serial port error";
        case MQTT_ERR_MODBUS:
            return "Modbus communication error";
        case MQTT_ERR_READ_TEMP:
            return "Temperature read error";
        case MQTT_ERR_DAEMON:
            return "Daemon error";
        case MQTT_ERR_FORK:
            return "Fork failed";
        case MQTT_ERR_PID_FILE:
            return "PID file error";
        case MQTT_ERR_MOSQUITTO_INIT:
            return "Mosquitto initialization failed";
        case MQTT_ERR_MOSQUITTO_LIB:
            return "Mosquitto library error";
        default:
            return "Unknown error";
    }
}
