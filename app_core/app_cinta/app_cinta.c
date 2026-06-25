#include "../app_cinta/app_cinta.h"
#include "../app/comandos.h"
#include "../utils/temporizador.h"
#include "../hal/include/hal_servo.h"
#include "../hal/include/hal_timer.h"
#include <stdio.h>  

static _eEstacion0 eEstacion0 = eEstacion0_Libre;

// Solo necesitamos el sensor de la entrada (S0) para disparar el cálculo
static Debouncer_t debounceIR0;

static uint16_t distanciaBase = 180; 

static uint8_t nDisparosHcsr04;
static uint16_t disparosMm[20]; 
static uint8_t idMuestraAnterior = 0;
static char msgBuffer[80];

// ==========================================
// FIFO: SISTEMA DE TRACKING POR LAZO ABIERTO
// ==========================================
static uint8_t cajasEnCola = 0;
static uint8_t idDestino[30];        // 0=Estación1, 1=Estación2, 2=Estación3
static Temporizador timerViaje[30];  // Arreglo de temporizadores (ETA)

static _eEstaciones eEstacion1 = eEstaciones_Libre;
static _eEstaciones eEstacion2 = eEstaciones_Libre;
static _eEstaciones eEstacion3 = eEstaciones_Libre;

static uint32_t msInicioDePasada;
static uint32_t msFinalDePasada;

static Temporizador servo1;
static Temporizador servo2;
static Temporizador servo3;

static uint8_t ServoMin0=90;
static uint8_t ServoMax0=0;
static uint8_t ServoMin1=90;
static uint8_t ServoMax1=0;
static uint8_t ServoMin2=90;
static uint8_t ServoMax2=0;

// Variables cinemáticas (en milímetros)
static uint16_t coordenadaEstacion1 = 400; 
static uint16_t coordenadaEstacion2 = 830; 
static uint16_t coordenadaEstacion3 = 1310;

// Tiempos mecánicos de los actuadores
static uint16_t msDesplegar0=500;
static uint16_t msRetraer0=500;
static uint16_t msDesplegar1=500;
static uint16_t msRetraer1=500;
static uint16_t msDesplegar2=500;
static uint16_t msRetraer2=500;

static uint8_t disparosMax=10;

void AppCinta_Init(void){
    Debounce_Init(&debounceIR0, 50, true);
    HCSR04_SetMode(true);
}

