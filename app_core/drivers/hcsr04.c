#include "hcsr04.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_timer.h" // Para HAL_GetMicros()
#include "../utils/temporizador.h"

//Pongo un modo rapido y un modo lento
//Rapido es para medir las cajas
//Lento es para telemetria
//Por cada medicion tiene que haber un id y se consulta el modo

static volatile _eHcsr04 eHcsr04 = eHcsr04_Libre;
static volatile uint32_t usInicioEco;
static volatile uint32_t usFinEco;
static uint32_t usObjetivo = 0;
static uint16_t ultimaDistanciaMm = 0;

static bool modoRapido = false; //Osea, lo pongo a andar siempre y listo
static uint16_t espera = 1000;
static Temporizador timerContinuo;
static uint8_t muestraID = 0;

void HCSR04_Init(void) {
    HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
    ultimaDistanciaMm = 0;
    eHcsr04 = eHcsr04_Libre;
}

bool HCSR04_Trigger(void) {
    if (eHcsr04 != eHcsr04_Libre) {
        return false; 
    }
    
    HAL_GPIO_WRITE_HIGH(TRIGGER_PORT, TRIGGER_PIN);
    usObjetivo = HAL_GetMicros(); 
    eHcsr04 = eHcsr04_Disparando;
    return true;
}

void HCSR04_Task(void) {
    uint32_t usActual = HAL_GetMicros();
    
    switch (eHcsr04) {
        case eHcsr04_Libre:
            break;
            
        case eHcsr04_Disparando:
            //El trigger esta en alto y cambio tras 10 uS
            if ((usActual - usObjetivo) >= 10UL) {
                HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
                usObjetivo = usActual; 
                eHcsr04 = eHcsr04_EsperandoInicioEco;
            }
            break;
            
        case eHcsr04_EsperandoInicioEco:
            //El eco se manipula mediante interrupcion, pero reinicia el sensor a los 60mS
        case eHcsr04_EsperandoFinEco:
            if ((usActual - usObjetivo) > 60000UL) {
                eHcsr04 = eHcsr04_Libre;
            }
            break;
            
        case eHcsr04_Calculando:
            ultimaDistanciaMm = (uint16_t)(((usFinEco - usInicioEco) * 343UL) / 2000UL);
            eHcsr04 = eHcsr04_Libre;
            muestraID++;
            break;
    }
}

void HCSR04_EXTI_Handler(void) {
    if (eHcsr04 == eHcsr04_EsperandoInicioEco) {
        if (HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
            usInicioEco = HAL_GetMicros();
            eHcsr04 = eHcsr04_EsperandoFinEco;
        }
    } 
    else if (eHcsr04 == eHcsr04_EsperandoFinEco) {
        if (!HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
            usFinEco = HAL_GetMicros();
            eHcsr04 = eHcsr04_Calculando;
        }
    }
}

uint16_t HCSR04_GetDistance(void) {
    return ultimaDistanciaMm;
}

uint8_t HCSR04_GetMuestraID(void) {
    return muestraID;
}

void HCSR04_InitContinuous(bool modo) {
    HCSR04_Init();
    HCSR04_SetMode(modo);
    HCSR04_Trigger();
    Temp_IniciarMS(&timerContinuo, espera);
}

void HCSR04_SetMode(bool modo) {
    if(modo){
        modoRapido = true;
        espera = 80;
    }else{
        modoRapido = false;
        espera = 1000;
    }
}

void HCSR04_TaskContinuous(void){
    HCSR04_Task();
    if(Temp_Expiro(&timerContinuo)){
        HCSR04_Trigger();
        Temp_IniciarMS(&timerContinuo, espera);
    }
}