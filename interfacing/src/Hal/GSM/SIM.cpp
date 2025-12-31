#include<Arduino.h>
#include "SIM2.h"
#include <WiFi.h>
#include <TinyGSM.h>

HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);

sim_state_t state = STATE_MODEM_RESTART;

void SIM_Init(void)
{
  #if SIM_800L_ENABLED==STD_ON
  Serial.println("[STATE_INIT] Starting Serial...");
    Serial.begin(9600);
    SerialAT.begin(SIM_BAUDRATE, SERIAL_8N1, SIM_RX, SIM_TX);
    delay(1000);
  #endif
}

bool SIM_ModemRestart(void)
{
  #if SIM_800L_ENABLED==STD_ON
    Serial.println("[STATE_MODEM_RESTART] Initializing modem...");
    if (!modem.init())
    {
        Serial.println("Modem restart failed!");
        delay(1000);
        return false;
    }

    Serial.println("Modem ready.");
    if (GSM_PIN && modem.getSimStatus() != SIM_Ready)
    {
        modem.simUnlock(GSM_PIN);
    }
    return true;
    #endif
}

sim_status_t SIM_CheckSIM(void)
{
  #if SIM_800L_ENABLED==STD_ON
  Serial.println("[STATE_CHECK_SIM] Checking SIM card...");
    int status = modem.getSimStatus();
    if (status != SIMReady)
    {
        Serial.println("SIM not ready! Status: " + String(status));
        delay(1000);
    }
    else
    {
        Serial.println("SIM detected.");
    }
    return (sim_status_t)status;
    #endif
}

bool SIM_WaitForNetwork(uint32_t timeout_ms)
{
  #if SIM_800L_ENABLED==STD_ON
    Serial.println("[STATE_WAIT_NETWORK] Waiting for network...");
    if (modem.waitForNetwork(timeout_ms))
    {
        Serial.println("Connected to network.");
        return true;
    }
    Serial.println("Network not found!");
    return false;
    #endif
}

bool SIM_GPRSConnect(const char* apn, const char* user, const char* pass)
{
  #if SIM_800L_ENABLED==STD_ON
  Serial.println("[STATE_GPRS_CONNECT] Connecting GPRS...");
    uint8_t sq = modem.getSignalQuality();
    Serial.println("Signal Quality: " + String(sq));

    if (!modem.gprsConnect("internet", "", ""))
    {
        Serial.println("GPRS connection failed!");
        return false;
    }
    Serial.println("GPRS connected.");
    return true;
    #endif
}

bool SIM_SendSMS(const char* recipient, const char* message)
{
  #if SIM_800L_ENABLED==STD_ON
    if (modem.sendSMS(recipient, message))
    {
        Serial.println("SMS sent successfully!");
        return true;
    }
    Serial.println("Failed to send SMS.");
    return false;
    #endif
}

uint8_t SIM_GetSignalQuality(void)
{
  #if SIM_800L_ENABLED==STD_ON
    return modem.getSignalQuality();
    #endif
}
bool SIM_MakeCall(const char* number)
{
  #if SIM_800L_ENABLED==STD_ON
    Serial.println("[SIM_MakeCall] Dialing: " + String(number));
    modem.sendAT("ATD" + String(number) + ";");  
    if (modem.waitResponse(10000L) == 1)       
    {
        Serial.println("Call started.");
        return true;
    }
    Serial.println("Failed to start call.");
    return false;
    #endif
}