#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "POT.h"


#if POT_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif


static POT_t* potConfig = NULL;

//This function initializes the POT by initializing the ADC and setting the channel and resolution
void POT_Init(POT_t* config) {
#if POT_ENABLED == STD_ON
    potConfig = config;
    DEBUG_PRINTLN("POT Initialized");
    
    ADC_Init(&(config->adcConfig));
    DEBUG_PRINTLN("POT Channel: " + String(config->adcConfig.channel));
    DEBUG_PRINTLN("POT Resolution: " + String(config->adcConfig.resolution));
#endif
}

//This function reads the value from the POT by reading the value from the ADC
uint32_t POT_ReadValue(void) {
#if POT_ENABLED == STD_ON
    if(potConfig == NULL) {
        DEBUG_PRINTLN("POT not initialized");
        return 0;
    }
    uint32_t rawValue = ADC_ReadValue(potConfig->adcConfig.channel);
    DEBUG_PRINTLN("POT Read Value: " + String(rawValue));
    return rawValue;
#else
    return 0; // Indicate that the POT is disabled
#endif
}

float POT_ReadPercentage(void) {
#if POT_ENABLED == STD_ON
    if(potConfig == NULL) {
        DEBUG_PRINTLN("POT not initialized");
        return 0.0;
    }
    uint32_t rawValue = POT_ReadValue();
    float percentage = (float)rawValue / (float)ADC_MAX_VALUE * 100.0;
    DEBUG_PRINTLN("POT Percentage: " + String(percentage) + "%");
    return percentage;
#else
    return 0.0; // Indicate that the POT is disabled
#endif
}