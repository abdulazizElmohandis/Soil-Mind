#ifndef DHT11_H
#define DHT11_H

#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include <DHT.h>

#if DHT11_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

#define MAX_SENSORS 1     

#define DHT_TYPE DHT11  

typedef struct
{
    uint8_t data_pin;      
} DHT11Cfg_t;


void DHT11_init(void);
float DHT11_readTemperature(uint8_t sensorIndex);
float DHT11_readHumidity(uint8_t sensorIndex);
bool DHT11_read(uint8_t sensorIndex, float *temperature, float *humidity);


#endif // DHT11_H
