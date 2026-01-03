#include "mqtt_app.h"
#include <Arduino.h>
#include "../Hal/MQTT/mqtt_core.h"
#include "../Hal/SoilMoisture/SoilMoisture.h"
#include "../Hal/Pump/Pump.h"
#include "../APP_Cfg.h"

#if MQTT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

// MQTT Topics - Application-specific, defined here in the app module
#define MQTT_TOPIC_TELEMETRY        "farm/site1/nodeA/telemetry"
#define MQTT_TOPIC_IRRIGATION_DECISION "farm/site1/nodeA/decision"
#define MQTT_TOPIC_PUMP_CONTROL     "farm/site1/nodeB/status"
#define MQTT_TOPIC_COMMAND          "farm/site1/nodeA/command"

// Static variables for application state
static unsigned long lastTelemetryTime = 0;
static uint32_t messageCount = 0;

// Forward declarations
static void publishSoilMoistureTelemetry(void);

// Initialize MQTT Application Module
void MQTT_APP_Init(void)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Application Initializing");

    // Register message handlers for subscribed topics
    MQTT_RegisterHandler(MQTT_TOPIC_COMMAND, MQTT_APP_OnCommand);
    MQTT_RegisterHandler(MQTT_TOPIC_PUMP_CONTROL, MQTT_APP_OnPumpCommand);

    DEBUG_PRINTLN("MQTT Application initialized successfully");
#endif
}

// Subscribe to all application-specific topics
void MQTT_APP_SubscribeTopics(void)
{
#if MQTT_ENABLED == STD_ON
    MQTT_Subscribe(MQTT_TOPIC_COMMAND, 0);
    MQTT_Subscribe(MQTT_TOPIC_PUMP_CONTROL, 0);

    DEBUG_PRINTLN("MQTT Application topics subscribed");
#endif
}

// Publish telemetry data
void MQTT_APP_PublishTelemetry(void)
{
#if MQTT_ENABLED == STD_ON && SOILMOISTURE_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        DEBUG_PRINTLN("MQTT not connected, skipping telemetry publish");
        return;
    }

    messageCount++;

    // Get sensor readings
    float soilMoisture = SoilMoisture_ReadPercentage();

    // Create telemetry payload
    String telemetryPayload = "{";
    telemetryPayload += "\"timestamp\":" + String(millis()) + ",";
    telemetryPayload += "\"messageCount\":" + String(messageCount) + ",";
    telemetryPayload += "\"soilMoisture\":" + String(soilMoisture, 1) + ",";
    telemetryPayload += "\"pumpStatus\":";

    // Get pump status (assuming Pump module has a status function)
    // For now, using a placeholder
    telemetryPayload += "\"unknown\"";  // TODO: Implement pump status reading

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

    // Create decision payload
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

// Handler for general commands
void MQTT_APP_OnCommand(const char* payload)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("Command received: " + String(payload));

    // Parse command payload and execute business logic
    // Example: JSON parsing for commands like "ping", "status", etc.

    if (strcmp(payload, "ping") == 0)
    {
        // Respond to ping
        MQTT_Publish("farm/site1/nodeA/response", "pong", 0, false);
    }
    else if (strcmp(payload, "status") == 0)
    {
        // Publish current status
        MQTT_APP_PublishTelemetry();
    }
    else if (strcmp(payload, "irrigate_now") == 0)
    {
        // Force irrigation decision
        MQTT_APP_PublishDecision(DECISION_IRRIGATE);
    }
    else
    {
        DEBUG_PRINTLN("Unknown command: " + String(payload));
    }
#endif
}

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
