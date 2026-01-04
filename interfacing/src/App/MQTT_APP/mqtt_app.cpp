#include "mqtt_app.h"
#include <Arduino.h>
#include "../../Hal/MQTT/mqtt_core.h"
#include "../../App/SoilMoisture/SoilMoisture.h"
#include "../../App/DHT/DHT11.h"
#include "../../Hal/Pump/Pump.h"
#include "../../Hal/WIFI/wifi.h"
#include "../../APP_Cfg.h"

#if MQTT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

// MQTT Topics - Application-specific, defined here in the app module
#define MQTT_TOPIC_TELEMETRY        "farm/site1/nodeA/telemetry"
#define MQTT_TOPIC_STATUS           "farm/site1/nodeA/status"
#define MQTT_TOPIC_COMMAND          "farm/site1/nodeA/cmd"

// Static variables for application state
static unsigned long lastTelemetryTime = 0;
static uint32_t messageCount = 0;

// Static variables for MQTT main timing
static bool mqttInitialized = false;
static TickType_t lastPublishTime = 0;
static TickType_t lastDecisionTime = 0;
static int dummyMessageCount = 0;
static int decisionCount = 0;

// Forward declarations
static void publishSoilMoistureTelemetry(void);
static void publishHeartbeat(void);
static void publishDummyData(void);
static void publishDummyDecision(void);

// Initialize MQTT Application Module
void MQTT_APP_Init(void)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Application Initializing");

    // Register message handlers for subscribed topics
    MQTT_RegisterHandler(MQTT_TOPIC_PUMP_CONTROL, MQTT_APP_OnPumpCommand);

    DEBUG_PRINTLN("MQTT Application initialized successfully");
#endif
}

// Subscribe to all application-specific topics
void MQTT_APP_SubscribeTopics(void)
{
#if MQTT_ENABLED == STD_ON
    MQTT_Subscribe(MQTT_TOPIC_PUMP_CONTROL, 0);

    DEBUG_PRINTLN("MQTT Application topics subscribed");
#endif
}

// Publish telemetry data
void MQTT_APP_PublishTelemetry(void)
{
#if MQTT_ENABLED == STD_ON && SOILMOISTURE_ENABLED == STD_ON && DHT11_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        DEBUG_PRINTLN("MQTT not connected, skipping telemetry publish");
        return;
    }

    messageCount++;

    // Get sensor readings from queues
    uint8_t soilMoistureRaw = 0;
    SoilMoisture_getMoisture(&soilMoistureRaw);
    float soilMoisture = (float)soilMoistureRaw;

    float temperature = 0.0f;
    DHT11_GetTemperature(&temperature);

    float humidity = 0.0f;
    DHT11_GetHumidity(&humidity);

    // Create telemetry payload
    String telemetryPayload = "{";
    telemetryPayload += "\"site\":\"site1\",";
    telemetryPayload += "\"node\":\"nodeA\",";
    telemetryPayload += "\"soil_moisture\":" + String(soilMoisture, 1) + ",";
    telemetryPayload += "\"temperature\":" + String((int)temperature) + ",";
    telemetryPayload += "\"humidity\":" + String(humidity);
    // Unimplemented sensors
    // telemetryPayload += ",\"ph\":0.0,";
    // telemetryPayload += "\"n\":0.0,";
    // telemetryPayload += "\"p\":0.0,";
    // telemetryPayload += "\"k\":0.0";
    telemetryPayload += "}";

    // Publish telemetry
    MQTT_Publish(MQTT_TOPIC_TELEMETRY, telemetryPayload.c_str(), 0, false);

    DEBUG_PRINTLN("Telemetry published: " + telemetryPayload);
#endif
}

