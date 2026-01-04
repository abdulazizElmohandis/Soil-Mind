#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "src/Hal/WIFI/wifi.h"
#include "src/App/MQTT_APP/mqtt_app.h"

// ============================================================================
// APP TASK CONFIGURATION
// ============================================================================
<<<<<<< HEAD
// Uncomment the line below to enable MQTT testing

=======
#define APP_TASK_CORE 0
#define APP_TASK_PRIORITY 3  // Higher than MQTT (2)
#define APP_TASK_STACK_SIZE 3072

// RTOS task handles
static TaskHandle_t appTaskHandle = NULL;
static TaskHandle_t mqttTaskHandle = NULL;

// ============================================================================
// APP TASK FUNCTION
// ============================================================================
static void appTask100ms(void* parameter)
{
    Serial.println("appTask100ms started on core: " + String(xPortGetCoreID()));

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms cycle

    for(;;)
    {
        // Call WiFi loop
        wifi_loop();

        // Debug: Print WiFi status every second
        static TickType_t lastDebugTime = 0;
        TickType_t currentTick = xTaskGetTickCount();
        if ((currentTick - lastDebugTime) >= pdMS_TO_TICKS(1000)) {
            Serial.println("appTask100ms: WiFi status check - Connected: " + String(WIFI_IsConnected() ? "YES" : "NO"));
            lastDebugTime = currentTick;
        }

        // Delay for next iteration
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// MQTT TASK FUNCTION
// ============================================================================
static void apptask_400ms(void* parameter)
{
    Serial.println("apptask_400ms started on core: " + String(xPortGetCoreID()));

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(400); // 400ms cycle

    for(;;)
    {
        // Call MQTT main
        mqtt_main();

        // Delay for next iteration
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
>>>>>>> 7b80536acc4f45c19dbaadc8a103e142d53c3769

// ============================================================================
// SETUP
// ============================================================================
void setup() {
<<<<<<< HEAD
  
=======
  Serial.begin(115200);
  delay(1000);
  // TODO: Add your application initialization here

  // Initialize MQTT APP
  MQTT_APP_Setup();

  // Create appTask100ms on Core 0
  Serial.println("Creating appTask100ms...");
  BaseType_t taskCreated = xTaskCreatePinnedToCore(
    appTask100ms,              // Task function
    "appTask100ms",            // Task name
    APP_TASK_STACK_SIZE,       // Stack size
    NULL,                      // Parameters
    APP_TASK_PRIORITY,         // Priority (higher than MQTT)
    &appTaskHandle,            // Task handle
    APP_TASK_CORE              // Core 0
  );

  if (taskCreated != pdPASS) {
    Serial.println("ERROR: Failed to create appTask100ms!");
  } else {
    Serial.println("appTask100ms created successfully on Core " + String(APP_TASK_CORE));
  }

  // Create apptask_400ms on Core 1
  Serial.println("Creating apptask_400ms...");
  taskCreated = xTaskCreatePinnedToCore(
    apptask_400ms,             // Task function
    "apptask_400ms",           // Task name
    APP_TASK_STACK_SIZE,       // Stack size
    NULL,                      // Parameters
    2,                         // Priority (lower than WiFi)
    &mqttTaskHandle,           // Task handle
    1                          // Core 1
  );

  if (taskCreated != pdPASS) {
    Serial.println("ERROR: Failed to create apptask_400ms!");
  } else {
    Serial.println("apptask_400ms created successfully on Core 1");
  }
>>>>>>> 7b80536acc4f45c19dbaadc8a103e142d53c3769
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  // TODO: Add your application main loop here
<<<<<<< HEAD
  
  // Uncomment to test MQTT (requires MQTTTest.h to be included above)

=======

  // MQTT RTOS Test runs automatically in background task
  // No need to call MQTTTest_Loop() anymore

  // Add a small delay to prevent busy waiting
  delay(1000);

  // Optional: Add periodic status output or other main loop tasks
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime >= 10000) { // Every 10 seconds
    Serial.println("Main loop: MQTT RTOS test running in background...");
    lastStatusTime = millis();
  }
>>>>>>> 7b80536acc4f45c19dbaadc8a103e142d53c3769
}

// ============================================================================
// CLEANUP (Optional - for testing)
// ============================================================================
// Uncomment the function below if you want to test deinitialization
/*
void cleanup() {
  // Stop appTask100ms
  if (appTaskHandle != NULL) {
    Serial.println("Stopping appTask100ms...");
    vTaskDelete(appTaskHandle);
    appTaskHandle = NULL;
    Serial.println("appTask100ms stopped");
  }

  // Stop apptask_400ms
  if (mqttTaskHandle != NULL) {
    Serial.println("Stopping apptask_400ms...");
    vTaskDelete(mqttTaskHandle);
    mqttTaskHandle = NULL;
    Serial.println("apptask_400ms stopped");
  }

  // TODO: Add MQTT_APP_Deinit if needed
}
*/
