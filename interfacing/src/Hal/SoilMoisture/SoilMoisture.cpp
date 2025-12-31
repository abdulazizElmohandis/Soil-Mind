#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "SoilMoisture.h"


#if SOILMOISTURE_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif


static SoilMoisture_t* soilMoistureConfig = NULL;

void SoilMoisture_Init(SoilMoisture_t* config) {
#if SOILMOISTURE_ENABLED == STD_ON
    soilMoistureConfig = config;
    DEBUG_PRINTLN("Soil Moisture Sensor Initialized");
    
    ADC_Init(&(config->adcConfig));
    DEBUG_PRINTLN("Soil Moisture Channel: " + String(config->adcConfig.channel));
    DEBUG_PRINTLN("Soil Moisture Resolution: " + String(config->adcConfig.resolution));
#endif
}

uint32_t SoilMoisture_ReadValue(void) {
#if SOILMOISTURE_ENABLED == STD_ON
    if(soilMoistureConfig == NULL) {
        DEBUG_PRINTLN("Soil Moisture Sensor not initialized");
        return 0;
    }
    uint32_t rawValue = ADC_ReadValue(soilMoistureConfig->adcConfig.channel);
    DEBUG_PRINTLN("Soil Moisture Read Value: " + String(rawValue));
    return rawValue;
#else
    return 0; // Indicate that the Soil Moisture Sensor is disabled
#endif
}

float SoilMoisture_ReadPercentage(void) {
#if SOILMOISTURE_ENABLED == STD_ON
    if(soilMoistureConfig == NULL) {
        DEBUG_PRINTLN("Soil Moisture Sensor not initialized");
        return 0.0;
    }
    uint32_t rawValue = SoilMoisture_ReadValue();
    float percentage = (float)rawValue / (float)ADC_MAX_VALUE * 100.0;
    DEBUG_PRINTLN("Soil Moisture Percentage: " + String(percentage) + "%");
    return percentage;
#else
    return 0.0; // Indicate that the Soil Moisture Sensor is disabled
#endif
}