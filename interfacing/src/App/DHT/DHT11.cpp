#include "DHT11.h"
#include "../../APP_Cfg.h"
#include "../SoilMoisture/SoilMoisture.h"
static float Temperature[Temperature_QUEUE_SIZE];
static uint8_t inT;
static uint8_t outT;
static uint8_t countT;
static float Humidity[Humidity_QUEUE_SIZE];
static uint8_t inH;
static uint8_t outH;
static uint8_t countH;
#if DHT11_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
static void Debug_PrintQueueT(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(inT);
    Serial.print(" out=");
    Serial.print(outT);
    Serial.print(" count=");
    Serial.print(countT);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Temperature_QUEUE_SIZE; i++)
    {
        Serial.print(Temperature[i]);
        Serial.print(" ");
    }
    Serial.println();
}
static void Debug_PrintQueueH(const char *tag)
{
    Serial.print("[QUEUE] ");
    Serial.print(tag);
    Serial.print(" | in=");
    Serial.print(inH);
    Serial.print(" out=");
    Serial.print(outH);
    Serial.print(" count=");
    Serial.print(countH);
    Serial.print(" | data: ");

    for (uint8_t i = 0; i < Humidity_QUEUE_SIZE; i++)
    {
        Serial.print(Humidity[i]);
        Serial.print(" ");
    }
    Serial.println();
}
#else
#define DEBUG_PRINTLN(var)
#define Debug_PrintQueueT(tag)
#define Debug_PrintQueueH(tag)
#endif
static void inqT(float data)
{
    // If full, overwrite oldest by advancing out
    if (countT >= Temperature_QUEUE_SIZE)
    {
        outT = (uint8_t)((outT + 1) % Temperature_QUEUE_SIZE);
        // count stays at max (full)
        countT = Temperature_QUEUE_SIZE;
    }
    else
    {
        countT++;
    }

    Temperature[inT] = data;
    inT = (uint8_t)((inT + 1) % Temperature_QUEUE_SIZE);
}

static queue_t deqT(float *data)
{
    if (data == NULL)
    {
        return queue_empty;
    }

    if (countT == 0)
    {
        return queue_empty;
    }

    *data = Temperature[outT];
    outT = (uint8_t)((outT + 1) % Temperature_QUEUE_SIZE);
    countT--;

    return queue_ok;
}

static void inqH(float data)
{
    // If full, overwrite oldest by advancing out
    if (countH >= Humidity_QUEUE_SIZE)
    {
        outH = (uint8_t)((outH + 1) % Humidity_QUEUE_SIZE);
        // count stays at max (full)
        countH = Humidity_QUEUE_SIZE;
    }
    else
    {
        countH++;
    }

    Humidity[inH] = data;
    inH = (uint8_t)((inH + 1) % Humidity_QUEUE_SIZE);
}

static queue_t deqH(float *data)
{
    if (data == NULL)
    {
        return queue_empty;
    }

    if (countH == 0)
    {
        return queue_empty;
    }

    *data = Humidity[outH];
    outH = (uint8_t)((outH + 1) % Humidity_QUEUE_SIZE);
    countH--;

    return queue_ok;
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
        Debug_PrintQueueT("AFTER INQ T");
        inqH(humidity);
        Debug_PrintQueueH("AFTER INQ H");
        DEBUG_PRINTLN("[DHT11] Sensor read SUCCESSFUL!");
    }

#endif
}

queue_t DHT11_GetTemperature(float *temperature)
{
#if DHT11_ENABLED == STD_ON
    queue_t status = deqT(temperature);
    Debug_PrintQueueT("AFTER DEQ T");
    DEBUG_PRINTLN("[DHT11] getting Temp = ");
    DEBUG_PRINTLN(*temperature);
    return status;
#else
    (void)temperature;
    return queue_empty;
#endif
}
queue_t DHT11_GetHumidity(float *humidity)
{
    #if DHT11_ENABLED == STD_ON
    queue_t status = deqH(humidity);
    Debug_PrintQueueH("AFTER DEQ H");
    DEBUG_PRINTLN("[DHT11] getting Humidity = ");
    DEBUG_PRINTLN(*humidity);
    return status;
#else
    (void)humidity;
    return queue_empty;
#endif
}
