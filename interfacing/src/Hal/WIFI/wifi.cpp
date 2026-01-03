#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include "../../APP_Cfg.h"
#include "wifi.h"
#if MQTT_ENABLED == STD_ON
#include "../MQTT/MQTT.h"
#endif

#if WIFI_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif


bool mqttInitialized = false;

void onWifiConnected(void)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Connected! IP: " + WiFi.localIP().toString());
    
    // Initialize MQTT only when WiFi is connected
#if MQTT_ENABLED == STD_ON
    if (!mqttInitialized) {
        // Use username/password if configured (non-empty strings)
        const char* username = (strlen(MQTT_USERNAME) > 0) ? MQTT_USERNAME : NULL;
        const char* password = (strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : NULL;
        MQTT_Init(MQTT_BROKER, MQTT_PORT, username, password);
        mqttInitialized = true;
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
static unsigned long g_lastReconnectAttempt = 0;
static unsigned long g_connectStartTime = 0;

#define WIFI_CONNECT_TIMEOUT_MS 15000

static void WIFI_StartConnection(void)
{
#if WIFI_ENABLED == STD_ON
    if (g_wifiCfg.ssid == NULL || g_wifiCfg.password == NULL)
    {
        g_wifiStatus = WIFI_STATUS_ERROR;
        return;
    }

    WiFi.disconnect(false, false);
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.begin(g_wifiCfg.ssid, g_wifiCfg.password);
    g_wifiStatus = WIFI_STATUS_CONNECTING;
    g_connectStartTime = millis();
    DEBUG_PRINTLN("WiFi connection started");
#endif
}

void WIFI_Init(const WIFI_Config_t *config)
{
#if WIFI_ENABLED == STD_ON
    DEBUG_PRINTLN("WiFi Initializing");
    g_wifiCfg = *config;
    g_lastReconnectAttempt = millis();
    WIFI_StartConnection();
#endif
}

void WIFI_Process(void)
{
#if WIFI_ENABLED == STD_ON
    wl_status_t st = WiFi.status();

    switch (g_wifiStatus)
    {
    case WIFI_STATUS_CONNECTING:
        if (st == WL_CONNECTED)
        {
            delay(500);
            
            if (WiFi.status() == WL_CONNECTED) {
                g_wifiStatus = WIFI_STATUS_CONNECTED;
                DEBUG_PRINTLN("WiFi connected! IP: " + WiFi.localIP().toString());
                
                if (g_wifiCfg.on_connect)
                    g_wifiCfg.on_connect();
            }
        }
        else if (st == WL_CONNECT_FAILED ||
                 st == WL_NO_SSID_AVAIL)
        {
            g_wifiStatus = WIFI_STATUS_DISCONNECTED;
            g_lastReconnectAttempt = millis();
            DEBUG_PRINTLN("WiFi connection failed");
        }
        else if (millis() - g_connectStartTime >= WIFI_CONNECT_TIMEOUT_MS)
        {
            DEBUG_PRINTLN("WiFi connection timeout");
            WiFi.disconnect(false, false);
            g_wifiStatus = WIFI_STATUS_DISCONNECTED;
            g_lastReconnectAttempt = millis();
        }
        break;

    case WIFI_STATUS_CONNECTED:
        if (st != WL_CONNECTED)
        {
            g_wifiStatus = WIFI_STATUS_DISCONNECTED;
            DEBUG_PRINTLN("WiFi disconnected!");
            
            if (g_wifiCfg.on_disconnect)
                g_wifiCfg.on_disconnect();
            g_lastReconnectAttempt = millis();
        }
        break;

    case WIFI_STATUS_DISCONNECTED:
        if (millis() - g_lastReconnectAttempt >= g_wifiCfg.reconnect_interval_ms)
        {
            DEBUG_PRINTLN("Attempting to reconnect WiFi...");
            WIFI_StartConnection();
            g_lastReconnectAttempt = millis();
        }
        break;

    case WIFI_STATUS_ERROR:
    default:
        break;
    }
#endif
}

WIFI_Status_t WIFI_GetStatus(void)
{
#if WIFI_ENABLED == STD_ON
    return g_wifiStatus;
#else
    return WIFI_STATUS_DISCONNECTED;
#endif
}

bool WIFI_IsConnected(void)
{
#if WIFI_ENABLED == STD_ON
    return (g_wifiStatus == WIFI_STATUS_CONNECTED);
#else
    return false;
#endif
}

int WIFI_GetRSSI(void)
{
#if WIFI_ENABLED == STD_ON
    if (WiFi.status() == WL_CONNECTED)
        return WiFi.RSSI();
#endif
    return 0;
}