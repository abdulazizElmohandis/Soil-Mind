#include "DHT22.h"
#include "../../APP_Cfg.h"

static DHT22Cfg_t DHT22_Sensors[MAX_SENSORS] = {
    {DHT22_1_PIN}  
};

static DHT dht22_sensors[MAX_SENSORS] = {
    DHT(DHT22_Sensors[0].data_pin, DHT22_TYPE)
};

void DHT22_init(void)
{
    int i = 0;
    for (i = 0; i < MAX_SENSORS; i++)
    {
        dht22_sensors[i].begin();
        DEBUG_PRINTLN("DHT22 Sensor Initialized on Pin: " + String(DHT22_Sensors[i].data_pin));
    }
}

float DHT22_readTemperature(uint8_t sensorIndex)
{
    float retTemp = NAN;

    if (sensorIndex < MAX_SENSORS)
    {
        retTemp = dht22_sensors[sensorIndex].readTemperature();

        if (isnan(retTemp))
        {
            DEBUG_PRINTLN("Failed to read temperature from DHT22 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT22 sensor index!");
    }

    return retTemp;
}


float DHT22_readHumidity(uint8_t sensorIndex)
{
    float retHumidity = NAN;

    if (sensorIndex < MAX_SENSORS)
    {
        retHumidity = dht22_sensors[sensorIndex].readHumidity();

        if (isnan(retHumidity))
        {
            DEBUG_PRINTLN("Failed to read humidity from DHT22 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT22 sensor index!");
    }

    return retHumidity;
}


bool DHT22_read(uint8_t sensorIndex, float *temperature, float *humidity)
{
    bool retStatus = false;

    if ((sensorIndex < MAX_SENSORS) && (temperature != NULL) && (humidity != NULL))
    {
        *temperature = dht22_sensors[sensorIndex].readTemperature();
        *humidity    = dht22_sensors[sensorIndex].readHumidity();

        if ((!isnan(*temperature)) && (!isnan(*humidity)))
        {
            retStatus = true;
        }
        else
        {
            DEBUG_PRINTLN("Failed to read DHT22 Sensor " + String(sensorIndex));
        }
    }
    else
    {
        DEBUG_PRINTLN("Invalid DHT22 sensor index or NULL pointer!");
    }

    return retStatus;
}

