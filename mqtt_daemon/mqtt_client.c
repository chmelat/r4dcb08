/*
 * MQTT client wrapper for libmosquitto
 * V1.0/2026-01-29
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mqtt_client.h"
#include "mqtt_error.h"

/* Callback functions */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    MqttClient *client = (MqttClient *)userdata;
    (void)mosq;

    if (rc == 0) {
        client->connected = 1;
        client->reconnect_delay = MQTT_RECONNECT_MIN_DELAY;
        mqtt_log_info("Connected to MQTT broker");
    } else {
        client->connected = 0;
        mqtt_log_error("Connection failed: %s", mosquitto_connack_string(rc));
    }
}

static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    MqttClient *client = (MqttClient *)userdata;
    (void)mosq;

    client->connected = 0;
    if (rc == 0) {
        mqtt_log_info("Disconnected from MQTT broker");
    } else {
        mqtt_log_warning("Unexpected disconnection (rc=%d)", rc);
    }
}

static void on_publish(struct mosquitto *mosq, void *userdata, int mid)
{
    (void)mosq;
    (void)userdata;
    (void)mid;
    mqtt_log_debug("Message %d published", mid);
}

static void on_log(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    (void)mosq;
    (void)userdata;

    switch (level) {
        case MOSQ_LOG_ERR:
            mqtt_log_error("mosquitto: %s", str);
            break;
        case MOSQ_LOG_WARNING:
            mqtt_log_warning("mosquitto: %s", str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            mqtt_log_debug("mosquitto: %s", str);
            break;
        default:
            break;
    }
}

MqttStatus mqtt_client_lib_init(void)
{
    int rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_error("Failed to initialize mosquitto library: %s",
                      mosquitto_strerror(rc));
        return MQTT_ERR_MOSQUITTO_LIB;
    }
    return MQTT_OK;
}

void mqtt_client_lib_cleanup(void)
{
    mosquitto_lib_cleanup();
}

MqttStatus mqtt_client_create(MqttClient *client, const MqttConfig *config)
{
    if (client == NULL || config == NULL) {
        return MQTT_ERR_CONFIG_VALUE;
    }

    memset(client, 0, sizeof(MqttClient));
    client->config = config;
    client->connected = 0;
    client->reconnect_delay = MQTT_RECONNECT_MIN_DELAY;

    /* Create mosquitto instance */
    client->mosq = mosquitto_new(config->client_id, true, client);
    if (client->mosq == NULL) {
        mqtt_log_error("Failed to create mosquitto instance");
        return MQTT_ERR_MOSQUITTO_INIT;
    }

    /* Set callbacks */
    mosquitto_connect_callback_set(client->mosq, on_connect);
    mosquitto_disconnect_callback_set(client->mosq, on_disconnect);
    mosquitto_publish_callback_set(client->mosq, on_publish);
    mosquitto_log_callback_set(client->mosq, on_log);

    /* Set username/password if provided */
    if (config->mqtt_user[0] != '\0') {
        int rc = mosquitto_username_pw_set(client->mosq,
                                           config->mqtt_user,
                                           config->mqtt_pass[0] != '\0' ?
                                           config->mqtt_pass : NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            mqtt_log_error("Failed to set credentials: %s", mosquitto_strerror(rc));
            mosquitto_destroy(client->mosq);
            client->mosq = NULL;
            return MQTT_ERR_MOSQUITTO_INIT;
        }
    }

    return MQTT_OK;
}

MqttStatus mqtt_client_connect(MqttClient *client)
{
    int rc;
    char lwt_topic[MQTT_MAX_TOPIC + 64];

    if (client == NULL || client->mosq == NULL) {
        return MQTT_ERR_CONNECT;
    }

    /* Set Last Will and Testament (LWT) */
    snprintf(lwt_topic, sizeof(lwt_topic), "%s/%d/status",
             client->config->topic_prefix, client->config->device_address);

    rc = mosquitto_will_set(client->mosq, lwt_topic, 7, "offline",
                           client->config->qos, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_warning("Failed to set LWT: %s", mosquitto_strerror(rc));
    }

    /* Connect to broker */
    mqtt_log_info("Connecting to %s:%d...",
                 client->config->mqtt_host, client->config->mqtt_port);

    rc = mosquitto_connect(client->mosq,
                          client->config->mqtt_host,
                          client->config->mqtt_port,
                          client->config->keepalive);

    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_error("Failed to connect: %s", mosquitto_strerror(rc));
        return MQTT_ERR_CONNECT;
    }

    /* Start network loop thread */
    rc = mosquitto_loop_start(client->mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_error("Failed to start network loop: %s", mosquitto_strerror(rc));
        mosquitto_disconnect(client->mosq);
        return MQTT_ERR_CONNECT;
    }

    /* Wait briefly for connection callback */
    for (int i = 0; i < 50 && !client->connected; i++) {
        usleep(100000);  /* 100ms */
    }

    if (!client->connected) {
        mqtt_log_warning("Connection callback not received yet");
    }

    return MQTT_OK;
}

