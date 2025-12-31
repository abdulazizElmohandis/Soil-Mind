#ifndef PUMP_H
#define PUMP_H
#include <stdint.h>
#include "../PWM/PWM.h"


typedef struct{
    PWM_t pwmConfig;
}Pump_t;



void Pump_Init(Pump_t* config);

void Pump_Start(void);

void Pump_Stop(void);

void Pump_SetSpeed(float speedPercentage);

#endif

