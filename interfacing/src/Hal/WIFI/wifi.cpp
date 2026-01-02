#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include "../../APP_Cfg.h"
#include "wifi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#if MQTT_ENABLED == STD_ON
#include "../MQTT/MQTT.h"
#endif

#if WIFI_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

// Pin WiFi task to Core 0 (same as WiFi driver)
#define WIFI_TASK_CORE 0
#define WIFI_TASK_PRIORITY 3
#define WIFI_TASK_STACK_SIZE 3072

// RTOS synchronization
static SemaphoreHandle_t g_wifiMutex = NULL;
static SemaphoreHandle_t g_mqttInitMutex = NULL;
static TaskHandle_t g_wifiTaskHandle = NULL;
static volatile bool g_wifiTaskRunning = false;

static volatile bool mqttInitialized = false;

void onWifiConnected(void)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Connected! IP: " + WiFi.localIP().toString());
    
#if MQTT_ENABLED == STD_ON
    if (xSemaphoreTake(g_mqttInitMutex, portMAX_DELAY) == pdTRUE) {
        if (!mqttInitialized) {
            const char* username = (strlen(MQTT_USERNAME) > 0) ? MQTT_USERNAME : NULL;
            const char* password = (strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : NULL;
            MQTT_Init(MQTT_BROKER, MQTT_PORT, username, password);
            mqttInitialized = true;
        }
        xSemaphoreGive(g_mqttInitMutex);
    }
#endif
#endif
}

void onWifiDisconnected(void)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Disconnected!");
#endif
}

static WIFI_Config_t g_wifiCfg = {
    .ssid = WIFI_SSID,
    .password = WIFI_PASSWORD,
    .reconnect_interval_ms = WIFI_RECONNECT_INTERVAL_MS,
    .on_connect = onWifiConnected,
    .on_disconnect = onWifiDisconnected
};

static WIFI_Status_t g_wifiStatus = WIFI_STATUS_DISCONNECTED;
static TickType_t g_lastReconnectAttempt = 0;
static TickType_t g_connectStartTime = 0;

#define WIFI_CONNECT_TIMEOUT_TICKS pdMS_TO_TICKS(15000)
#define WIFI_STABILITY_CHECK_TICKS pdMS_TO_TICKS(500)

static void WIFI_StartConnection(void)
{
#if WIFI_ENABLED == STD_ON
    if (g_wifiCfg.ssid == NULL || g_wifiCfg.password == NULL)
    {
        g_wifiStatus = WIFI_STATUS_ERROR;
        return;
    }

    WiFi.disconnect(false, false);
    // Note: Small delay handled by WiFi library internally
    // No vTaskDelay needed here as WiFi.begin() is non-blocking

    WiFi.mode(WIFI_STA);
    WiFi.begin(g_wifiCfg.ssid, g_wifiCfg.password);
    g_wifiStatus = WIFI_STATUS_CONNECTING;
    g_connectStartTime = xTaskGetTickCount();
    DEBUG_PRINTLN("WiFi connection started");
#endif
}

