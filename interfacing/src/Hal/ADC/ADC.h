#ifndef ADC_H
#define ADC_H
#include <stdint.h>


typedef struct{
    uint8_t channel;
    uint8_t resolution;
}ADC_t;



void ADC_Init(ADC_t* config);

uint32_t ADC_ReadValue(uint8_t channel);

#endif
