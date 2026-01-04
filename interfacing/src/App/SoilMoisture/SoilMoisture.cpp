#include <Arduino.h>
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"
#include "SoilMoisture.h"

static uint8_t Soil_Moisture_Queue[Moisture_QUEUE_SIZE];
static uint8_t in;
static uint8_t out;
static uint8_t count;
#if SOILMOISTURE_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static void inq(int data)
{
        if (in == Moisture_QUEUE_SIZE)
        {
            in = 0;
        }
        else
        {
            ;
        }
        Soil_Moisture_Queue[in++] = data;
        count++;
}

static queue_t deq(int *data)
{
    queue_t status = queue_ok;
    if (out == Moisture_QUEUE_SIZE)
    {
        out = 0;
    }
    else
    {
        ;
    }
    if ((in == out) && (count == 0))
    {
        status = queue_empty;
    }
    else
    {
        *(data) = Soil_Moisture_Queue[out++];
        status = queue_ok;
        count--;
    }
    return status;
}

static SoilMoisture_t defaultSoilMoistureConfig = {
    {SOILMOISTURE_PIN, SOILMOISTURE_RESOLUTION}};

void SoilMoisture_Init(void)
{
#if SOILMOISTURE_ENABLED == STD_ON
    DEBUG_PRINTLN("Soil Moisture Sensor Initialized");

    ADC_Init(&(defaultSoilMoistureConfig.adcConfig));
    DEBUG_PRINTLN("Soil Moisture Channel: " + String(defaultSoilMoistureConfig.adcConfig.channel));
    DEBUG_PRINTLN("Soil Moisture Resolution: " + String(defaultSoilMoistureConfig.adcConfig.resolution));
#endif
}

void SoilMoisture_main(void)
{
#if SOILMOISTURE_ENABLED == STD_ON
    uint32_t rawValue = ADC_ReadValue(defaultSoilMoistureConfig.adcConfig.channel);
    uint8_t moisture = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
    DEBUG_PRINTLN("Soil Moisture Read Value: " + String(rawValue));
    DEBUG_PRINTLN("Soil Moisture percentage: " + String(moisture));
    inq(moisture);

#endif
}
void SoilMoisture_getMoisture(uint8_t *moisture)
{
#if SOILMOISTURE_ENABLED == STD_ON
    if (deq((int *)moisture) == queue_empty)
    {
        *moisture = 0; // Indicate that the queue is empty
    }
#endif
}
