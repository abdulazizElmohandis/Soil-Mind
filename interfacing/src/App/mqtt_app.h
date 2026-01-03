#ifndef MQTT_APP_H
#define MQTT_APP_H

// Decision types for irrigation system
typedef enum {
    DECISION_IRRIGATE = 0,
    DECISION_NO_IRRIGATION = 1,
    DECISION_CHECK_SYSTEM = 2
} Decision_t;

// MQTT Application Module - Contains all business logic
// Responsible for topic management, command handling, and decision publishing
// Communicates with MQTT Core only through public APIs

void MQTT_APP_Init(void);

void MQTT_APP_SubscribeTopics(void);
void MQTT_APP_PublishTelemetry(void);
void MQTT_APP_PublishDecision(Decision_t decision);

// Message handlers for incoming commands
void MQTT_APP_OnCommand(const char* payload);
void MQTT_APP_OnPumpCommand(const char* payload);

#endif // MQTT_APP_H
