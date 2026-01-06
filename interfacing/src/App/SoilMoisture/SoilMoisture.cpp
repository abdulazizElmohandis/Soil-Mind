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


#if SOILMOISTURE_DEBUG == STD_ON
static void Debug_PrintQueue(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(in);
    Serial.print(" out=");
    Serial.print(out);
    Serial.print(" count=");
    Serial.print(count);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Moisture_QUEUE_SIZE; i++)
    {
        Serial.print(Soil_Moisture_Queue[i]);
        Serial.print(" ");
    }
    Serial.println();
}
#else
#define Debug_PrintQueue(tag)
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

static queue_t deq(uint8_t *data)
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
    uint32_t dry = DRY_VALUE;
    uint32_t wet = WET_VALUE;
    // Clamp raw between wet and dry regardless of which is bigger
    uint32_t lo = min(dry, wet);
    uint32_t hi = max(dry, wet);
    rawValue = constrain(rawValue, lo, hi);
    // Convert to 0..100 with correct direction
    int moisture;
    if (dry > wet) {
    // dry -> 0, wet -> 100
    moisture = (int)(( (long)(dry - rawValue) * 100L ) / (long)(dry - wet));
    } else {
    // dry -> 0, wet -> 100 (opposite direction)
    moisture = (int)(( (long)(rawValue - dry) * 100L ) / (long)(wet - dry));
    }
    moisture = constrain(moisture, 0, 100);
    uint8_t m = (uint8_t)moisture;

    DEBUG_PRINTLN("Soil Moisture Read Value: " + String(rawValue));
    DEBUG_PRINTLN("Soil Moisture percentage: " + String(m));
    inq(m);
    Debug_PrintQueue("AFTER INQ");


#endif
}
void SoilMoisture_getMoisture(uint8_t *moisture)
{
#if SOILMOISTURE_ENABLED == STD_ON
    if (deq(moisture) == queue_empty)
    {
        *moisture = 0; // Indicate that the queue is empty
    }
    Debug_PrintQueue("AFTER INQ");

#endif
}
