/*
 * MQTT temperature publishing logic
 * V1.0/2026-01-29
 */
#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include "mqtt_client.h"
#include "mqtt_config.h"
#include "mqtt_error.h"
#include "mqtt_metrics.h"

/* Maximum payload size */
#define MQTT_MAX_PAYLOAD 64

/* Temperature reading context */
typedef struct {
    int fd;                     /* Serial port file descriptor */
    const MqttConfig *config;   /* Configuration */
    int filter_initialized;     /* Filter state flag */
} TempContext;

/**
 * Initialize temperature reading context
 *
 * @param ctx Pointer to context structure
 * @param config Pointer to configuration
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_temp_init(TempContext *ctx, const MqttConfig *config);

/**
 * Open serial port for temperature reading
 *
 * @param ctx Pointer to context structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_temp_open(TempContext *ctx);

/**
 * Close serial port
 *
 * @param ctx Pointer to context structure
 */
void mqtt_temp_close(TempContext *ctx);

/**
 * Read temperatures from device and publish to MQTT
 *
 * Reads all configured channels, applies filters if enabled,
 * and publishes to MQTT topics:
 *   {prefix}/{address}/temperature/ch1 ... chN
 *   {prefix}/{address}/timestamp
 *   {prefix}/{address}/status
 *
 * @param ctx Pointer to temperature context
 * @param client Pointer to MQTT client
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_publish_temperatures(TempContext *ctx, MqttClient *client);

/**
 * Publish device status
 *
 * @param client Pointer to MQTT client
 * @param status Status string ("online", "offline", "error")
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_publish_status(MqttClient *client, const char *status);

/**
 * Publish diagnostic metrics
 *
 * Publishes JSON payload to {prefix}/{address}/diagnostics:
 * {"uptime":N,"reads":{"total":N,"success":N,"failure":N},"mqtt_reconnects":N,"consecutive_errors":N}
 *
 * @param client Pointer to MQTT client
 * @param metrics Pointer to metrics structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_publish_diagnostics(MqttClient *client, const MqttMetrics *metrics);

#endif /* MQTT_PUBLISH_H */
