#include "Phosphorus_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static uint8_t PhosphorusValue[Phosphorus_QUEUE_SIZE];
static uint8_t inP;
static uint8_t outP;
static uint8_t countP;

#if Phosphorus_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static void inqP(int data)
{
    if (inP == Phosphorus_QUEUE_SIZE)
    {
        inP = 0;
    }
    PhosphorusValue[inP++] = data;
    countP++;
}

static queue_t deqP(int *data)
{
    queue_t status = queue_ok;

    if (outP == Phosphorus_QUEUE_SIZE)
    {
        outP = 0;
    }

    if ((inP == outP) && (countP == 0))
    {
        status = queue_empty;
    }
    else
    {
        *data = PhosphorusValue[outP++];
        countP--;
        status = queue_ok;
    }
    return status;
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
#endif
}

void PhosphorusSensor_getvalue(int *value)
{
#if Phosphorus_ENABLED == STD_ON
    if (deqP(value) == queue_empty)
    {
        *value = 0;
    }
#endif
}
