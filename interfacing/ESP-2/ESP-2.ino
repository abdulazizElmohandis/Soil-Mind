#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Queue.h>

#define PUMP_PIN 26

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Actuator queues
QueueHandle_t commandQueue;

// ----------------- Decision Struct -----------------
struct IrrigationDecision {
  bool pumpOn;
  int durationSec;
  unsigned long timestamp;
};

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  // Initialize WiFi & MQTT
  WiFi.begin("SSID", "PASS");
  mqttClient.setServer("BROKER_IP", 1883);
  mqttClient.setCallback(MQTTCallback);

  // Create queue
  commandQueue = xQueueCreate(5, sizeof(IrrigationDecision));

  // Create tasks
  xTaskCreatePinnedToCore(ActuatorTask, "ActuatorTask", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", 4096, NULL, 2, NULL, 0);
}

// ----------------- MQTT Callback -----------------
void MQTTCallback(char* topic, byte* payload, unsigned int length){
  if(strcmp(topic, "farm/site1/nodeA/decision")==0){
    IrrigationDecision decision;
    // parse payload (assume JSON parser exists)
    parseDecisionPayload(payload, length, &decision);
    xQueueOverwrite(commandQueue, &decision);
    // Send ACK back
    mqttClient.publish("farm/site1/nodeB/ack", "OK", true);
  }
}

// ----------------- Tasks -----------------
void ActuatorTask(void* pvParameters){
  IrrigationDecision decision;
  for(;;){
    if(xQueuePeek(commandQueue, &decision, portMAX_DELAY) == pdTRUE){
      if(decision.pumpOn){
        digitalWrite(PUMP_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(decision.durationSec*1000));
        digitalWrite(PUMP_PIN, LOW);
      }
    }
  }
}

void MQTTTask(void* pvParameters){
  for(;;){
    if(!mqttClient.connected()){
      while(!mqttClient.connected()){
        if(mqttClient.connect("ESP32B")){
          mqttClient.subscribe("farm/site1/nodeA/decision");
        } else {
          delay(1000);
        }
      }
    }
    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
