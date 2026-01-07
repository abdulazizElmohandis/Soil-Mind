#include "mqtt_app.h"
#include <Arduino.h>
#include "../../Hal/MQTT/mqtt_core.h"
#include "../../Hal/Pump/Pump.h"
#include "../../Hal/WIFI/wifi.h"
#include "../../APP_Cfg.h"

#if MQTT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

// MQTT Topics - NodeB specific
#define MQTT_TOPIC_STATUS                  "farm/site1/nodeB/status"
#define MQTT_TOPIC_COMMAND                 "farm/site1/nodeB/control"
#define MQTT_TOPIC_IRRIGATION_DECISION     "farm/site1/nodeA/decision"

// Operation modes
typedef enum {
    MODE_AUTO = 0,
    MODE_MANUAL = 1
} OperationMode_t;

// Static variables for application state
static OperationMode_t currentMode = MODE_AUTO;  // Default to AUTO mode
static int currentDecision = 0;  // 0 = no irrigation, 1 = irrigate
static int lastDecision = 0;
static bool pumpState = false;

// Static variables for MQTT main timing
static bool mqttInitialized = false;
static TickType_t lastStatusPublishTime = 0;

// Forward declarations
static void publishStatus(void);
static void handleIrrigationDecision(int decision);
static void setPumpState(bool state);

// Initialize MQTT Application Module
void MQTT_APP_Init(void)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("MQTT Application (NodeB) Initializing");

    // Register message handlers for subscribed topics
    MQTT_RegisterHandler(MQTT_TOPIC_IRRIGATION_DECISION, MQTT_APP_OnIrrigationDecision);
    MQTT_RegisterHandler(MQTT_TOPIC_COMMAND, MQTT_APP_OnCommand);

    DEBUG_PRINTLN("MQTT Application (NodeB) initialized successfully");
#endif
}

// Subscribe to all application-specific topics
void MQTT_APP_SubscribeTopics(void)
{
#if MQTT_ENABLED == STD_ON
    MQTT_Subscribe(MQTT_TOPIC_IRRIGATION_DECISION, 0);
    MQTT_Subscribe(MQTT_TOPIC_COMMAND, 0);

    DEBUG_PRINTLN("MQTT Application topics subscribed (NodeB)");
#endif
}

// Publish actuator status
void MQTT_APP_PublishStatus(void)
{
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        DEBUG_PRINTLN("MQTT not connected, skipping status publish");
        return;
    }

    // Create status payload
    String statusPayload = "{";
    statusPayload += "\"site\":\"site1\",";
    statusPayload += "\"node\":\"nodeB\",";
    statusPayload += "\"online\":1,";
    statusPayload += "\"mode\":\"";
    statusPayload += (currentMode == MODE_AUTO) ? "AUTO" : "MANUAL";
    statusPayload += "\",";
    statusPayload += "\"current_decision\":" + String(currentDecision) + ",";
    statusPayload += "\"last_decision\":" + String(lastDecision);
    statusPayload += "}";

    // Publish status
    MQTT_Publish(MQTT_TOPIC_STATUS, statusPayload.c_str(), 0, false);

    DEBUG_PRINTLN("Status published: " + statusPayload);
#endif
}

// Initialize MQTT Application and dependencies
void MQTT_APP_Setup(void) {
    Serial.println("=== MQTT APP Setup Starting (NodeB) ===");

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

    // Initialize pump to OFF state
#if PUMP_ENABLED == STD_ON
    Pump_Stop();
#endif
}

// Main MQTT function to be called periodically
void mqtt_main(void) {
    TickType_t currentTick = xTaskGetTickCount();

    if (WIFI_IsConnected() && mqttInitialized) {
        MQTT_Loop();

        // Publish status every 5 seconds
        if (currentTick - lastStatusPublishTime >= pdMS_TO_TICKS(5000)) {
            publishStatus();
            lastStatusPublishTime = currentTick;
        }
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
        Serial.println("MQTT modules initialized successfully (NodeB)");
    }
#endif
}

