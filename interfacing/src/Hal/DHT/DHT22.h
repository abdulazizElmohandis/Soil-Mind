#ifndef DHT22_H
#define DHT22_H

#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include <DHT.h>

#if DHT22_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

#define MAX_SENSORS 1     

#define DHT22_TYPE DHT22  

typedef struct
{
    uint8_t data_pin;      
} DHT22Cfg_t;


void DHT22_init(void);
float DHT22_readTemperature(uint8_t sensorIndex);
float DHT22_readHumidity(uint8_t sensorIndex);
bool DHT22_read(uint8_t sensorIndex, float *temperature, float *humidity);


#endif // DHT22_H
