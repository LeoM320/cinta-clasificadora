#ifndef APP_CINTA_H_
#define APP_CINTA_H_

#include "../config/hardware.h"
#include "../config/gpio.h"
#include "../utils/debounce.h"
#include "../drivers/hcsr04.h"
#include "../app/comandos.h"
//#include "../utils/uner_protocol.h"
//Estados de la cinta
/*
Por un lado voy a tener el sistema de mediciones
y por el otro el sistema clasificador
*/

typedef enum {
    eEstacion0_Detenido,
    eEstacion0_Calibracion,
    eEstacion0_Libre,
    eEstacion0_Midiendo,
    eEstacion0_Despachando
}_eEstacion0;

typedef enum {
    eEstaciones_Libre,
    eEstaciones_Esperando,
    eEstaciones_Desplegando,
    eEstaciones_Retrayendo
}_eEstaciones;

void AppCinta_Init(void);
void AppCinta_Task(void);

void AppCinta_Iniciar(void);
void AppCinta_Detener(void);

void AppCinta_QuitarDeCola(uint8_t pos);

void AppCinta_SetVariable(_eSetVariables idVariable, uint32_t valor);
#endif /* APP_CINTA_H_ */