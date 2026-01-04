#ifndef POTASSIUM_SENSOR_H
#define POTASSIUM_SENSOR_H
#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"





typedef struct{
    ADC_t adcConfig;
}Potassium_t;



void PotassiumSensor_init(void);

void PotassiumSensor_main(void);


void PotassiumSensor_getvalue(int *value);





#endif // NITROGEN_SENSOR_H