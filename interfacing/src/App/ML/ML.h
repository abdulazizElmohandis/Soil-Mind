#ifndef ML_H
#define ML_H

#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "../MQTT_APP/mqtt_app.h"

// ML Model includes
#include "irrigation_model.h"
#include "plant_health_model.h"

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

// Health model decision types
typedef enum {
    HEALTH_DECISION_HEALTHY = 0,
    HEALTH_DECISION_NITROGEN_DEFICIENCY = 1,
    HEALTH_DECISION_PH_STRESS_ACIDIC = 2,
    HEALTH_DECISION_PH_STRESS_ALKALINE = 3,
    HEALTH_DECISION_PHOSPHORUS_DEFICIENCY = 4,
    HEALTH_DECISION_POTASSIUM_DEFICIENCY = 5,
    HEALTH_DECISION_WATER_STRESS = 6,
    HEALTH_DECISION_CHECK_SYSTEM = 7
} HealthDecision_t;

// ML inference functions
bool ML_Init();
float ML_RunInference();
Decision_t ML_GetDecision(float probability);
void ML_ProcessDecision();

// Health model inference functions
bool ML_HealthInit();
int ML_RunHealthInference(float n, float p, float k, float ph, float moisture, float temperature);
HealthDecision_t ML_GetHealthDecision(int classIndex);
void ML_ProcessHealthDecision(float n, float p, float k, float ph, float moisture, float temperature);

// Sensor data getters
bool ML_GetSensorData(float *temperature, float *humidity, uint8_t *soilMoisture);

#endif // ML_H
