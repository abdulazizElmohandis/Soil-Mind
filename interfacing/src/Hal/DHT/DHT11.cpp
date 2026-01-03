#include "DHT11.h"
#include "../../APP_Cfg.h"

static DHT11Cfg_t DHT11_Sensors[MAX_SENSORS] = {
    {DHT11_1_PIN}  
};

static DHT dht11_sensors[MAX_SENSORS] = {
    DHT(DHT11_Sensors[0].data_pin, DHT_TYPE)
};

void DHT11_init(void)
{
    #if DHT11_ENABLED ==STD_ON
    int i = 0;
    for (i = 0; i < MAX_SENSORS; i++)
    {
        dht11_sensors[i].begin();
        DEBUG_PRINTLN("DHT11 Sensor Initialized on Pin: " + String(DHT11_Sensors[i].data_pin));
    }
    #endif
}

float DHT11_readTemperature(uint8_t sensorIndex)
{
    #if DHT11_ENABLED ==STD_ON
    float retTemp = NAN;

    if (sensorIndex < MAX_SENSORS)
    {
        retTemp = dht11_sensors[sensorIndex].readTemperature();

        if (isnan(retTemp))
        {
            DEBUG_PRINTLN("Failed to read temperature from DHT11 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT11 sensor index!");
    }

    return retTemp;
    #endif
}


float DHT11_readHumidity(uint8_t sensorIndex)
{
    #if DHT11_ENABLED ==STD_ON
    float retHumidity = NAN;

    if (sensorIndex < MAX_SENSORS)
    {
        retHumidity = dht11_sensors[sensorIndex].readHumidity();

        if (isnan(retHumidity))
        {
            DEBUG_PRINTLN("Failed to read humidity from DHT11 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT11 sensor index!");
    }

    return retHumidity;
    
    #endif
}


bool DHT11_read(uint8_t sensorIndex, float *temperature, float *humidity)
{
    #if DHT11_ENABLED ==STD_ON
    bool retStatus = false;

    if ((sensorIndex < MAX_SENSORS) && (temperature != NULL) && (humidity != NULL))
    {
        *temperature = dht11_sensors[sensorIndex].readTemperature();
        *humidity    = dht11_sensors[sensorIndex].readHumidity();

        if ((!isnan(*temperature)) && (!isnan(*humidity)))
        {
            retStatus = true;
        }
        else
        {
            DEBUG_PRINTLN("Failed to read DHT11 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT11 sensor index or NULL pointer!");
    }

    return retStatus;
    #endif
}

