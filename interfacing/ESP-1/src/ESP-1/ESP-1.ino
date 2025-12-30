#include <Arduino.h>
#include <Queue.h>          // Use FreeRTOS queues
#include <WiFi.h>
#include <PubSubClient.h>

// Assume these modules exist
#include "NPKSensor.h"
#include "SoilMoistureSensor.h"
#include "DHT22Sensor.h"
#include "MLModel_Health.h"
#include "MLModel_Irrigation.h"

// ----------------- Globals -----------------
QueueHandle_t sensorQueue;
QueueHandle_t decisionQueue;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define SENSOR_TASK_PERIOD_MS 5000     // 5 sec
#define ML_TASK_PERIOD_MS 60000        // 1 min
#define MQTT_TASK_PERIOD_MS 2000       // 2 sec

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);

  // Initialize sensors
  NPKSensor.begin();
  SoilMoistureSensor.begin();
  DHT22Sensor.begin();

  // Initialize ML models
  MLModel_Health.begin();
  MLModel_Irrigation.begin();

  // Initialize MQTT
  WiFi.begin("SSID", "PASS");
  mqttClient.setServer("BROKER_IP", 1883);

  // Create queues
  sensorQueue = xQueueCreate(10, sizeof(SensorData));
  decisionQueue = xQueueCreate(5, sizeof(IrrigationDecision));

  // Create RTOS tasks
  xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(MLTask, "MLTask", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", 4096, NULL, 1, NULL, 0);
}

// ----------------- Sensor Data Struct -----------------
struct SensorData {
  float N, P, K;
  float soilMoisture;
  float temperature;
  float humidity;
  unsigned long timestamp;
};

// ----------------- Decision Struct -----------------
struct IrrigationDecision {
  bool pumpOn;
  int durationSec;
  unsigned long timestamp;
};

// ----------------- Tasks -----------------

void SensorTask(void* pvParameters) {
  SensorData data;
  for(;;) {
    // Read sensors
    data.N = NPKSensor.readN();
    data.P = NPKSensor.readP();
    data.K = NPKSensor.readK();
    data.soilMoisture = SoilMoistureSensor.read();
    data.temperature = DHT22Sensor.readTemperature();
    data.humidity = DHT22Sensor.readHumidity();
    data.timestamp = millis();

    // Push to queue
    xQueueOverwrite(sensorQueue, &data);

    vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
  }
}

void MLTask(void* pvParameters) {
  SensorData sensorData;
  IrrigationDecision decision;

  for(;;) {
    if (xQueuePeek(sensorQueue, &sensorData, portMAX_DELAY) == pdTRUE) {
      // Run Health model
      auto healthOutput = MLModel_Health.predict(sensorData);

      // Run Irrigation model
      decision = MLModel_Irrigation.predict(sensorData, healthOutput);

      decision.timestamp = millis();

      // Push decision to MQTT task
      xQueueOverwrite(decisionQueue, &decision);
    }
    vTaskDelay(pdMS_TO_TICKS(ML_TASK_PERIOD_MS));
  }
}

void MQTTTask(void* pvParameters) {
  IrrigationDecision decision;
  for(;;) {
    if(!mqttClient.connected()){
      // Reconnect logic
      while(!mqttClient.connected()){
        if(mqttClient.connect("ESP32A")){
          Serial.println("MQTT Connected");
        } else {
          delay(1000);
        }
      }
    }
    mqttClient.loop();

    // Publish sensor telemetry
    SensorData sensorData;
    if(xQueuePeek(sensorQueue, &sensorData, 0) == pdTRUE){
      String payload = String("{") +
                       "\"N\":" + sensorData.N + "," +
                       "\"P\":" + sensorData.P + "," +
                       "\"K\":" + sensorData.K + "," +
                       "\"soilMoisture\":" + sensorData.soilMoisture + "," +
                       "\"temp\":" + sensorData.temperature + "," +
                       "\"humidity\":" + sensorData.humidity + 
                       "}";
      mqttClient.publish("farm/site1/nodeA/telemetry", payload.c_str(), false);
    }

    // Publish irrigation decision if available
    if(xQueuePeek(decisionQueue, &decision, 0) == pdTRUE){
      String payload = String("{") +
                       "\"pumpOn\":" + decision.pumpOn + "," +
                       "\"duration\":" + decision.durationSec + 
                       "}";
      mqttClient.publish("farm/site1/nodeA/decision", payload.c_str(), true);
    }

    vTaskDelay(pdMS_TO_TICKS(MQTT_TASK_PERIOD_MS));
  }
}
