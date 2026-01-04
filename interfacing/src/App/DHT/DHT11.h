#ifndef DHT11_H
#define DHT11_H

#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include <DHT.h>

   

#define DHT_TYPE DHT11  

typedef struct
{
    uint8_t data_pin;
} DHT11Cfg_t;


void DHT11_init(void);
void DHT11_main(void);
void DHT11_GetTemperature(float *temperature);
void DHT11_GetHumidity(float *humidity);



#endif // DHT11_H