// RTOS Task for WiFi - Runs on Core 0
static void WIFI_Task(void* parameter)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Task running on core: " + String(xPortGetCoreID()));
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // 500ms cycle
    TickType_t stabilityCheckTime = 0;
    bool needStabilityCheck = false;

    for(;;)
    {
        TickType_t currentTick = xTaskGetTickCount();
        
        if (xSemaphoreTake(g_wifiMutex, portMAX_DELAY) == pdTRUE)
        {
            wl_status_t st = WiFi.status();

            switch (g_wifiStatus)
            {
            case WIFI_STATUS_CONNECTING:
                if (st == WL_CONNECTED)
                {
                    // Schedule stability check instead of blocking
                    if (!needStabilityCheck) {
                        needStabilityCheck = true;
                        stabilityCheckTime = currentTick;
                    }
                    
                    // Check if stability period has elapsed
                    if (needStabilityCheck && 
                        (currentTick - stabilityCheckTime >= WIFI_STABILITY_CHECK_TICKS))
                    {
                        if (WiFi.status() == WL_CONNECTED) {
                            g_wifiStatus = WIFI_STATUS_CONNECTED;
                            needStabilityCheck = false;
                            DEBUG_PRINTLN("WiFi connected! IP: " + WiFi.localIP().toString());
                            
                            // Release mutex before callback
                            xSemaphoreGive(g_wifiMutex);
                            
                            if (g_wifiCfg.on_connect)
                                g_wifiCfg.on_connect();
                            
                            // Continue to next iteration
                            vTaskDelayUntil(&xLastWakeTime, xFrequency);
                            continue;
                        } else {
                            // Connection lost during stability check
                            needStabilityCheck = false;
                        }
                    }
                }
                else if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL)
                {
                    g_wifiStatus = WIFI_STATUS_DISCONNECTED;
                    g_lastReconnectAttempt = currentTick;
                    needStabilityCheck = false;
                    DEBUG_PRINTLN("WiFi connection failed");
                }
                else if ((currentTick - g_connectStartTime) >= WIFI_CONNECT_TIMEOUT_TICKS)
                {
                    DEBUG_PRINTLN("WiFi connection timeout");
                    WiFi.disconnect(false, false);
                    g_wifiStatus = WIFI_STATUS_DISCONNECTED;
                    g_lastReconnectAttempt = currentTick;
                    needStabilityCheck = false;
                }
                break;

            case WIFI_STATUS_CONNECTED:
                if (st != WL_CONNECTED)
                {
                    g_wifiStatus = WIFI_STATUS_DISCONNECTED;
                    DEBUG_PRINTLN("WiFi disconnected!");
                    
                    // Release mutex before callback
                    xSemaphoreGive(g_wifiMutex);
                    
                    if (g_wifiCfg.on_disconnect)
                        g_wifiCfg.on_disconnect();
                    
                    // Re-acquire mutex to update reconnect time
                    if (xSemaphoreTake(g_wifiMutex, portMAX_DELAY) == pdTRUE) {
                        g_lastReconnectAttempt = xTaskGetTickCount();
                        xSemaphoreGive(g_wifiMutex);
                    }
                    
                    vTaskDelayUntil(&xLastWakeTime, xFrequency);
                    continue;
                }
                break;

            case WIFI_STATUS_DISCONNECTED:
            {
                TickType_t reconnectInterval = pdMS_TO_TICKS(g_wifiCfg.reconnect_interval_ms);
                if ((currentTick - g_lastReconnectAttempt) >= reconnectInterval)
                {
                    DEBUG_PRINTLN("Attempting to reconnect WiFi...");
                    WIFI_StartConnection();
                    g_lastReconnectAttempt = currentTick;
                }
                break;
            }

            case WIFI_STATUS_ERROR:
            default:
                break;
            }

            xSemaphoreGive(g_wifiMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
#endif
}

void WIFI_Init(const WIFI_Config_t *config)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Initializing");
    
    g_wifiMutex = xSemaphoreCreateMutex();
    if (g_wifiMutex == NULL) {
        DEBUG_PRINTLN("WiFi Mutex creation failed!");
        g_wifiStatus = WIFI_STATUS_ERROR;
        return;
    }

    g_mqttInitMutex = xSemaphoreCreateMutex();
    if (g_mqttInitMutex == NULL) {
        DEBUG_PRINTLN("MQTT Init Mutex creation failed!");
        vSemaphoreDelete(g_wifiMutex);
        g_wifiMutex = NULL;
        g_wifiStatus = WIFI_STATUS_ERROR;
        return;
    }

    g_wifiCfg = *config;
    g_lastReconnectAttempt = xTaskGetTickCount();
    WIFI_StartConnection();

    // Create WiFi task PINNED TO CORE 0
    BaseType_t taskCreated = xTaskCreatePinnedToCore(
        WIFI_Task,              // Task function
        "WIFI_Task",            // Task name
        WIFI_TASK_STACK_SIZE,   // Stack size
        NULL,                   // Parameters
        WIFI_TASK_PRIORITY,     // Priority (higher than MQTT)
        &g_wifiTaskHandle,      // Task handle
        WIFI_TASK_CORE          // Core 0 - Network operations
    );

    if (taskCreated != pdPASS) {
        DEBUG_PRINTLN("WiFi Task creation failed!");
        
        // Stop WiFi hardware before cleanup
        WiFi.disconnect(true);  // true = turn off WiFi radio
        WiFi.mode(WIFI_OFF);
        
        // Clean up synchronization primitives
        vSemaphoreDelete(g_wifiMutex);
        vSemaphoreDelete(g_mqttInitMutex);
        g_wifiMutex = NULL;
        g_mqttInitMutex = NULL;
        
        // Reset state
        g_wifiStatus = WIFI_STATUS_ERROR;
        g_wifiTaskRunning = false;
        
        return;
    }

    g_wifiTaskRunning = true;
    DEBUG_PRINTLN("WiFi Task created on Core " + String(WIFI_TASK_CORE));
#endif
}

void WIFI_Process(void)
{
    // Deprecated - Task handles everything
}

WIFI_Status_t WIFI_GetStatus(void)
{
#if WIFI_ENABLED == STD_ON
    WIFI_Status_t status = WIFI_STATUS_DISCONNECTED;
    if (xSemaphoreTake(g_wifiMutex, portMAX_DELAY) == pdTRUE)
    {
        status = g_wifiStatus;
        xSemaphoreGive(g_wifiMutex);
    }
    return status;
#else
    return WIFI_STATUS_DISCONNECTED;
#endif
}

bool WIFI_IsConnected(void)
{
#if WIFI_ENABLED == STD_ON
    bool connected = false;
    if (xSemaphoreTake(g_wifiMutex, portMAX_DELAY) == pdTRUE)
    {
        connected = (g_wifiStatus == WIFI_STATUS_CONNECTED);
        xSemaphoreGive(g_wifiMutex);
    }
    return connected;
#else
    return false;
#endif
}

int WIFI_GetRSSI(void)
{
#if WIFI_ENABLED == STD_ON
    int rssi = 0;
    if (xSemaphoreTake(g_wifiMutex, portMAX_DELAY) == pdTRUE)
    {
        if (WiFi.status() == WL_CONNECTED)
            rssi = WiFi.RSSI();
        xSemaphoreGive(g_wifiMutex);
    }
    return rssi;
#else
    return 0;
#endif
}

void WIFI_Deinit(void)
{
#if WIFI_ENABLED == STD_ON
    if (!g_wifiTaskRunning) return;

    if (g_wifiTaskHandle != NULL) {
        vTaskDelete(g_wifiTaskHandle);
        g_wifiTaskHandle = NULL;
    }

    if (g_wifiMutex != NULL) {
        vSemaphoreDelete(g_wifiMutex);
        g_wifiMutex = NULL;
    }

    if (g_mqttInitMutex != NULL) {
        vSemaphoreDelete(g_mqttInitMutex);
        g_mqttInitMutex = NULL;
    }

    WiFi.disconnect(true);
    g_wifiTaskRunning = false;
    DEBUG_PRINTLN("WiFi Deinitialized");
#endif
}