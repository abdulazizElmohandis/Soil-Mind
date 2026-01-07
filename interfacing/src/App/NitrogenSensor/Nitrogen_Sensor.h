#ifndef NITROGEN_SENSOR_H
#define NITROGEN_SENSOR_H
#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"





typedef struct{
    ADC_t adcConfig;
}Nitrogen_t;



void NitrogenSensor_init(void);

void NitrogenSensor_main(void);


queue_t NitrogenSensor_getvalue(int *value);





#endif // NITROGEN_SENSOR_H
