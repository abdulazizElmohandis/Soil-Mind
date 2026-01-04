#ifndef ML_H
#define ML_H

#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "../MQTT_APP/mqtt_app.h"

// ML Model includes
#include "irrigation_model.h"

// Configuration
#define HISTORY_SIZE 4
#define NUM_FEATURES 8
#define IRRIGATION_THRESHOLD 0.5f

// TensorFlow Lite configuration
#define kTensorArenaSize 8 * 1024
extern uint8_t tensorArena[kTensorArenaSize];

// Sensor history buffer
struct SensorHistory {
    float temperature[HISTORY_SIZE];
    float soilmoisture[HISTORY_SIZE];
    uint8_t index;
    uint8_t count;

    void init();
    void addReading(float temp, float moisture);
    float getTemp(uint8_t stepsAgo);
    float getMoisture(uint8_t stepsAgo);
    float tempMean();
    float moistureMean();
    float tempTrend();
    float moistureTrend();
    bool isReady();
};

// ML inference functions
bool ML_Init();
float ML_RunInference();
Decision_t ML_GetDecision(float probability);
void ML_ProcessDecision();

// Sensor data getters
bool ML_GetSensorData(float *temperature, float *humidity, uint8_t *soilMoisture);

#endif // ML_H