MqttStatus mqtt_client_disconnect(MqttClient *client)
{
    if (client == NULL || client->mosq == NULL) {
        return MQTT_ERR_DISCONNECT;
    }

    /* Stop network loop */
    mosquitto_loop_stop(client->mosq, false);

    /* Disconnect */
    int rc = mosquitto_disconnect(client->mosq);
    if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
        mqtt_log_warning("Disconnect error: %s", mosquitto_strerror(rc));
    }

    client->connected = 0;
    return MQTT_OK;
}

void mqtt_client_destroy(MqttClient *client)
{
    if (client == NULL) {
        return;
    }

    if (client->mosq != NULL) {
        mqtt_client_disconnect(client);
        mosquitto_destroy(client->mosq);
        client->mosq = NULL;
    }
}

int mqtt_client_is_connected(const MqttClient *client)
{
    if (client == NULL) {
        return 0;
    }
    return client->connected;
}

MqttStatus mqtt_client_reconnect(MqttClient *client)
{
    int rc;

    if (client == NULL || client->mosq == NULL) {
        return MQTT_ERR_RECONNECT;
    }

    if (client->connected) {
        return MQTT_OK;
    }

    mqtt_log_info("Reconnecting in %d seconds...", client->reconnect_delay);
    sleep(client->reconnect_delay);

    rc = mosquitto_reconnect(client->mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_error("Reconnect failed: %s", mosquitto_strerror(rc));

        /* Exponential backoff */
        client->reconnect_delay *= MQTT_RECONNECT_MULTIPLIER;
        if (client->reconnect_delay > MQTT_RECONNECT_MAX_DELAY) {
            client->reconnect_delay = MQTT_RECONNECT_MAX_DELAY;
        }

        return MQTT_ERR_RECONNECT;
    }

    /* Wait for connection callback */
    for (int i = 0; i < 50 && !client->connected; i++) {
        usleep(100000);  /* 100ms */
    }

    if (client->connected) {
        client->reconnect_delay = MQTT_RECONNECT_MIN_DELAY;
        return MQTT_OK;
    }

    return MQTT_ERR_RECONNECT;
}

MqttStatus mqtt_client_loop(MqttClient *client, int timeout)
{
    if (client == NULL || client->mosq == NULL) {
        return MQTT_ERR_MOSQUITTO_LIB;
    }

    /* Note: Using mosquitto_loop_start() means we don't need to call loop manually */
    (void)timeout;
    return MQTT_OK;
}

MqttStatus mqtt_client_publish(MqttClient *client, const char *topic,
                               const char *payload, int qos, int retain)
{
    char full_topic[MQTT_MAX_TOPIC + 128];

    if (client == NULL || topic == NULL) {
        return MQTT_ERR_TOPIC;
    }

    snprintf(full_topic, sizeof(full_topic), "%s/%d/%s",
             client->config->topic_prefix,
             client->config->device_address,
             topic);

    return mqtt_client_publish_raw(client, full_topic, payload, qos, retain);
}

MqttStatus mqtt_client_publish_raw(MqttClient *client, const char *full_topic,
                                   const char *payload, int qos, int retain)
{
    int rc;

    if (client == NULL || client->mosq == NULL || full_topic == NULL) {
        return MQTT_ERR_PUBLISH;
    }

    if (!client->connected) {
        mqtt_log_debug("Not connected, cannot publish");
        return MQTT_ERR_CONNECT;
    }

    rc = mosquitto_publish(client->mosq, NULL, full_topic,
                          payload ? (int)strlen(payload) : 0,
                          payload, qos, retain);

    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_log_error("Publish failed: %s", mosquitto_strerror(rc));
        return MQTT_ERR_PUBLISH;
    }

    mqtt_log_debug("Published to %s: %s", full_topic, payload ? payload : "(null)");
    return MQTT_OK;
}
