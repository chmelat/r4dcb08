/*
 * MQTT daemon diagnostic metrics
 * V1.0/2026-01-29
 */
#ifndef MQTT_METRICS_H
#define MQTT_METRICS_H

#include <stdint.h>
#include <time.h>

typedef struct {
    time_t start_time;
    uint32_t read_total;
    uint32_t read_success;
    uint32_t read_failure;
    uint32_t mqtt_reconnect_count;
    int consecutive_errors;
} MqttMetrics;

/**
 * Initialize metrics structure
 *
 * @param metrics Pointer to metrics structure
 */
void mqtt_metrics_init(MqttMetrics *metrics);

/**
 * Get uptime in seconds
 *
 * @param metrics Pointer to metrics structure
 * @return Uptime in seconds
 */
uint32_t mqtt_metrics_uptime(const MqttMetrics *metrics);

/**
 * Record a successful read
 *
 * @param metrics Pointer to metrics structure
 */
void mqtt_metrics_read_success(MqttMetrics *metrics);

/**
 * Record a failed read
 *
 * @param metrics Pointer to metrics structure
 */
void mqtt_metrics_read_failure(MqttMetrics *metrics);

/**
 * Record an MQTT reconnect
 *
 * @param metrics Pointer to metrics structure
 */
void mqtt_metrics_reconnect(MqttMetrics *metrics);

/**
 * Set consecutive errors count
 *
 * @param metrics Pointer to metrics structure
 * @param count Number of consecutive errors
 */
void mqtt_metrics_set_consecutive_errors(MqttMetrics *metrics, int count);

#endif /* MQTT_METRICS_H */