// Publish irrigation decision
void MQTT_APP_PublishDecision(Decision_t decision)
{
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        DEBUG_PRINTLN("MQTT not connected, skipping decision publish");
        return;
    }

    // Publish command based on decision
    String commandPayload = "{";
    if (decision == DECISION_IRRIGATE)
    {
        commandPayload += "\"cmd\":\"ON\"";
    }
    else
    {
        commandPayload += "\"cmd\":\"OFF\"";
    }
    commandPayload += "}";

    MQTT_Publish(MQTT_TOPIC_COMMAND, commandPayload.c_str(), 0, false);
    DEBUG_PRINTLN("Command published: " + commandPayload);

    // Create decision payload (legacy, can be removed later)
    String decisionPayload = "{";
    decisionPayload += "\"timestamp\":" + String(millis()) + ",";
    decisionPayload += "\"decision\":";

    switch (decision)
    {
        case DECISION_IRRIGATE:
            decisionPayload += "\"IRRIGATE\"";
            break;
        case DECISION_NO_IRRIGATION:
            decisionPayload += "\"NO_IRRIGATION\"";
            break;
        case DECISION_CHECK_SYSTEM:
            decisionPayload += "\"CHECK_SYSTEM\"";
            break;
        default:
            decisionPayload += "\"UNKNOWN\"";
            break;
    }

    decisionPayload += "}";

    // Publish decision
    MQTT_Publish(MQTT_TOPIC_IRRIGATION_DECISION, decisionPayload.c_str(), 0, false);

    DEBUG_PRINTLN("Decision published: " + decisionPayload);
#endif
}

// Initialize MQTT Application and dependencies
void MQTT_APP_Setup(void) {
    Serial.println("=== MQTT APP Setup Starting ===");

#if WIFI_ENABLED == STD_ON
    WIFI_Config_t wifiConfig = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .reconnect_interval_ms = WIFI_RECONNECT_INTERVAL_MS,
        .on_connect = onWifiConnected,
        .on_disconnect = onWifiDisconnected
    };

    Serial.println("Initializing WiFi...");
    Serial.println("SSID: " + String(WIFI_SSID));
    WIFI_Init(&wifiConfig);
#else
    Serial.println("ERROR: WiFi is disabled in APP_Cfg.h");
#endif
}

// Main MQTT function to be called periodically
void mqtt_main(void) {
    TickType_t currentTick = xTaskGetTickCount();

    if (WIFI_IsConnected() && mqttInitialized) {
        MQTT_Loop();

        if (currentTick - lastPublishTime >= pdMS_TO_TICKS(5000)) {
            publishHeartbeat();
            lastPublishTime = currentTick;
        }

        // Decision publishing commented out since ML task not implemented
        // if (currentTick - lastDecisionTime >= pdMS_TO_TICKS(15000)) {
        //     publishDummyDecision();
        //     lastDecisionTime = currentTick;
        // }
    } else {
        // Print status periodically when not connected
        static TickType_t lastStatusPrint = 0;
        if (currentTick - lastStatusPrint >= pdMS_TO_TICKS(2000)) {
            if (!WIFI_IsConnected()) {
                Serial.println("mqtt_main: Waiting for WiFi connection...");
            } else if (!mqttInitialized) {
                Serial.println("mqtt_main: Waiting for MQTT initialization...");
            }
            lastStatusPrint = currentTick;
        }
    }
}

// WiFi connection callback
void onWifiConnected(void) {
    Serial.println("WiFi Connected! Initializing MQTT modules...");

#if MQTT_ENABLED == STD_ON
    if (!mqttInitialized) {
        MQTT_Config_t mqttConfig = {
            .broker = MQTT_BROKER,
            .port = MQTT_PORT,
            .username = (strlen(MQTT_USERNAME) > 0) ? MQTT_USERNAME : NULL,
            .password = (strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : NULL
        };

        MQTT_Init(&mqttConfig);

        MQTT_APP_Init();

        MQTT_APP_SubscribeTopics();

        mqttInitialized = true;
        Serial.println("MQTT modules initialized successfully");
    }
#endif
}

// WiFi disconnection callback
void onWifiDisconnected(void) {
    Serial.println("WiFi Disconnected!");
    mqttInitialized = false;
}

// Publish dummy telemetry data
static void publishDummyData(void) {
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected()) {
        Serial.println("MQTT not connected, skipping telemetry publish");
        return;
    }

    dummyMessageCount++;

    MQTT_APP_PublishTelemetry();

    Serial.println("Dummy telemetry published #" + String(dummyMessageCount) +
                   " | RSSI: " + String(WIFI_GetRSSI()) + " dBm");
#endif
}

