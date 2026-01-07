#include "Nitrogen_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static int NitrogenValue[Nitrogen_QUEUE_SIZE];
static uint8_t inN;
static uint8_t outN;
static uint8_t countN;

#if Nitrogen_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
static void Debug_PrintQueueN(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(inN);
    Serial.print(" out=");
    Serial.print(outN);
    Serial.print(" count=");
    Serial.print(countN);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Nitrogen_QUEUE_SIZE; i++)
    {
        Serial.print(NitrogenValue[i]);
        Serial.print(" ");
    }
    Serial.println();
}
#else
#define DEBUG_PRINTLN(var)
#define Debug_PrintQueueN(tag)
#endif

static void inqN(int data)
{
    // If full, overwrite oldest by advancing out
    if (countN >= Nitrogen_QUEUE_SIZE)
    {
        outN = (uint8_t)((outN + 1) % Nitrogen_QUEUE_SIZE);
        // count stays at max (full)
        countN = Nitrogen_QUEUE_SIZE;
    }
    else
    {
        countN++;
    }

    NitrogenValue[inN] = data;
    inN = (uint8_t)((inN + 1) % Nitrogen_QUEUE_SIZE);
}

static queue_t deqN(int *data)
{
    if (data == NULL)
    {
        return queue_empty;
    }

    if (countN == 0)
    {
        return queue_empty;
    }

    *data = NitrogenValue[outN];
    outN = (uint8_t)((outN + 1) % Nitrogen_QUEUE_SIZE);
    countN--;

    return queue_ok;
}

static Nitrogen_t defaultNitrogenConfig = {
    {Nitrogen_SENSOR_PIN, Nitrogen_RESOLUTION}};

void NitrogenSensor_init(void)
{
    // Initialize ADC for Nitrogen Sensor Pin
    #if Nitrogen_ENABLED == STD_ON
    ADC_Init(&(defaultNitrogenConfig.adcConfig));
    DEBUG_PRINTLN("Nitrogen Sensor Initialized on Pin: " + String(Nitrogen_SENSOR_PIN));
    #endif
}

void NitrogenSensor_main(void)
{
    // Read value from Nitrogen Sensor
    #if Nitrogen_ENABLED == STD_ON
    int adcValue = ADC_ReadValue(defaultNitrogenConfig.adcConfig.channel);
    int nitrogenValue = map(adcValue, Zero, ADC_MAX, Zero, NITROGEN_MAX); 
    DEBUG_PRINTLN("Nitrogen Value (mg/kg): " + String(nitrogenValue));
    inqN(nitrogenValue);
    Debug_PrintQueueN("AFTER INQ");
    #endif
}

queue_t NitrogenSensor_getvalue(int *value)
{
#if Nitrogen_ENABLED == STD_ON
    queue_t status = deqN(value);
    Debug_PrintQueueN("AFTER DEQ");
    return status;
#else
    (void)value;
    return queue_empty;
#endif
}
