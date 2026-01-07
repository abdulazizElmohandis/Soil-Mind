
#include "MQTT.h"
#include <WiFi.h>
#include <string.h>
#include "../WIFI/wifi.h"
#include "../../APP_Cfg.h"

#if MQTT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static const char* g_broker;
static int g_port;
static const char* g_username = NULL;
static const char* g_password = NULL;

static void MQTT_Reconnect(void);

static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
#if MQTT_ENABLED == STD_ON
    char buffer[256];
    if (length >= sizeof(buffer)) length = sizeof(buffer) - 1;
    for (unsigned int i = 0; i < length; i++) buffer[i] = (char)payload[i];
    buffer[length] = '\0';
    
    DEBUG_PRINTLN("MQTT Message received - Topic: " + String(topic) + ", Payload: " + String(buffer));
    
    // Handle soil monitoring commands
    if (strcmp(topic, MQTT_TOPIC_PUMP_CONTROL) == 0) {
        // Handle pump control commands
        DEBUG_PRINTLN("Pump control command: " + String(buffer));
    }
    else if (strcmp(topic, MQTT_TOPIC_IRRIGATION_DECISION) == 0) {
        // Handle irrigation decision commands
        DEBUG_PRINTLN("Irrigation decision: " + String(buffer));
    }
#endif
}

void MQTT_Init(const char* broker, int port, const char* username, const char* password)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Initializing");
    g_broker = broker;
    g_port = port;
    g_username = username;
    g_password = password;

    mqttClient.setServer(g_broker, g_port);
    mqttClient.setCallback(mqttCallback);
    DEBUG_PRINTLN("MQTT Broker: " + String(broker) + ", Port: " + String(port));
    if (username != NULL && password != NULL) {
        DEBUG_PRINTLN("MQTT Authentication: Enabled (Username: " + String(username) + ")");
    } else {
        DEBUG_PRINTLN("MQTT Authentication: Disabled");
    }
#endif
}

void MQTT_Loop(void)
{
#if MQTT_ENABLED == STD_ON
    // Only try MQTT if WiFi is connected
    if (WIFI_IsConnected())
    {
        if (!mqttClient.connected()) MQTT_Reconnect();
        mqttClient.loop();
    }
#endif
}

void MQTT_SubscribeAll(void)
{
#if MQTT_ENABLED == STD_ON
    mqttClient.subscribe(MQTT_TOPIC_PUMP_CONTROL);
    mqttClient.subscribe(MQTT_TOPIC_IRRIGATION_DECISION);
    DEBUG_PRINTLN("MQTT Subscribed to all topics");
#endif
}


void MQTT_Publish(const char* topic, const char* payload)
{
#if MQTT_ENABLED == STD_ON
    if (!WIFI_IsConnected() || !mqttClient.connected()) 
    {
        DEBUG_PRINTLN("MQTT publish failed: Not connected");
        return;
    }
    
    if (mqttClient.publish(topic, payload))
    {
        DEBUG_PRINTLN("Published to " + String(topic) + ": " + String(payload));
    }
    else
    {
        DEBUG_PRINTLN("MQTT publish failed");
    }
#endif
}

bool MQTT_IsConnected(void)
{
#if MQTT_ENABLED == STD_ON
    return mqttClient.connected();
#else
    return false;
#endif
}

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

        String id = "ESP32-SoilMind-" + String(random(0xffff), HEX);
        bool connected = false;
        
        // Connect with or without authentication
        if (g_username != NULL && g_password != NULL) {
            connected = mqttClient.connect(id.c_str(), g_username, g_password);
        } else {
            connected = mqttClient.connect(id.c_str());
        }
        
        if (connected)
        {
            DEBUG_PRINTLN("MQTT Connected with ID: " + id);
            MQTT_SubscribeAll();
        }
        else
        {
            DEBUG_PRINTLN("MQTT Connection failed, retrying in 2 seconds...");
            delay(2000);
        }
    }
#endif
}
