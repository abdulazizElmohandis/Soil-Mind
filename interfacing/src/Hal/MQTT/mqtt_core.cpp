#include "mqtt_core.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include "../WIFI/wifi.h"
#include "../../APP_Cfg.h"

#if MQTT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

// Internal MQTT client
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

// Configuration storage
static MQTT_Config_t g_config;

// Subscription and handler management
#define MAX_SUBSCRIPTIONS 10
#define MAX_HANDLERS 10

typedef struct {
    char topic[128];
    uint8_t qos;
    bool active;
} Subscription_t;

typedef struct {
    char topic[128];
    MQTT_MessageHandler_t handler;
    bool active;
} Handler_t;

static Subscription_t subscriptions[MAX_SUBSCRIPTIONS];
static Handler_t handlers[MAX_HANDLERS];
static uint8_t subscriptionCount = 0;
static uint8_t handlerCount = 0;

// Forward declarations
static void MQTT_Reconnect(void);
static void mqttCallback(char* topic, byte* payload, unsigned int length);
static void resubscribeAll(void);

// Initialize MQTT Core Module
void MQTT_Init(const MQTT_Config_t* cfg)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Core Initializing");

    // Store configuration
    g_config = *cfg;

    // Initialize PubSubClient
    mqttClient.setServer(g_config.broker, g_config.port);
    mqttClient.setCallback(mqttCallback);

    // Initialize tables
    memset(subscriptions, 0, sizeof(subscriptions));
    memset(handlers, 0, sizeof(handlers));
    subscriptionCount = 0;
    handlerCount = 0;

    DEBUG_PRINTLN("MQTT Core initialized successfully");
#endif
}

// Main MQTT loop - non-blocking, designed for RTOS task
void MQTT_Loop(void)
{
#if MQTT_ENABLED == STD_ON
    // Only process MQTT if WiFi is connected
    if (WIFI_IsConnected())
    {
        if (!mqttClient.connected())
        {
            MQTT_Reconnect();
        }
        mqttClient.loop();
    }
#endif
}

// Check MQTT connection status
bool MQTT_IsConnected(void)
{
#if MQTT_ENABLED == STD_ON
    return mqttClient.connected();
#else
    return false;
#endif
}

// Publish message to topic
bool MQTT_Publish(const char* topic, const char* payload, uint8_t qos, bool retain)
{
#if MQTT_ENABLED == STD_ON
    if (!WIFI_IsConnected() || !mqttClient.connected())
    {
        DEBUG_PRINTLN("MQTT publish failed: Not connected");
        return false;
    }

    bool result = mqttClient.publish(topic, payload, retain);
    if (result)
    {
        DEBUG_PRINTLN("Published to " + String(topic) + ": " + String(payload));
    }
    else
    {
        DEBUG_PRINTLN("MQTT publish failed");
    }
    return result;
#else
    return false;
#endif
}

// Subscribe to a topic
bool MQTT_Subscribe(const char* topic, uint8_t qos)
{
#if MQTT_ENABLED == STD_ON
    if (subscriptionCount >= MAX_SUBSCRIPTIONS)
    {
        DEBUG_PRINTLN("MQTT subscription limit reached");
        return false;
    }

    // Check if already subscribed
    for (uint8_t i = 0; i < subscriptionCount; i++)
    {
        if (strcmp(subscriptions[i].topic, topic) == 0)
        {
            DEBUG_PRINTLN("Already subscribed to: " + String(topic));
            return true; // Already subscribed
        }
    }

    // Add to subscription table
    strcpy(subscriptions[subscriptionCount].topic, topic);
    subscriptions[subscriptionCount].qos = qos;
    subscriptions[subscriptionCount].active = true;
    subscriptionCount++;

    // Subscribe immediately if connected
    if (mqttClient.connected())
    {
        bool result = mqttClient.subscribe(topic, qos);
        if (result)
        {
            DEBUG_PRINTLN("Subscribed to: " + String(topic));
        }
        return result;
    }

    DEBUG_PRINTLN("Topic queued for subscription: " + String(topic));
    return true;
#else
    return false;
#endif
}

// Register a message handler for a topic
bool MQTT_RegisterHandler(const char* topic, MQTT_MessageHandler_t handler)
{
#if MQTT_ENABLED == STD_ON
    if (handlerCount >= MAX_HANDLERS)
    {
        DEBUG_PRINTLN("MQTT handler limit reached");
        return false;
    }

    // Check if handler already registered for this topic
    for (uint8_t i = 0; i < handlerCount; i++)
    {
        if (strcmp(handlers[i].topic, topic) == 0)
        {
            DEBUG_PRINTLN("Handler already registered for: " + String(topic));
            handlers[i].handler = handler; // Update handler
            return true;
        }
    }

    // Add to handler table
    strcpy(handlers[handlerCount].topic, topic);
    handlers[handlerCount].handler = handler;
    handlers[handlerCount].active = true;
    handlerCount++;

    DEBUG_PRINTLN("Handler registered for: " + String(topic));
    return true;
#else
    return false;
#endif
}

// Internal: MQTT callback function - dispatches to registered handlers
static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
#if MQTT_ENABLED == STD_ON
    // Safely copy payload
    char buffer[256];
    if (length >= sizeof(buffer)) length = sizeof(buffer) - 1;
    for (unsigned int i = 0; i < length; i++) buffer[i] = (char)payload[i];
    buffer[length] = '\0';

    DEBUG_PRINTLN("MQTT Message received - Topic: " + String(topic) + ", Payload: " + String(buffer));

    // Dispatch to registered handlers
    for (uint8_t i = 0; i < handlerCount; i++)
    {
        if (handlers[i].active && strcmp(handlers[i].topic, topic) == 0)
        {
            if (handlers[i].handler != NULL)
            {
                handlers[i].handler(buffer);
                break; // Only one handler per topic
            }
        }
    }
#endif
}

// Internal: Reconnect to MQTT broker
static void MQTT_Reconnect(void)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Reconnecting...");
    while (!mqttClient.connected())
    {
        if (!WIFI_IsConnected())
        {
            delay(1000); // Wait until WiFi reconnects
            continue;
        }

        String clientId = "ESP32-SoilMind-" + String(random(0xffff), HEX);
        bool connected = false;

        // Connect with or without authentication
        if (g_config.username != NULL && strlen(g_config.username) > 0 &&
            g_config.password != NULL && strlen(g_config.password) > 0)
        {
            connected = mqttClient.connect(clientId.c_str(), g_config.username, g_config.password);
        }
        else
        {
            connected = mqttClient.connect(clientId.c_str());
        }

        if (connected)
        {
            DEBUG_PRINTLN("MQTT Connected with ID: " + clientId);
            resubscribeAll();
        }
        else
        {
            DEBUG_PRINTLN("MQTT Connection failed, retrying in 2 seconds...");
            delay(2000);
        }
    }
#endif
}

// Internal: Resubscribe to all active topics after reconnection
static void resubscribeAll(void)
{
#if MQTT_ENABLED == STD_ON
    for (uint8_t i = 0; i < subscriptionCount; i++)
    {
        if (subscriptions[i].active)
        {
            bool result = mqttClient.subscribe(subscriptions[i].topic, subscriptions[i].qos);
            if (result)
            {
                DEBUG_PRINTLN("Resubscribed to: " + String(subscriptions[i].topic));
            }
            else
            {
                DEBUG_PRINTLN("Failed to resubscribe to: " + String(subscriptions[i].topic));
            }
        }
    }
#endif
}
