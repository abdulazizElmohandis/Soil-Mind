#include "Potassium_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static int PotassiumValue[Potassium_QUEUE_SIZE];
static uint8_t inK;
static uint8_t outK;
static uint8_t countK;

#if Potassium_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
static void Debug_PrintQueueK(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(inK);
    Serial.print(" out=");
    Serial.print(outK);
    Serial.print(" count=");
    Serial.print(countK);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Potassium_QUEUE_SIZE; i++)
    {
        Serial.print(PotassiumValue[i]);
        Serial.print(" ");
    }
    Serial.println();
}
#else
#define DEBUG_PRINTLN(var)
#define Debug_PrintQueueK(tag)
#endif

static void inqK(int data)
{
    // If full, overwrite oldest by advancing out
    if (countK >= Potassium_QUEUE_SIZE)
    {
        outK = (uint8_t)((outK + 1) % Potassium_QUEUE_SIZE);
        // count stays at max (full)
        countK = Potassium_QUEUE_SIZE;
    }
    else
    {
        countK++;
    }

    PotassiumValue[inK] = data;
    inK = (uint8_t)((inK + 1) % Potassium_QUEUE_SIZE);
}

static queue_t deqK(int *data)
{
    if (data == NULL)
    {
        return queue_empty;
    }

    if (countK == 0)
    {
        return queue_empty;
    }

    *data = PotassiumValue[outK];
    outK = (uint8_t)((outK + 1) % Potassium_QUEUE_SIZE);
    countK--;

    return queue_ok;
}

static Potassium_t defaultPotassiumConfig =
{
    {Potassium_SENSOR_PIN, Potassium_RESOLUTION}
};

void PotassiumSensor_init(void)
{
#if Potassium_ENABLED == STD_ON
    ADC_Init(&(defaultPotassiumConfig.adcConfig));
    DEBUG_PRINTLN("Potassium Sensor Initialized");
#endif
}

void PotassiumSensor_main(void)
{
#if Potassium_ENABLED == STD_ON
    int adcValue = ADC_ReadValue(defaultPotassiumConfig.adcConfig.channel);
    int potassiumValue = map(adcValue, Zero, ADC_MAX, Zero, POTASSIUM_MAX);
    DEBUG_PRINTLN("Potassium Value (mg/kg): " + String(potassiumValue));
    inqK(potassiumValue);
    Debug_PrintQueueK("AFTER INQ");
#endif
}

queue_t PotassiumSensor_getvalue(int *value)
{
#if Potassium_ENABLED == STD_ON
    queue_t status = deqK(value);
    Debug_PrintQueueK("AFTER DEQ");
    return status;
#else
    (void)value;
    return queue_empty;
#endif
}
