#ifndef SOILMOISTURE_H
#define SOILMOISTURE_H
#include <stdint.h>
#include "../ADC/ADC.h"


typedef struct{
    ADC_t adcConfig;
}SoilMoisture_t;



void SoilMoisture_Init(SoilMoisture_t* config);

uint32_t SoilMoisture_ReadValue(void);

float SoilMoisture_ReadPercentage(void);

#endif

