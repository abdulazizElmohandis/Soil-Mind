#include "Phosphorus_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static int PhosphorusValue[Phosphorus_QUEUE_SIZE];
static uint8_t inP;
static uint8_t outP;
static uint8_t countP;

#if Phosphorus_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
static void Debug_PrintQueueP(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(inP);
    Serial.print(" out=");
    Serial.print(outP);
    Serial.print(" count=");
    Serial.print(countP);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Phosphorus_QUEUE_SIZE; i++)
    {
        Serial.print(PhosphorusValue[i]);
        Serial.print(" ");
    }
    Serial.println();
}
#else
#define DEBUG_PRINTLN(var)
#define Debug_PrintQueueP(tag)
#endif

static void inqP(int data)
{
    // If full, overwrite oldest by advancing out
    if (countP >= Phosphorus_QUEUE_SIZE)
    {
        outP = (uint8_t)((outP + 1) % Phosphorus_QUEUE_SIZE);
        // count stays at max (full)
        countP = Phosphorus_QUEUE_SIZE;
    }
    else
    {
        countP++;
    }

    PhosphorusValue[inP] = data;
    inP = (uint8_t)((inP + 1) % Phosphorus_QUEUE_SIZE);
}

static queue_t deqP(int *data)
{
    if (data == NULL)
    {
        return queue_empty;
    }

    if (countP == 0)
    {
        return queue_empty;
    }

    *data = PhosphorusValue[outP];
    outP = (uint8_t)((outP + 1) % Phosphorus_QUEUE_SIZE);
    countP--;

    return queue_ok;
}

static Phosphorus_t defaultPhosphorusConfig =
{
    {Phosphorus_SENSOR_PIN, Phosphorus_RESOLUTION}
};

void PhosphorusSensor_init(void)
{
#if Phosphorus_ENABLED == STD_ON
    ADC_Init(&(defaultPhosphorusConfig.adcConfig));
    DEBUG_PRINTLN("Phosphorus Sensor Initialized");
#endif
}

void PhosphorusSensor_main(void)
{
#if Phosphorus_ENABLED == STD_ON
    int adcValue = ADC_ReadValue(defaultPhosphorusConfig.adcConfig.channel);
    int phosphorusValue = map(adcValue, Zero, ADC_MAX, Zero, PHOSPHORUS_MAX);
    DEBUG_PRINTLN("Phosphorus Value (mg/kg): " + String(phosphorusValue));
    inqP(phosphorusValue);
    Debug_PrintQueueP("AFTER INQ");
#endif
}

queue_t PhosphorusSensor_getvalue(int *value)
{
#if Phosphorus_ENABLED == STD_ON
    queue_t status = deqP(value);
    Debug_PrintQueueP("AFTER DEQ");
    return status;
#else
    (void)value;
    return queue_empty;
#endif
}
