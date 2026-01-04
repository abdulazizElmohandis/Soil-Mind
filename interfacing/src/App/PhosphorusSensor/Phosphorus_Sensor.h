#ifndef PHOSPHORUS_SENSOR_H
#define PHOSPHORUS_SENSOR_H
#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"





typedef struct{
    ADC_t adcConfig;
}Phosphorus_t;



void PhosphorusSensor_init(void);

void PhosphorusSensor_main(void);


void PhosphorusSensor_getvalue(int *value);





#endif // NITROGEN_SENSOR_H