/*
 * R4DCB08 MQTT daemon main entry point
 * V1.1/2026-01-29
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

#ifdef USE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include "mqtt_config.h"
#include "mqtt_error.h"
#include "mqtt_client.h"
#include "mqtt_publish.h"
#include "mqtt_metrics.h"

/* Program name for logging */
#define PROGRAM_NAME "r4dcb08-mqtt"
#define PROGRAM_VERSION "1.1"

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

/* Write PID file */
static int write_pid_file(const char *pid_file)
{
    FILE *fp;

    if (pid_file == NULL || pid_file[0] == '\0') {
        return 0;  /* No PID file requested */
    }

    fp = fopen(pid_file, "w");
    if (fp == NULL) {
        mqtt_log_error("Cannot create PID file %s: %s", pid_file, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d\n", getpid());
    fclose(fp);

    mqtt_log_debug("PID file created: %s", pid_file);
    return 0;
}

/* Remove PID file */
static void remove_pid_file(const char *pid_file)
{
    if (pid_file != NULL && pid_file[0] != '\0') {
        unlink(pid_file);
    }
}

/* Daemonize process */
static int daemonize(void)
{
    pid_t pid;
    int fd;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        mqtt_log_error("Fork failed: %s", strerror(errno));
        return -1;
    }

    /* Exit parent */
    if (pid > 0) {
        _exit(0);  /* Use _exit to avoid flushing stdio buffers */
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
        _exit(0);
    }

    /* Change working directory */
    if (chdir("/") < 0) {
        mqtt_log_error("chdir failed: %s", strerror(errno));
        return -1;
    }

    /* Set restrictive file mode mask */
    umask(027);  /* rwxr-x--- for directories, rw-r----- for files */

    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Redirect stdin to /dev/null */
    fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    if (fd != STDIN_FILENO) {
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    /* Redirect stdout to /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    if (fd != STDOUT_FILENO) {
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    /* Redirect stderr to /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    if (fd != STDERR_FILENO) {
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    return 0;
}

/* Main daemon loop */
static int daemon_loop(MqttConfig *config)
{
    MqttClient client;
    TempContext temp_ctx;
    MqttMetrics metrics;
    MqttStatus status;
    struct timespec ts;
    int consecutive_errors = 0;
    const int max_consecutive_errors = 10;
    int diag_counter = 0;

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

    /* Initialize metrics */
    mqtt_metrics_init(&metrics);

    /* Open serial port */
    status = mqtt_temp_open(&temp_ctx);
    if (status != MQTT_OK) {
        mqtt_log_error("Failed to open serial port");
        mqtt_temp_close(&temp_ctx);  /* Clean up any partially initialized state */
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

    /* Clear password from memory after it's been passed to mosquitto */
    mqtt_config_clear_sensitive(config);

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

    /* Notify systemd we are ready */
#ifdef USE_SYSTEMD
    sd_notify(0, "READY=1");
#endif

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
                mqtt_metrics_set_consecutive_errors(&metrics, consecutive_errors);
                if (consecutive_errors >= max_consecutive_errors) {
                    mqtt_log_error("Too many consecutive reconnect failures");
                    break;
                }
                continue;
            }
            mqtt_metrics_reconnect(&metrics);
            consecutive_errors = 0;
            mqtt_metrics_set_consecutive_errors(&metrics, 0);
            mqtt_publish_status(&client, "online");
        }

        /* Read and publish temperatures */
        status = mqtt_publish_temperatures(&temp_ctx, &client);
        if (status != MQTT_OK) {
            mqtt_metrics_read_failure(&metrics);
            consecutive_errors++;
            mqtt_metrics_set_consecutive_errors(&metrics, consecutive_errors);
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
            mqtt_metrics_read_success(&metrics);
            consecutive_errors = 0;
            mqtt_metrics_set_consecutive_errors(&metrics, 0);
        }

        /* Publish diagnostics every N intervals */
        if (config->diagnostics_interval > 0) {
            diag_counter++;
            if (diag_counter >= config->diagnostics_interval) {
                mqtt_publish_diagnostics(&client, &metrics);
                diag_counter = 0;
            }
        }

        /* Notify systemd watchdog on each iteration */
#ifdef USE_SYSTEMD
        sd_notify(0, "WATCHDOG=1");
#endif

        /* Sleep for interval, checking for shutdown every second */
        for (int i = 0; i < config->interval && running; i++) {
            ts.tv_sec = 1;
            ts.tv_nsec = 0;
            nanosleep(&ts, NULL);
        }
    }

    /* Cleanup */
    mqtt_log_info("Shutting down...");

    /* Notify systemd we are stopping */
#ifdef USE_SYSTEMD
    sd_notify(0, "STOPPING=1");
#endif

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

    /* Load password from file or environment */
    status = mqtt_config_load_password(&config);
    if (status != MQTT_OK) {
        fprintf(stderr, "Error loading password\n");
        return 1;
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

        /* Create PID file */
        if (write_pid_file(config.pid_file) < 0) {
            mqtt_log_error("Failed to create PID file");
            mqtt_log_close();
            return 1;
        }
    }

    mqtt_log_info("%s version %s starting", PROGRAM_NAME, PROGRAM_VERSION);

    /* Run main daemon loop */
    exit_code = daemon_loop(&config);

    /* Cleanup */
    if (config.daemon_mode) {
        remove_pid_file(config.pid_file);
    }
    mqtt_log_close();

    return exit_code;
}
