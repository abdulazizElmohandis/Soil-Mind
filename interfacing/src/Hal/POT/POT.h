#ifndef POT_H
#define POT_H
#include <stdint.h>
#include "../ADC/ADC.h"


typedef struct{
    ADC_t adcConfig;
}POT_t;



void POT_Init(POT_t* config);

uint32_t POT_ReadValue(void);

float POT_ReadPercentage(void);

#endif