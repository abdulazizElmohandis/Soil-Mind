#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "ADC.h"


#if ADC_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif



void ADC_Init(ADC_t* config) {
#if ADC_ENABLED == STD_ON
    DEBUG_PRINTLN("ADC Initialized");
    
    DEBUG_PRINTLN("Channel: " + String(config->channel));

    DEBUG_PRINTLN("Resolution: " + String(config->resolution));
    analogReadResolution(config->resolution);
#endif
}

uint32_t ADC_ReadValue(uint8_t channel) {
#if ADC_ENABLED == STD_ON
    int rawValue = analogRead(channel);
    DEBUG_PRINTLN("Read Value from channel " + String(channel) + ": " + String(rawValue));
    return rawValue;
#else
    return 0; // Indicate that the ADC is disabled
#endif
}

