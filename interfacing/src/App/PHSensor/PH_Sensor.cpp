#include "PH_Sensor.h"
#include "../../APP_Cfg.h"
#include "../../Hal/ADC/ADC.h"

static uint8_t PHValue[PH_QUEUE_SIZE];
static uint8_t inPH;
static uint8_t outPH;
static uint8_t countPH;

#if PH_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static void inqPH(int data)
{
    if (inPH == PH_QUEUE_SIZE)
    {
        inPH = 0;
    }
    PHValue[inPH++] = data;
    countPH++;
}

static queue_t deqPH(int *data)
{
    queue_t status = queue_ok;

    if (outPH == PH_QUEUE_SIZE)
    {
        outPH = 0;
    }

    if ((inPH == outPH) && (countPH == 0))
    {
        status = queue_empty;
    }
    else
    {
        *data = PHValue[outPH++];
        countPH--;
        status = queue_ok;
    }
    return status;
}

static PH_t defaultPHConfig =
{
    {PH_SENSOR_PIN, PH_RESOLUTION}
};

void PHSensor_init(void)
{
#if PH_ENABLED == STD_ON
    ADC_Init(&(defaultPHConfig.adcConfig));
    DEBUG_PRINTLN("PH Sensor Initialized");
#endif
}

void PHSensor_main(void)
{
#if PH_ENABLED == STD_ON
    int adcValue = ADC_ReadValue(defaultPHConfig.adcConfig.channel);

    /* Map ADC → pH (0 → 14) */
    int phValue = map(adcValue, Zero, ADC_MAX, Zero, PH_MAX);

    DEBUG_PRINTLN("PH Value: " + String(phValue));
    inqPH(phValue);
#endif
}

void PHSensor_getvalue(int *value)
{
#if PH_ENABLED == STD_ON
    if (deqPH(value) == queue_empty)
    {
        *value = 7;   /* neutral default */
    }
#endif
}
