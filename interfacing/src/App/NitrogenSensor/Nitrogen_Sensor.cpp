#include "Nitrogen_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static uint8_t NitrogenValue[Nitrogen_QUEUE_SIZE];
static uint8_t inN;
static uint8_t outN;
static uint8_t countN;

#if Nitrogen_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif
static void inqN(int data)
{
    if (inN == Nitrogen_QUEUE_SIZE)
    {
        inN = 0;
    }
    else
    {
        ;
    }
    NitrogenValue[inN++] = data;
    countN++;
}

static queue_t deqN(int *data)
{
    queue_t status = queue_ok;
    if (outN == Nitrogen_QUEUE_SIZE)
    {
        outN = 0;
    }
    else
    {
        ;
    }
    if ((inN == outN) && (countN == 0))
    {
        status = queue_empty;
    }
    else
    {
        *(data) = NitrogenValue[outN++];
        status = queue_ok;
        countN--;
    }
    return status;
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
    #endif
}

void NitrogenSensor_getvalue(int *value)
{
    #if Nitrogen_ENABLED == STD_ON
    if (deqN(value) == queue_empty)
    {
        *value = 0;
    }
    #endif
}