void AppCinta_Task(void){
    // Solo actualizamos el IR0, ahorramos ciclos de CPU
    Debounce_Update(&debounceIR0, GPIO_LeerSensor(0));
    
    uint8_t cajaA, cajaB, cajaC, i;
    uint32_t sumaAlturas = 0;
    uint8_t disparosValidos = 0; 

    // ---------------------------------------------------------
    // SUBSISTEMA DE MEDICIÓN (S0)
    // ---------------------------------------------------------
    switch(eEstacion0){
        case eEstacion0_Detenido:
            break;
            
        case eEstacion0_Libre:
            if(debounceIR0.flanco_bajada){
                msInicioDePasada = HAL_GetMillis();
                nDisparosHcsr04 = 0;
                idMuestraAnterior = HCSR04_GetMuestraID();
                
                Comandos_EnviarLog("Inicio de lectura de caja");
                eEstacion0 = eEstacion0_Midiendo;
            }
            break;
            
        case eEstacion0_Midiendo: 
            if(debounceIR0.flanco_subida){
                msFinalDePasada = HAL_GetMillis();
                Comandos_EnviarLog("Fin de lectura de caja");
                eEstacion0 = eEstacion0_Despachando;
            }
            
            if(HCSR04_GetMuestraID() != idMuestraAnterior && nDisparosHcsr04 < disparosMax){
                idMuestraAnterior = HCSR04_GetMuestraID();                
                disparosMm[nDisparosHcsr04] = HCSR04_GetDistance();
                nDisparosHcsr04++;
            }
            break;
            
        case eEstacion0_Despachando:
            cajaA = 0; cajaB = 0; cajaC = 0;
            sumaAlturas = 0;
            disparosValidos = 0;
            for(i = 0; i < nDisparosHcsr04; i++){
                if(distanciaBase >= disparosMm[i]){
                    uint16_t alturaTemporal = distanciaBase - disparosMm[i];
                    if((alturaTemporal >= 52 && alturaTemporal <= 68) ||
                       (alturaTemporal > 72 && alturaTemporal <= 88) ||
                       (alturaTemporal > 92 && alturaTemporal <= 108)){
                        sumaAlturas += alturaTemporal;
                        disparosValidos++;
                        if(alturaTemporal >= 52 && alturaTemporal <= 68){
                            cajaA++;
                        }else if(alturaTemporal > 72 && alturaTemporal <= 88){
                            cajaB++;
                        }else if(alturaTemporal > 92 && alturaTemporal <= 108){
                            cajaC++;
                        }
                    }
                }
            }
            
            if(disparosValidos > 0 && cajasEnCola < 30){
                uint16_t alturaPromedio = (uint16_t)(sumaAlturas / disparosValidos);
                char charCaja = '?';
                uint16_t msEsperar;
                
                // Cálculo de ETA (Estimated Time of Arrival) basado en cinemática:
                // v = dx/dt -> msEsperar = (distancia * dt) / longitud_caja
                if(cajaA > cajaB && cajaA > cajaC){
                    msEsperar = (coordenadaEstacion1 * (msFinalDePasada - msInicioDePasada)) / 100;
                    idDestino[cajasEnCola] = 0;
                    Temp_IniciarMS(&timerViaje[cajasEnCola++], msEsperar);
                    charCaja = 'A';
                }else if(cajaB > cajaA && cajaB > cajaC){
                    msEsperar = (coordenadaEstacion2 * (msFinalDePasada - msInicioDePasada)) / 100;
                    idDestino[cajasEnCola] = 1;
                    Temp_IniciarMS(&timerViaje[cajasEnCola++], msEsperar);
                    charCaja = 'B';
                }else if(cajaC > cajaA && cajaC > cajaB){
                    msEsperar = (coordenadaEstacion3 * (msFinalDePasada - msInicioDePasada)) / 100;
                    idDestino[cajasEnCola] = 2;
                    Temp_IniciarMS(&timerViaje[cajasEnCola++], msEsperar);
                    charCaja = 'C';
                }
                
                if (charCaja != '?') {
                    sprintf(msgBuffer, "Caja %c | Alt: %u mm | ETA: %u ms", charCaja, alturaPromedio, msEsperar);
                    Comandos_EnviarLog(msgBuffer);
                }
            } else {
                Comandos_EnviarLog("Objeto ignorado o cola llena");
            }
            eEstacion0 = eEstacion0_Libre;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMAS DE EYECCIÓN (Basados 100% en Temporizadores)
    // ---------------------------------------------------------

    // ESTACIÓN 1
    switch(eEstacion1){
        case eEstaciones_Libre:
            for(i=0; i<cajasEnCola; i++){
                if(idDestino[i] == 0 && Temp_Expiro(&timerViaje[i])){
                    AppCinta_QuitarDeCola(i);
                    HAL_Servo_SetAngle(0, ServoMax0);
                    Temp_IniciarMS(&servo1, msDesplegar0);
                    eEstacion1 = eEstaciones_Desplegando;
                    break;
                }
            }
            break;
        case eEstaciones_Desplegando:
            if(Temp_Expiro(&servo1)){
                HAL_Servo_SetAngle(0, ServoMin0);
                Temp_IniciarMS(&servo1, msRetraer0);
                eEstacion1 = eEstaciones_Retrayendo;
            }
            break;
        case eEstaciones_Retrayendo:
            if(Temp_Expiro(&servo1)){
                eEstacion1 = eEstaciones_Libre;
            }
            break;
        default: break;
    }

    // ESTACIÓN 2
    switch(eEstacion2){
        case eEstaciones_Libre:
            for(i=0; i<cajasEnCola; i++){
                if(idDestino[i] == 1 && Temp_Expiro(&timerViaje[i])){
                    AppCinta_QuitarDeCola(i);
                    HAL_Servo_SetAngle(1, ServoMax1);
                    Temp_IniciarMS(&servo2, msDesplegar1);
                    eEstacion2 = eEstaciones_Desplegando;
                    break;
                }
            }
            break;
        case eEstaciones_Desplegando:
            if(Temp_Expiro(&servo2)){
                HAL_Servo_SetAngle(1, ServoMin1);
                Temp_IniciarMS(&servo2, msRetraer1);
                eEstacion2 = eEstaciones_Retrayendo;
            }
            break;
        case eEstaciones_Retrayendo:
            if(Temp_Expiro(&servo2)){
                eEstacion2 = eEstaciones_Libre;
            }
            break;
        default: break;
    }

    // ESTACIÓN 3
    switch(eEstacion3){
        case eEstaciones_Libre:
            for(i=0; i<cajasEnCola; i++){
                if(idDestino[i] == 2 && Temp_Expiro(&timerViaje[i])){
                    AppCinta_QuitarDeCola(i);
                    HAL_Servo_SetAngle(2, ServoMax2);
                    Temp_IniciarMS(&servo3, msDesplegar2);
                    eEstacion3 = eEstaciones_Desplegando;
                    break;
                }
            }
            break;
        case eEstaciones_Desplegando:
            if(Temp_Expiro(&servo3)){
                HAL_Servo_SetAngle(2, ServoMin2);
                Temp_IniciarMS(&servo3, msRetraer2);
                eEstacion3 = eEstaciones_Retrayendo;
            }
            break;
        case eEstaciones_Retrayendo:
            if(Temp_Expiro(&servo3)){
                eEstacion3 = eEstaciones_Libre;
            }
            break;
        default: break;
    }
}

void AppCinta_Iniciar(void){
    GPIO_SetCinta(true);
    eEstacion0 = eEstacion0_Libre;
}

void AppCinta_Detener(void){
    GPIO_SetCinta(false);
    eEstacion0 = eEstacion0_Detenido;
}

void AppCinta_QuitarDeCola(uint8_t pos){
    if (cajasEnCola == 0) return; 
    
    // Desplazamos los arreglos un índice hacia atrás
    for(uint8_t i = pos; i < (cajasEnCola - 1); i++){
        timerViaje[i] = timerViaje[i+1];
        idDestino[i]  = idDestino[i+1];
    }
    
    // Limpiamos el último elemento
    idDestino[cajasEnCola - 1] = 0;
    cajasEnCola--;
}

void AppCinta_SetVariable(_eSetVariables idVariable, uint32_t valor){
    static const char* nombresVariables[] = {
        "ServoMin0", "ServoMax0", "ServoMin1", "ServoMax1", "ServoMin2", "ServoMax2",
        "Ciego", "Delta", "CoordEstacion1", "CoordEstacion2", "CoordEstacion3",
        "msDesplegar0", "msRetraer0", "msEsperar0",
        "msDesplegar1", "msRetraer1", "msEsperar1",
        "msDesplegar2", "msRetraer2", "msEsperar2",
        "DistanciaBase", "DisparosMax"
    };
    switch(idVariable){
        case eSetServoMin0: ServoMin0 = valor; break;
        case eSetServoMax0: ServoMax0 = valor; break;
        case eSetServoMin1: ServoMin1 = valor; break;
        case eSetServoMax1: ServoMax1 = valor; break;
        case eSetServoMin2: ServoMin2 = valor; break;
        case eSetServoMax2: ServoMax2 = valor; break;
        
        // Las siguientes variables espaciales (Delta, Limites) ya NO afectan al control,
        // pero las mantenemos para no romper la compatibilidad con el HMI de Qt.
        case eSetDelta: break; 
        case eSetCiego: break; 
            
        case eSetCoordenadaEstacion1: coordenadaEstacion1 = valor; break;
        case eSetCoordenadaEstacion2: coordenadaEstacion2 = valor; break;
        case eSetCoordenadaEstacion3: coordenadaEstacion3 = valor; break;
        
        case eSetMsDesplegar0: msDesplegar0 = valor; break;
        case eSetMsRetraer0: msRetraer0 = valor; break;
        case eSetMsDesplegar1: msDesplegar1 = valor; break;
        case eSetMsRetraer1: msRetraer1 = valor; break;
        case eSetMsDesplegar2: msDesplegar2 = valor; break;
        case eSetMsRetraer2: msRetraer2 = valor; break;
        
        case eSetDistanciaBase: distanciaBase = valor; break;
        case eSetDisparosMax: disparosMax = valor; break;
        default: return;
    }
    sprintf(msgBuffer, "SET: %s = %lu", nombresVariables[idVariable], valor);
    Comandos_EnviarLog(msgBuffer);
}