#ifndef DRIVERS_HCSR04_H_
#define DRIVERS_HCSR04_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    eHcsr04_Libre,            
    eHcsr04_Disparando,    
    eHcsr04_EsperandoInicioEco,  
    eHcsr04_EsperandoFinEco,
    eHcsr04_Calculando
}_eHcsr04;

void HCSR04_Init(void);
bool HCSR04_Trigger(void);
void HCSR04_Task(void);
void HCSR04_EXTI_Handler(void);
uint16_t HCSR04_GetDistance(void);
uint8_t HCSR04_GetMuestraID(void);

void HCSR04_InitContinuous(bool modoRapido);
void HCSR04_SetMode(bool modo);
void HCSR04_TaskContinuous(void);
#endif /* DRIVERS_HCSR04_H_ */