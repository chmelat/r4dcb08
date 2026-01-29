/*
 * R4DCB08 MQTT daemon main entry point
 * V1.0/2026-01-29
 *
 * Reads temperatures from R4DCB08 sensor via Modbus RTU
 * and publishes to MQTT broker using libmosquitto.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "mqtt_config.h"
#include "mqtt_error.h"
#include "mqtt_client.h"
#include "mqtt_publish.h"

/* Program name for logging */
#define PROGRAM_NAME "r4dcb08-mqtt"
#define PROGRAM_VERSION "1.0"

/* Global flag for graceful shutdown */
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t reload_config = 0;

/* Signal handler */
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        running = 0;
    } else if (sig == SIGHUP) {
        reload_config = 1;
    }
}

/* Setup signal handlers */
static void setup_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
}

/* Daemonize process */
static int daemonize(void)
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        mqtt_log_error("Fork failed: %s", strerror(errno));
        return -1;
    }

    /* Exit parent */
    if (pid > 0) {
        exit(0);
    }

    /* Create new session */
    if (setsid() < 0) {
        mqtt_log_error("setsid failed: %s", strerror(errno));
        return -1;
    }

    /* Fork again to prevent terminal acquisition */
    pid = fork();
    if (pid < 0) {
        mqtt_log_error("Second fork failed: %s", strerror(errno));
        return -1;
    }

    if (pid > 0) {
        exit(0);
    }

    /* Change working directory */
    if (chdir("/") < 0) {
        mqtt_log_error("chdir failed: %s", strerror(errno));
        return -1;
    }

    /* Set file mode mask */
    umask(0);

    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Redirect to /dev/null */
    open("/dev/null", O_RDONLY);  /* stdin */
    open("/dev/null", O_WRONLY);  /* stdout */
    open("/dev/null", O_WRONLY);  /* stderr */

    return 0;
}

/* Main daemon loop */
static int daemon_loop(MqttConfig *config)
{
    MqttClient client;
    TempContext temp_ctx;
    MqttStatus status;
    struct timespec ts;
    int consecutive_errors = 0;
    const int max_consecutive_errors = 10;

    /* Initialize MQTT client library */
    status = mqtt_client_lib_init();
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to initialize MQTT library");
        return 1;
    }

    /* Initialize temperature reading context */
    status = mqtt_temp_init(&temp_ctx, config);
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to initialize temperature context");
        mqtt_client_lib_cleanup();
        return 1;
    }

    /* Open serial port */
    status = mqtt_temp_open(&temp_ctx);
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to open serial port");
        mqtt_client_lib_cleanup();
        return 1;
    }

    /* Create MQTT client */
    status = mqtt_client_create(&client, config);
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to create MQTT client");
        mqtt_temp_close(&temp_ctx);
        mqtt_client_lib_cleanup();
        return 1;
    }

    /* Connect to MQTT broker */
    status = mqtt_client_connect(&client);
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to connect to MQTT broker");
        mqtt_client_destroy(&client);
        mqtt_temp_close(&temp_ctx);
        mqtt_client_lib_cleanup();
        return 1;
    }

    /* Publish initial online status */
    mqtt_publish_status(&client, "online");

    mqtt_log_info("Daemon started, interval=%d s", config->interval);

    /* Main loop */
    while (running) {
        /* Check for SIGHUP (config reload) */
        if (reload_config) {
            mqtt_log_info("Received SIGHUP, reloading configuration not implemented");
            reload_config = 0;
        }

        /* Check MQTT connection */
        if (!mqtt_client_is_connected(&client)) {
            mqtt_log_warning("MQTT connection lost, reconnecting...");
            status = mqtt_client_reconnect(&client);
            if (status != MQTT_OK) {
                consecutive_errors++;
                if (consecutive_errors >= max_consecutive_errors) {
                    mqtt_log_error("Too many consecutive reconnect failures");
                    break;
                }
                continue;
            }
            consecutive_errors = 0;
            mqtt_publish_status(&client, "online");
        }

        /* Read and publish temperatures */
        status = mqtt_publish_temperatures(&temp_ctx, &client);
        if (status != MQTT_OK) {
            consecutive_errors++;
            mqtt_log_warning("Temperature read/publish failed (%d/%d)",
                           consecutive_errors, max_consecutive_errors);

            if (consecutive_errors >= max_consecutive_errors) {
                mqtt_log_error("Too many consecutive errors, exiting");
                break;
            }

            /* Try to reopen serial port on error */
            mqtt_temp_close(&temp_ctx);
            sleep(1);
            if (mqtt_temp_open(&temp_ctx) != MQTT_OK) {
                mqtt_log_error("Failed to reopen serial port");
            }
        } else {
            consecutive_errors = 0;
        }

        /* Sleep for interval */
        if (running) {
            ts.tv_sec = config->interval;
            ts.tv_nsec = 0;
            nanosleep(&ts, NULL);
        }
    }

    /* Cleanup */
    mqtt_log_info("Shutting down...");

    /* Publish offline status before disconnecting */
    if (mqtt_client_is_connected(&client)) {
        mqtt_publish_status(&client, "offline");
        usleep(100000);  /* Allow message to be sent */
    }

    mqtt_client_destroy(&client);
    mqtt_temp_close(&temp_ctx);
    mqtt_client_lib_cleanup();

    mqtt_log_info("Daemon stopped");
    return 0;
}

int main(int argc, char *argv[])
{
    MqttConfig config;
    MqttStatus status;
    int exit_code = 0;

    /* Initialize configuration with defaults */
    mqtt_config_init(&config);

    /* Parse command line arguments (first pass for config file) */
    status = mqtt_config_parse_args(argc, argv, &config);
    if (status != MQTT_OK) {
        fprintf(stderr, "Error parsing arguments\n");
        return 1;
    }

    /* Parse config file if specified */
    if (config.config_file[0] != '\0') {
        status = mqtt_config_parse_file(config.config_file, &config);
        if (status != MQTT_OK) {
            fprintf(stderr, "Error parsing config file: %s\n", config.config_file);
            return 1;
        }

        /* Re-parse command line to override config file */
        optind = 1;  /* Reset getopt */
        mqtt_config_parse_args(argc, argv, &config);
    }

    /* Validate configuration */
    status = mqtt_config_validate(&config);
    if (status != MQTT_OK) {
        fprintf(stderr, "Invalid configuration\n");
        return 1;
    }

    /* Initialize logging */
    mqtt_log_init(config.daemon_mode, PROGRAM_NAME);
    mqtt_log_set_verbose(config.verbose);

    /* Print configuration if verbose */
    if (config.verbose) {
        mqtt_config_print(&config);
    }

    /* Setup signal handlers */
    setup_signals();

    /* Daemonize if requested */
    if (config.daemon_mode) {
        mqtt_log_info("Starting in daemon mode");
        if (daemonize() < 0) {
            mqtt_log_error("Failed to daemonize");
            mqtt_log_close();
            return 1;
        }
        /* Re-initialize logging after daemonization */
        mqtt_log_init(1, PROGRAM_NAME);
        mqtt_log_set_verbose(config.verbose);
    }

    mqtt_log_info("%s version %s starting", PROGRAM_NAME, PROGRAM_VERSION);

    /* Run main daemon loop */
    exit_code = daemon_loop(&config);

    /* Cleanup */
    mqtt_log_close();

    return exit_code;
}
