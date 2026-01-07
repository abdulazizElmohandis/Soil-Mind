#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "Pump.h"


#if PUMP_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif


static Pump_t* pumpConfig = NULL;

//This function initializes the Pump by initializing the PWM with 20 kHz frequency
void Pump_Init(Pump_t* config) {
#if PUMP_ENABLED == STD_ON
    pumpConfig = config;
    DEBUG_PRINTLN("Pump Initialized");
    
    // Set PWM frequency to 20 kHz for pump
    config->pwmConfig.frequency = PUMP_PWM_FREQUENCY;
    DEBUG_PRINTLN("Pump PWM Frequency: " + String(config->pwmConfig.frequency) + " Hz");
    DEBUG_PRINTLN("Pump Channel: " + String(config->pwmConfig.channel));
    DEBUG_PRINTLN("Pump Resolution: " + String(config->pwmConfig.resolution));
    
    // Initialize PWM channel
    PWM_initChannel(&(config->pwmConfig));
    
    // Initialize pump to stopped state (0% duty cycle)
    Pump_Stop();
#endif
}

//This function starts the pump by setting PWM duty cycle to 100%
void Pump_Start(void) {
#if PUMP_ENABLED == STD_ON
    if(pumpConfig == NULL) {
        DEBUG_PRINTLN("Pump not initialized------------------------");
        return;
    }
    PWM_setDutyCycle(pumpConfig->pwmConfig.channel, 0);
    DEBUG_PRINTLN("Pump Started 0 %------------------------");
#endif
}

//This function stops the pump by setting PWM duty cycle to 0%
void Pump_Stop(void) {
#if PUMP_ENABLED == STD_ON
    if(pumpConfig == NULL) {
        DEBUG_PRINTLN("Pump not initialized");
        return;
    }
    PWM_setDutyCycle(pumpConfig->pwmConfig.channel, 100);
    DEBUG_PRINTLN("Pump Stopped");
#endif
}

//This function sets the pump speed by setting PWM duty cycle to the specified percentage
void Pump_SetSpeed(float speedPercentage) {
#if PUMP_ENABLED == STD_ON
    if(pumpConfig == NULL) {
        DEBUG_PRINTLN("Pump not initialized");
        return;
    }
    // Clamp speed between 0 and 100
    if(speedPercentage < 0.0) {
        speedPercentage = 0.0;
    }
    if(speedPercentage > 100.0) {
        speedPercentage = 100.0;
    }
    PWM_setDutyCycle(pumpConfig->pwmConfig.channel, speedPercentage);
    DEBUG_PRINTLN("Pump Speed Set to: " + String(speedPercentage) + "%");
#endif
}