#ifndef MQTT_CORE_H
#define MQTT_CORE_H

#include <stdint.h>

// MQTT Configuration Structure
typedef struct {
    const char* broker;
    int port;
    const char* username;
    const char* password;
} MQTT_Config_t;

// Message Handler Function Pointer
typedef void (*MQTT_MessageHandler_t)(const char* payload);

// Public API for MQTT Core Module
// Generic and reusable - handles connection, reconnection, and message dispatching
// Contains NO business logic

void MQTT_Init(const MQTT_Config_t* cfg);
void MQTT_Loop(void);                 // Called inside RTOS task - non-blocking
bool MQTT_IsConnected(void);

bool MQTT_Publish(const char* topic,
                  const char* payload,
                  uint8_t qos = 0,
                  bool retain = false);

bool MQTT_Subscribe(const char* topic, uint8_t qos = 0);
bool MQTT_RegisterHandler(const char* topic,
                          MQTT_MessageHandler_t handler);

#endif // MQTT_CORE_H