// Publish heartbeat/status
static void publishHeartbeat(void) {
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected()) {
        Serial.println("MQTT not connected, skipping heartbeat publish");
        return;
    }

    String heartbeatPayload = "{";
    heartbeatPayload += "\"site\":\"site1\",";
    heartbeatPayload += "\"node\":\"nodeA\",";
    heartbeatPayload += "\"online\":true";
    heartbeatPayload += "}";

    MQTT_Publish(MQTT_TOPIC_STATUS, heartbeatPayload.c_str(), 0, false);

    DEBUG_PRINTLN("Heartbeat published: " + heartbeatPayload);
#endif
}

// Publish dummy irrigation decision
static void publishDummyDecision(void) {
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected()) {
        Serial.println("MQTT not connected, skipping decision publish");
        return;
    }

    decisionCount++;

    Decision_t decision;
    switch (decisionCount % 3) {
        case 0:
            decision = DECISION_IRRIGATE;
            break;
        case 1:
            decision = DECISION_NO_IRRIGATION;
            break;
        case 2:
            decision = DECISION_CHECK_SYSTEM;
            break;
        default:
            decision = DECISION_IRRIGATE;
            break;
    }

    MQTT_APP_PublishDecision(decision);

    Serial.println("Dummy decision published #" + String(decisionCount));
#endif
}

//// Handler for general commands (removed since we publish commands instead of subscribing)
// void MQTT_APP_OnCommand(const char* payload)
// {
// #if MQTT_ENABLED == STD_ON
//     DEBUG_PRINTLN("Command received: " + String(payload));

//     // Parse command payload and execute business logic
//     String payloadStr = String(payload);

//     if (payloadStr.indexOf("\"cmd\":\"ON\"") != -1)
//     {
//         // Turn irrigation on
//         Pump_Start();
//         DEBUG_PRINTLN("Irrigation turned ON");
//     }
//     else if (payloadStr.indexOf("\"cmd\":\"OFF\"") != -1)
//     {
//         // Turn irrigation off
//         Pump_Stop();
//         DEBUG_PRINTLN("Irrigation turned OFF");
//     }
//     else if (strcmp(payload, "ping") == 0)
//     {
//         // Respond to ping
//         MQTT_Publish("farm/site1/nodeA/response", "pong", 0, false);
//     }
//     else if (strcmp(payload, "status") == 0)
//     {
//         // Publish current status
//         MQTT_APP_PublishTelemetry();
//     }
//     else if (strcmp(payload, "irrigate_now") == 0)
//     {
//         // Force irrigation decision
//         MQTT_APP_PublishDecision(DECISION_IRRIGATE);
//     }
//     else
//     {
//         DEBUG_PRINTLN("Unknown command: " + String(payload));
//     }
// #endif
// }

// Handler for pump control commands
void MQTT_APP_OnPumpCommand(const char* payload)
{
#if MQTT_ENABLED == STD_ON && PUMP_ENABLED == STD_ON
    DEBUG_PRINTLN("Pump command received: " + String(payload));

    // Parse pump control commands and execute hardware control
    if (strcmp(payload, "ON") == 0 || strcmp(payload, "on") == 0)
    {
        // Turn pump on
        Pump_Start();
        DEBUG_PRINTLN("Pump turned ON");
    }
    else if (strcmp(payload, "OFF") == 0 || strcmp(payload, "off") == 0)
    {
        // Turn pump off
        Pump_Stop();
        DEBUG_PRINTLN("Pump turned OFF");
    }
    else if (strcmp(payload, "STATUS") == 0 || strcmp(payload, "status") == 0)
    {
        // Report pump status
        String statusPayload = "{\"pumpStatus\":\"";
        // TODO: Get actual pump status
        statusPayload += "unknown";
        statusPayload += "\"}";

        MQTT_Publish("farm/site1/nodeA/pump_response", statusPayload.c_str(), 0, false);
    }
    else
    {
        DEBUG_PRINTLN("Unknown pump command: " + String(payload));
    }
#endif
}