// WiFi disconnection callback
void onWifiDisconnected(void) {
    Serial.println("WiFi Disconnected!");
    mqttInitialized = false;
}

// Publish status (internal helper)
static void publishStatus(void) {
    MQTT_APP_PublishStatus();
}

// Handle irrigation decision
static void handleIrrigationDecision(int decision) {
    // Only process in AUTO mode
    if (currentMode != MODE_AUTO) {
        DEBUG_PRINTLN("In MANUAL mode, ignoring irrigation decision");
        return;
    }

    lastDecision = currentDecision;
    currentDecision = decision;

    // Control pump based on decision
    if (decision == 1) {
        // Irrigate
        setPumpState(true);
        DEBUG_PRINTLN("AUTO mode: Irrigation decision received - Pump ON");
    } else if (decision == 0) {
        // No irrigation
        setPumpState(false);
        DEBUG_PRINTLN("AUTO mode: No irrigation decision - Pump OFF");
    } else if (decision == -1) {
        // System check - turn off pump
        setPumpState(false);
        DEBUG_PRINTLN("AUTO mode: System check decision - Pump OFF");
    } else {
        // Unknown decision - default to OFF for safety
        setPumpState(false);
        DEBUG_PRINTLN("AUTO mode: Unknown decision - Pump OFF (safety)");
    }

    // Publish updated status
    MQTT_APP_PublishStatus();
}

// Set pump state (internal helper)
static void setPumpState(bool state) {
#if PUMP_ENABLED == STD_ON
    if (state) {
        Pump_Start();
        pumpState = true;
    } else {
        Pump_Stop();
        pumpState = false;
    }
#endif
}

// Handler for irrigation decision messages
void MQTT_APP_OnIrrigationDecision(const char* payload)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("Irrigation decision received: " + String(payload));

    // Parse decision payload
    String payloadStr = String(payload);
    
    // Extract decision value from JSON
    int decisionIndex = payloadStr.indexOf("\"decision\":");
    if (decisionIndex != -1) {
        int decision = payloadStr.substring(decisionIndex + 11).toInt();
        handleIrrigationDecision(decision);
    } else {
        DEBUG_PRINTLN("Error: Could not parse decision from payload");
    }
#endif
}

// Handler for manual control commands
void MQTT_APP_OnCommand(const char* payload)
{
#if MQTT_ENABLED == STD_ON
    DEBUG_PRINTLN("Command received: " + String(payload));

    // Parse command payload
    String payloadStr = String(payload);

    if (payloadStr.indexOf("\"cmd\":\"ON\"") != -1) {
        // Manual ON command
        currentMode = MODE_MANUAL;
        lastDecision = currentDecision;
        currentDecision = 1;
        setPumpState(true);
        DEBUG_PRINTLN("MANUAL mode: Pump turned ON");
        MQTT_APP_PublishStatus();
    }
    else if (payloadStr.indexOf("\"cmd\":\"OFF\"") != -1) {
        // Manual OFF command
        currentMode = MODE_MANUAL;
        lastDecision = currentDecision;
        currentDecision = 0;
        setPumpState(false);
        DEBUG_PRINTLN("MANUAL mode: Pump turned OFF");
        MQTT_APP_PublishStatus();
    }
    else if (payloadStr.indexOf("\"cmd\":\"AUTO\"") != -1) {
        // Switch to AUTO mode
        currentMode = MODE_AUTO;
        DEBUG_PRINTLN("Switched to AUTO mode");
        MQTT_APP_PublishStatus();
    }
    else if (payloadStr.indexOf("\"cmd\":\"MANUAL\"") != -1) {
        // Switch to MANUAL mode (without changing pump state)
        currentMode = MODE_MANUAL;
        DEBUG_PRINTLN("Switched to MANUAL mode");
        MQTT_APP_PublishStatus();
    }
    else {
        DEBUG_PRINTLN("Unknown command: " + String(payload));
    }
#endif
}