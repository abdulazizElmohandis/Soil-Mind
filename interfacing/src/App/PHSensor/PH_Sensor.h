#ifndef PH_SENSOR_H
#define PH_SENSOR_H
#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

typedef struct{
    ADC_t adcConfig;
}PH_t;



void PHSensor_init(void);

void PHSensor_main(void);


void PHSensor_getvalue(int *value);





#endif // PH_SENSOR_H


