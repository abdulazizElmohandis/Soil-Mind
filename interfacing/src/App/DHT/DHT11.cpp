#include "DHT11.h"
#include "../../APP_Cfg.h"
static uint8_t Temperature[Temperature_QUEUE_SIZE];
static uint8_t inT;
static uint8_t outT;
static uint8_t countT;
static uint8_t Humidity[Humidity_QUEUE_SIZE];
static uint8_t inH;
static uint8_t outH;
static uint8_t countH;
#if DHT11_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif
static void inqT(int data)
{
    if (countT < Temperature_QUEUE_SIZE)
    {
        if (inT == Temperature_QUEUE_SIZE)
        {
            inT = 0;
        }
        else
        {
            ;
        }
        Temperature[inT++] = data;
        countT++;
    }
}

static queue_t deqT(int *data)
{
    queue_t status = queue_ok;
    if (outT == Temperature_QUEUE_SIZE)
    {
        outT = 0;
    }
    else
    {
        ;
    }
    if ((inT == outT) && (countT == 0))
    {
        status = queue_empty;
    }
    else
    {
        *(data) = Temperature[outT++];
        status = queue_ok;
        countT--;
    }
    return status;
}

static void inqH(int data)
{
    if (countH < Humidity_QUEUE_SIZE)
    {
        if (inH == Humidity_QUEUE_SIZE)
        {
            inH = 0;
        }
        else
        {
            ;
        }
        Humidity[inH++] = data;
        countH++;
    }
}

static queue_t deqH(int *data)
{
    queue_t status = queue_ok;
    if (outH == Humidity_QUEUE_SIZE)
    {
        outH = 0;
    }
    else
    {
        ;
    }
    if ((inH == outH) && (countH == 0))
    {
        status = queue_empty;
    }
    else
    {
        *(data) = Humidity[outH++];
        status = queue_ok;
        countH--;
    }
    return status;
}

static DHT11Cfg_t DHT11_Sensors[MAX_SENSORS_DHT] = {
    {DHT11_1_PIN}};

static DHT dht11_sensor(DHT11_Sensors[0].data_pin, DHT_TYPE);

void DHT11_init(void)
{
#if DHT11_ENABLED == STD_ON
    int i = 0;
    for (i = 0; i < MAX_SENSORS_DHT; i++)
    {
        dht11_sensor.begin();
        DEBUG_PRINTLN("DHT11 Sensor Initialized on Pin: " + String(DHT11_Sensors[i].data_pin));
    }
#endif
}

void DHT11_main(void)
{
#if DHT11_ENABLED == STD_ON

    uint8_t temperature = dht11_sensor.readTemperature();
    uint8_t humidity = dht11_sensor.readHumidity();

    /* Debug output */
    DEBUG_PRINTLN("[DHT11] Temp = ");
    DEBUG_PRINTLN(temperature);
    DEBUG_PRINTLN(" C | Humidity = ");
    DEBUG_PRINTLN(humidity);
    DEBUG_PRINTLN(" %");
    /* Check for invalid readings */
    if (isnan(temperature) || isnan(humidity))
    {
        DEBUG_PRINTLN("[DHT11] Sensor read FAILED!");
        return;
    }
    else
    {
        inqT(temperature);
        inqH(humidity);
        DEBUG_PRINTLN("[DHT11] Sensor read SUCCESSFUL!");
    }

#endif
}

void DHT11_GetTemperature(int *temperature)
{
#if DHT11_ENABLED == STD_ON
    if (deqT(temperature) == queue_empty)
    {
        *temperature = 0; // Indicate that the queue is empty
    }
#endif
}
void DHT11_GetHumidity(int *humidity)
{
    #if DHT11_ENABLED == STD_ON
    if (deqH(humidity) == queue_empty)
    {
        *humidity = 0; // Indicate that the queue is empty
    }
#endif
}
