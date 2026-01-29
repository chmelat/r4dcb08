/*
 * MQTT daemon diagnostic metrics
 * V1.0/2026-01-29
 */
#include "mqtt_metrics.h"

void mqtt_metrics_init(MqttMetrics *metrics)
{
    if (metrics == NULL) {
        return;
    }

    metrics->start_time = time(NULL);
    metrics->read_total = 0;
    metrics->read_success = 0;
    metrics->read_failure = 0;
    metrics->mqtt_reconnect_count = 0;
    metrics->consecutive_errors = 0;
}

uint32_t mqtt_metrics_uptime(const MqttMetrics *metrics)
{
    if (metrics == NULL) {
        return 0;
    }

    time_t now = time(NULL);
    if (now < metrics->start_time) {
        return 0;
    }

    return (uint32_t)(now - metrics->start_time);
}

void mqtt_metrics_read_success(MqttMetrics *metrics)
{
    if (metrics == NULL) {
        return;
    }

    metrics->read_total++;
    metrics->read_success++;
}

void mqtt_metrics_read_failure(MqttMetrics *metrics)
{
    if (metrics == NULL) {
        return;
    }

    metrics->read_total++;
    metrics->read_failure++;
}

void mqtt_metrics_reconnect(MqttMetrics *metrics)
{
    if (metrics == NULL) {
        return;
    }

    metrics->mqtt_reconnect_count++;
}

void mqtt_metrics_set_consecutive_errors(MqttMetrics *metrics, int count)
{
    if (metrics == NULL) {
        return;
    }

    metrics->consecutive_errors = count;
}
