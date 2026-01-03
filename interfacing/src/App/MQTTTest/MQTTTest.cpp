#include <Arduino.h>
#include "MQTTTest.h"
#include "../../APP_Cfg.h"
#include "../../Hal/WIFI/wifi.h"
#include "../../Hal/MQTT/mqtt_core.h"
#include "../mqtt_app.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// RTOS Task Configuration
#define MQTT_TASK_CORE 1
#define MQTT_TASK_PRIORITY 2  // Lower than WiFi (3)
#define MQTT_TASK_STACK_SIZE 4096

// Test intervals
#define MQTT_LOOP_INTERVAL_MS 20   // MQTT_Loop() every 20ms as per requirements
#define PUBLISH_INTERVAL_MS 5000   // Publish dummy data every 5 seconds
#define DECISION_INTERVAL_MS 15000 // Publish dummy decisions every 15 seconds

// RTOS task handle
static TaskHandle_t mqttTaskHandle = NULL;
static volatile bool mqttTaskRunning = false;

// Timing variables
static unsigned long lastPublishTime = 0;
static unsigned long lastDecisionTime = 0;
static int messageCount = 0;
static int decisionCount = 0;

// Forward declarations
static void Task_MQTT_Test(void* parameter);
static void publishDummyData(void);
static void publishDummyDecision(void);

static bool mqttInitialized = false;

void MQTTTest_Setup(void) {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== MQTT RTOS Test Starting ===");

#if WIFI_ENABLED == STD_ON
  // Configure WiFi
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

#if MQTT_ENABLED == STD_ON
  Serial.println("Creating MQTT RTOS Task...");

  // Create MQTT RTOS task
  BaseType_t taskCreated = xTaskCreatePinnedToCore(
    Task_MQTT_Test,           // Task function
    "MQTT_Test_Task",         // Task name
    MQTT_TASK_STACK_SIZE,     // Stack size
    NULL,                     // Parameters
    MQTT_TASK_PRIORITY,       // Priority
    &mqttTaskHandle,          // Task handle
    MQTT_TASK_CORE            // Core 1
  );

  if (taskCreated != pdPASS) {
    Serial.println("ERROR: Failed to create MQTT RTOS task!");
    return;
  }

  mqttTaskRunning = true;
  Serial.println("MQTT RTOS Task created successfully on Core " + String(MQTT_TASK_CORE));
#else
  Serial.println("ERROR: MQTT is disabled in APP_Cfg.h");
#endif
}

void MQTTTest_Deinit(void) {
#if MQTT_ENABLED == STD_ON
  if (mqttTaskRunning && mqttTaskHandle != NULL) {
    Serial.println("Stopping MQTT RTOS Task...");
    vTaskDelete(mqttTaskHandle);
    mqttTaskHandle = NULL;
    mqttTaskRunning = false;
    Serial.println("MQTT RTOS Task stopped");
  }
#endif

#if WIFI_ENABLED == STD_ON
  WIFI_Deinit();
#endif
}

void onWifiConnected(void)
{
    Serial.println("WiFi Connected! Initializing MQTT modules...");

#if MQTT_ENABLED == STD_ON
    if (!mqttInitialized)
    {
        // Initialize MQTT Core
        MQTT_Config_t mqttConfig = {
            .broker = MQTT_BROKER,
            .port = MQTT_PORT,
            .username = (strlen(MQTT_USERNAME) > 0) ? MQTT_USERNAME : NULL,
            .password = (strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : NULL
        };

        MQTT_Init(&mqttConfig);

        // Initialize MQTT Application
        MQTT_APP_Init();

        // Subscribe to topics
        MQTT_APP_SubscribeTopics();

        mqttInitialized = true;
        Serial.println("MQTT modules initialized successfully");
    }
#endif
}

void onWifiDisconnected(void)
{
    Serial.println("WiFi Disconnected!");
    mqttInitialized = false;
}

// RTOS Task: MQTT Test Task - runs MQTT loop and publishes dummy data
static void Task_MQTT_Test(void* parameter)
{
    Serial.println("MQTT RTOS Task started on core: " + String(xPortGetCoreID()));

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t mqttLoopInterval = pdMS_TO_TICKS(MQTT_LOOP_INTERVAL_MS);

    for(;;)
    {
        // Wait for WiFi to be connected and MQTT to be initialized
        if (WIFI_IsConnected() && mqttInitialized)
        {
            // Call MQTT loop every 20ms as per requirements
            MQTT_Loop();

            // Check if it's time to publish dummy telemetry
            if (millis() - lastPublishTime >= PUBLISH_INTERVAL_MS)
            {
                publishDummyData();
                lastPublishTime = millis();
            }

            // Check if it's time to publish dummy decision
            if (millis() - lastDecisionTime >= DECISION_INTERVAL_MS)
            {
                publishDummyDecision();
                lastDecisionTime = millis();
            }
        }
        else
        {
            // Print status periodically when not connected
            static TickType_t lastStatusPrint = 0;
            TickType_t currentTick = xTaskGetTickCount();
            if ((currentTick - lastStatusPrint) >= pdMS_TO_TICKS(2000))
            {
                if (!WIFI_IsConnected())
                {
                    Serial.println("MQTT Task: Waiting for WiFi connection...");
                }
                else if (!mqttInitialized)
                {
                    Serial.println("MQTT Task: Waiting for MQTT initialization...");
                }
                lastStatusPrint = currentTick;
            }
        }

        // Delay for next MQTT loop iteration
        vTaskDelayUntil(&xLastWakeTime, mqttLoopInterval);
    }
}

// Publish dummy telemetry data
static void publishDummyData(void)
{
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        Serial.println("MQTT not connected, skipping telemetry publish");
        return;
    }

    messageCount++;

    // Use application layer to publish telemetry (which generates dummy data)
    MQTT_APP_PublishTelemetry();

    // Print status
    Serial.println("Dummy telemetry published #" + String(messageCount) +
                   " | RSSI: " + String(WIFI_GetRSSI()) + " dBm");
#endif
}

// Publish dummy irrigation decision
static void publishDummyDecision(void)
{
#if MQTT_ENABLED == STD_ON
    if (!MQTT_IsConnected())
    {
        Serial.println("MQTT not connected, skipping decision publish");
        return;
    }

    decisionCount++;

    // Cycle through different decision types for testing
    Decision_t decision;
    switch (decisionCount % 3)
    {
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

    // Use application layer to publish decision
    MQTT_APP_PublishDecision(decision);

    // Print status
    Serial.println("Dummy decision published #" + String(decisionCount));
#endif
}
