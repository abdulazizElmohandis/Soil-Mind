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

// ---------- Enqueue: overwrite oldest when full ----------
static void inq(uint8_t data)
{
    // If full, discard oldest by advancing out
    if (count >= Moisture_QUEUE_SIZE)
    {
        out = (uint8_t)((out + 1) % Moisture_QUEUE_SIZE);
        // count stays at max (full)
        count = Moisture_QUEUE_SIZE;
    }
    else
    {
        count++;
    }

    Soil_Moisture_Queue[in] = data;
    in = (uint8_t)((in + 1) % Moisture_QUEUE_SIZE);
}

// ---------- Dequeue ----------
static queue_t deq(uint8_t *data)
{
    if (data == NULL)
    {
        // If you have queue_error in your enum, return it.
        // Otherwise return queue_empty.
        return queue_empty;
    }

    if (count == 0)
    {
        return queue_empty;
    }

    *data = Soil_Moisture_Queue[out];
    out = (uint8_t)((out + 1) % Moisture_QUEUE_SIZE);
    count--;

    return queue_ok;
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
    if (dry == wet)
    {
        DEBUG_PRINTLN("Soil moisture calibration error: DRY_VALUE == WET_VALUE");
        return;
    }
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
queue_t SoilMoisture_getMoisture(uint8_t *moisture)
{
#if SOILMOISTURE_ENABLED == STD_ON
    return deq(moisture);
    Debug_PrintQueue("AFTER DEQ");

#else
    (void)moisture;
    return queue_empty;
#endif
}
