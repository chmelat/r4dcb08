/*
 * MQTT client wrapper for libmosquitto
 * V1.0/2026-01-29
 */
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mosquitto.h>
#include "mqtt_config.h"
#include "mqtt_error.h"

/* Reconnect parameters */
#define MQTT_RECONNECT_MIN_DELAY 1      /* Minimum delay in seconds */
#define MQTT_RECONNECT_MAX_DELAY 60     /* Maximum delay in seconds */
#define MQTT_RECONNECT_MULTIPLIER 2     /* Exponential backoff multiplier */

/* MQTT client context */
typedef struct {
    struct mosquitto *mosq;
    const MqttConfig *config;
    volatile int connected;  /* Accessed from callback thread */
    int reconnect_delay;
} MqttClient;

/**
 * Initialize MQTT client library
 * Must be called once before creating clients
 *
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_lib_init(void);

/**
 * Cleanup MQTT client library
 * Call when done using MQTT clients
 */
void mqtt_client_lib_cleanup(void);

/**
 * Create and initialize MQTT client
 *
 * @param client Pointer to client structure
 * @param config Pointer to configuration
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_create(MqttClient *client, const MqttConfig *config);

/**
 * Connect to MQTT broker
 * Sets up LWT (Last Will Testament) for offline status
 *
 * @param client Pointer to client structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_connect(MqttClient *client);

/**
 * Disconnect from MQTT broker
 *
 * @param client Pointer to client structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_disconnect(MqttClient *client);

/**
 * Destroy MQTT client and free resources
 *
 * @param client Pointer to client structure
 */
void mqtt_client_destroy(MqttClient *client);

/**
 * Check if client is connected
 *
 * @param client Pointer to client structure
 * @return 1 if connected, 0 otherwise
 */
int mqtt_client_is_connected(const MqttClient *client);

/**
 * Attempt to reconnect with exponential backoff
 *
 * @param client Pointer to client structure
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_reconnect(MqttClient *client);

/**
 * Process network events (call periodically from main loop)
 *
 * @param client Pointer to client structure
 * @param timeout Timeout in milliseconds
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_loop(MqttClient *client, int timeout);

/**
 * Publish message to topic
 *
 * @param client Pointer to client structure
 * @param topic Topic string (will be prefixed with config topic_prefix)
 * @param payload Message payload
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain flag
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_publish(MqttClient *client, const char *topic,
                               const char *payload, int qos, int retain);

/**
 * Publish raw message to full topic (no prefix added)
 *
 * @param client Pointer to client structure
 * @param full_topic Full topic string
 * @param payload Message payload
 * @param qos Quality of Service
 * @param retain Retain flag
 * @return MQTT_OK on success, error code on failure
 */
MqttStatus mqtt_client_publish_raw(MqttClient *client, const char *full_topic,
                                   const char *payload, int qos, int retain);

#endif /* MQTT_CLIENT_H */
