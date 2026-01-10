#include "Potassium_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static uint8_t PotassiumValue[Potassium_QUEUE_SIZE];
static uint8_t inK;
static uint8_t outK;
static uint8_t countK;

#if Potassium_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static void inqK(int data)
{
    if (inK == Potassium_QUEUE_SIZE)
    {
        inK = 0;
    }
    PotassiumValue[inK++] = data;
    countK++;
}

static queue_t deqK(int *data)
{
    queue_t status = queue_ok;

    if (outK == Potassium_QUEUE_SIZE)
    {
        outK = 0;
    }

    if ((inK == outK) && (countK == 0))
    {
        status = queue_empty;
    }
    else
    {
        *data = PotassiumValue[outK++];
        countK--;
        status = queue_ok;
    }
    return status;
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
#endif
}

void PotassiumSensor_getvalue(int *value)
{
#if Potassium_ENABLED == STD_ON
    if (deqK(value) == queue_empty)
    {
        *value = 0;
    }
#endif
}
