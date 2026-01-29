/*
 * MQTT temperature publishing logic
 * V1.0/2026-01-29
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "mqtt_publish.h"
#include "mqtt_error.h"

/* Include original r4dcb08 modules */
#include "../serial.h"
#include "../packet.h"
#include "../monada.h"
#include "../typedef.h"
#include "../now.h"
#include "../median_filter.h"
#include "../maf_filter.h"
#include "../constants.h"
#include "../define_error_resp.h"

MqttStatus mqtt_temp_init(TempContext *ctx, const MqttConfig *config)
{
    if (ctx == NULL || config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    memset(ctx, 0, sizeof(TempContext));
    ctx->fd = -1;
    ctx->config = config;
    ctx->filter_initialized = 0;

    /* Initialize MAF filter if enabled */
    if (config->enable_maf_filter) {
        int rc = maf_init(config->maf_window_size);
        if (rc != MAF_SUCCESS) {
            mqtt_log_error("MAF filter initialization failed: %d", rc);
            return MQTT_ERR_CONFIG_VALUE;
        }
        ctx->filter_initialized = 1;
        mqtt_log_info("MAF filter initialized (window=%d)", config->maf_window_size);
    }

    return MQTT_OK;
}

MqttStatus mqtt_temp_open(TempContext *ctx)
{
    int rc;

    if (ctx == NULL || ctx->config == NULL) {
        return MQTT_ERR_SERIAL;
    }

    /* Close if already open */
    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    /* Open serial port */
    ctx->fd = open_port(ctx->config->serial_port);
    if (ctx->fd < 0) {
        mqtt_log_error("Failed to open serial port: %s", ctx->config->serial_port);
        return MQTT_ERR_SERIAL;
    }

    /* Configure serial port */
    rc = set_port(ctx->fd, ctx->config->baudrate);
    if (rc != SERIAL_SUCCESS) {
        mqtt_log_error("Failed to configure serial port: %d", rc);
        close(ctx->fd);
        ctx->fd = -1;
        return MQTT_ERR_SERIAL;
    }

    mqtt_log_info("Serial port opened: %s @ %d baud",
                 ctx->config->serial_port, ctx->config->baudrate);

    return MQTT_OK;
}

void mqtt_temp_close(TempContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    if (ctx->filter_initialized) {
        maf_reset();
        ctx->filter_initialized = 0;
    }
}

MqttStatus mqtt_publish_temperatures(TempContext *ctx, MqttClient *client)
{
    PACKET pr;
    PACKET *p_pr = &pr;
    uint8_t *p_data;
    uint8_t input_data[DMAX];
    int i, rc;
    float T[MAX_CHANNELS];
    float T_filtered[MAX_CHANNELS];
    char sample_time[DBUF];
    char sample_filtered[DBUF];
    char payload[MQTT_MAX_PAYLOAD];
    char topic[64];
    MqttStatus status;
    AppStatus app_status;
    int n;

    if (ctx == NULL || client == NULL || ctx->fd < 0) {
        return MQTT_ERR_READ_TEMP;
    }

    n = ctx->config->num_channels;

    /* Prepare Modbus request */
    /* Register address (2 byte) + Read count (2 byte) */
    input_data[0] = 0x00;
    input_data[1] = 0x00;
    input_data[2] = 0x00;
    input_data[3] = (uint8_t)n;

    /* Read temperatures via Modbus */
    app_status = monada(ctx->fd, ctx->config->device_address, '\x03', 4,
                       input_data, p_pr, 0, "read_temp", 0, &p_data);

    if (app_status != STATUS_OK) {
        mqtt_log_error("Modbus read failed: %d", app_status);
        mqtt_publish_status(client, "error");
        return MQTT_ERR_MODBUS;
    }

    /* Get timestamp */
    char *t = now();
    if (t != NULL) {
        strncpy(sample_time, t, sizeof(sample_time) - 1);
        sample_time[sizeof(sample_time) - 1] = '\0';
    } else {
        strcpy(sample_time, "unknown");
    }

    /* Parse temperature values */
    for (i = 0; i < n; i++) {
        T[i] = (float)INT16(p_data[2*i+1], p_data[2*i]) / 10.0f;
        /* Check for invalid readings */
        if (T[i] < MIN_TEMPERATURE || T[i] > MAX_TEMPERATURE) {
            T[i] = ERRRESP;
        }
    }

    /* Apply median filter if enabled */
    if (ctx->config->enable_median_filter) {
        rc = median_filter(sample_time, n, T, sample_filtered, T_filtered);
        if (rc == MF_SUCCESS) {
            strncpy(sample_time, sample_filtered, sizeof(sample_time) - 1);
            sample_time[sizeof(sample_time) - 1] = '\0';
            for (i = 0; i < n; i++) {
                T[i] = T_filtered[i];
            }
        } else {
            mqtt_log_warning("Median filter failed: %d", rc);
        }
    }

    /* Apply MAF filter if enabled */
    if (ctx->config->enable_maf_filter) {
        rc = maf_filter(sample_time, n, T, sample_filtered, T_filtered);
        if (rc == MAF_SUCCESS) {
            strncpy(sample_time, sample_filtered, sizeof(sample_time) - 1);
            sample_time[sizeof(sample_time) - 1] = '\0';
            for (i = 0; i < n; i++) {
                T[i] = T_filtered[i];
            }
        } else {
            mqtt_log_warning("MAF filter failed: %d", rc);
        }
    }

    /* Publish temperatures to MQTT */
    for (i = 0; i < n; i++) {
        snprintf(topic, sizeof(topic), "temperature/ch%d", i + 1);

        if (T[i] != ERRRESP) {
            snprintf(payload, sizeof(payload), "%.1f", T[i]);
        } else {
            snprintf(payload, sizeof(payload), "NaN");
        }

        status = mqtt_client_publish(client, topic, payload,
                                    ctx->config->qos, ctx->config->retain);
        if (status != MQTT_OK) {
            mqtt_log_warning("Failed to publish ch%d", i + 1);
        }
    }

    /* Publish timestamp */
    status = mqtt_client_publish(client, "timestamp", sample_time,
                                ctx->config->qos, ctx->config->retain);
    if (status != MQTT_OK) {
        mqtt_log_warning("Failed to publish timestamp");
    }

    /* Publish status */
    mqtt_publish_status(client, "online");

    /* Log reading */
    mqtt_log_debug("Published: %s", sample_time);
    for (i = 0; i < n; i++) {
        if (T[i] != ERRRESP) {
            mqtt_log_debug("  ch%d: %.1f C", i + 1, T[i]);
        } else {
            mqtt_log_debug("  ch%d: NaN", i + 1);
        }
    }

    return MQTT_OK;
}

MqttStatus mqtt_publish_status(MqttClient *client, const char *status)
{
    if (client == NULL || status == NULL) {
        return MQTT_ERR_PUBLISH;
    }

    return mqtt_client_publish(client, "status", status,
                              client->config->qos, 1);  /* Always retain status */
}
