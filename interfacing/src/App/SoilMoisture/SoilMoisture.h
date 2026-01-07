#ifndef SOILMOISTURE_H
#define SOILMOISTURE_H
#include <stdint.h>
#include "../../Hal/ADC/ADC.h"
#include "../../APP_Cfg.h"


typedef struct{
    ADC_t adcConfig;
}SoilMoisture_t;


#define DRY_VALUE   3800

#define WET_VALUE   1250    



void SoilMoisture_Init(void);

void SoilMoisture_main(void);

queue_t SoilMoisture_getMoisture(uint8_t *moisture);


#endif
