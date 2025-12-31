#include <Arduino.h>
#include "src/APP_Cfg.h"
#include "src/Hal/WIFI/wifi.h"
#include "src/Hal/MQTT/MQTT.h"

// Test interval
#define PUBLISH_INTERVAL_MS 5000  // Publish every 5 seconds

static unsigned long lastPublishTime = 0;
static int messageCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== MQTT Test Starting ===");
  
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
  Serial.println("MQTT will be initialized when WiFi connects");
#else
  Serial.println("ERROR: MQTT is disabled in APP_Cfg.h");
#endif
}

void loop() {
#if WIFI_ENABLED == STD_ON
  // Process WiFi state machine
  WIFI_Process();
  
  // Check if WiFi is connected
  if (WIFI_IsConnected()) {
#if MQTT_ENABLED == STD_ON
    // Process MQTT loop
    MQTT_Loop();
    
    // Publish test messages periodically
    if (millis() - lastPublishTime >= PUBLISH_INTERVAL_MS) {
      publishTestMessages();
      lastPublishTime = millis();
    }
#endif
  } else {
    // Print WiFi status
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= 2000) {
      Serial.print("WiFi Status: ");
      switch(WIFI_GetStatus()) {
        case WIFI_STATUS_DISCONNECTED:
          Serial.println("DISCONNECTED");
          break;
        case WIFI_STATUS_CONNECTING:
          Serial.println("CONNECTING...");
          break;
        case WIFI_STATUS_CONNECTED:
          Serial.println("CONNECTED");
          break;
        case WIFI_STATUS_ERROR:
          Serial.println("ERROR");
          break;
      }
      lastStatusPrint = millis();
    }
  }
#else
  Serial.println("WiFi is disabled. Enable it in APP_Cfg.h");
  delay(5000);
#endif
}

void publishTestMessages() {
#if MQTT_ENABLED == STD_ON
  if (!MQTT_IsConnected()) {
    Serial.println("MQTT not connected, skipping publish");
    return;
  }
  
  messageCount++;
  
  // Publish test telemetry message
  String telemetryPayload = "{";
  telemetryPayload += "\"test\":true,";
  telemetryPayload += "\"messageCount\":" + String(messageCount) + ",";
  telemetryPayload += "\"timestamp\":" + String(millis()) + ",";
  telemetryPayload += "\"soilMoisture\":" + String(random(0, 100)) + ",";
  telemetryPayload += "\"temperature\":" + String(random(20, 30)) + ".";
  telemetryPayload += String(random(0, 9));
  telemetryPayload += "}";
  
  Serial.println("Publishing to " + String(MQTT_TOPIC_TELEMETRY));
  MQTT_Publish(MQTT_TOPIC_TELEMETRY, telemetryPayload.c_str());
  
  // Publish a simple test message
  String testMessage = "Test message #" + String(messageCount) + " from ESP32";
  Serial.println("Publishing test message to pump control topic");
  MQTT_Publish(MQTT_TOPIC_PUMP_CONTROL, testMessage.c_str());
  
  // Print connection info
  Serial.println("---");
  Serial.println("WiFi RSSI: " + String(WIFI_GetRSSI()) + " dBm");
  Serial.println("Messages published: " + String(messageCount));
  Serial.println("---");
#endif
}
