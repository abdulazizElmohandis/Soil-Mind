#ifndef SIM_H
#define SIM_H

#define TINY_GSM_MODEM_SIM800
#define  SIM_BAUDRATE    9600
#define SIM_RX       16  
#define SIM_TX       17


#define SIM_BAUDRATE 9600
#define SIM_RX 16
#define SIM_TX 17
#define GSM_PIN ""
#define SIMReady    1

/**************************************/

typedef enum
{
    STATE_MODEM_RESTART,
    STATE_CHECK_SIM,
    STATE_WAIT_NETWORK,
    STATE_GPRS_CONNECT,
    STATE_SEND_MESSAGE,
    STATE_IDLE,
    STATE_ERROR
} sim_state_t;

typedef enum
{
    SIM_Ready = 3
}sim_status_t;

// APIs
void SIM_Init(void);
bool SIM_ModemRestart(void);
sim_status_t SIM_CheckSIM(void);
bool SIM_WaitForNetwork(uint32_t timeout_ms);
bool SIM_GPRSConnect(const char* apn, const char* user, const char* pass);
bool SIM_SendSMS(const char* recipient, const char* message);
uint8_t SIM_GetSignalQuality(void);
bool SIM_MakeCall(const char* number);

#endif
