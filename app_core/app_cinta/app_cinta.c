#include "../app_cinta/app_cinta.h"
#include "../app/comandos.h"
#include "../utils/temporizador.h"
#include "../utils/ringbuffer.h"
#include "../hal/include/hal_servo.h"
#include "../hal/include/hal_timer.h"
#include <stdio.h>

#define MAX_CAJAS_E1 10
#define MAX_CAJAS_E2 10
#define MAX_CAJAS_E3 10

/**
 * @brief Procesa la Máquina de Estados de una estación genérica.
 * @param estado Puntero a la variable de estado de la estación (ej. &eEstacion1)
 * @param rbPropio Buffer circular de entrada para esta estación
 * @param rbSiguiente Buffer circular de la próxima estación (NULL si es la última)
 * @param idEstacion ID lógico de la estación (1, 2 o 3)
 * @param servoId ID del actuador hardware (0, 1 o 2)
 * @param tempServo Temporizador de software asociado al servo
 * @param sensorIR Puntero al debouncer del sensor óptico de esta estación
 * @param msEsperar Retardo cinemático tras la detección del sensor
 * @param msDesplegar Tiempo que tarda el servo en abrir
 * @param msRetraer Tiempo que tarda el servo en cerrar
 * @param anguloMax Posición PWM de eyección
 * @param anguloMin Posición PWM de reposo
 */
static void ProcesarEstacionFSM(_eEstaciones* estado,
                                RingBuffer_t* rbPropio,
                                RingBuffer_t* rbSiguiente,
                                uint8_t idEstacion,
                                uint8_t servoId,
                                Temporizador* tempServo,
                                Debouncer_t* sensorIR,
                                uint16_t msEsperar,
                                uint16_t msDesplegar,
                                uint16_t msRetraer,
                                uint8_t anguloMax,
                                uint8_t anguloMin);

// 1. Estructura de dominio
typedef struct {
    Temporizador timerViaje; 
    uint16_t alturaPromedio; 
    uint8_t estacionDestino;
    char idClase;            
} _sCaja;

// 2. Memoria física estática para los Buffers Circulares
static _sCaja mem_rb_estacion1[MAX_CAJAS_E1];
static _sCaja mem_rb_estacion2[MAX_CAJAS_E2];
static _sCaja mem_rb_estacion3[MAX_CAJAS_E3];

// 3. Controladores del Ring Buffer
static RingBuffer_t rbEstacion1;
static RingBuffer_t rbEstacion2;
static RingBuffer_t rbEstacion3;

static _eEstacion0 eEstacion0 = eEstacion0_Libre;

static bool ciego = false; 

static Debouncer_t debounceIR0;
static Debouncer_t debounceIR1;
static Debouncer_t debounceIR2;
static Debouncer_t debounceIR3;

static uint16_t distanciaBase = 180; 
static uint8_t nDisparosHcsr04;
static uint16_t disparosMm[20]; 
static uint8_t idMuestraAnterior = 0;
static char msgBuffer[80];

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

// ==========================================
// PARÁMETROS DE CLASIFICACIÓN (Alturas en mm)
// ==========================================
static uint16_t hMinA = 52, hMaxA = 68;
static uint16_t hMinB = 72, hMaxB = 88;
static uint16_t hMinC = 92, hMaxC = 108;

// ==========================================
// MATRIZ DE ENRUTAMIENTO (Routing Table)
// 1 = Estación 1 | 2 = Estación 2 | 3 = Estación 3 | 0 = Descarte (Dejar pasar)
// ==========================================
static uint8_t destinoClaseA = 1; 
static uint8_t destinoClaseB = 2;
static uint8_t destinoClaseC = 3;

static uint16_t msDesplegar0=500;
static uint16_t msRetraer0=500;
static uint16_t msEsperar0=500;

static uint16_t msDesplegar1=500;
static uint16_t msRetraer1=500;
static uint16_t msEsperar1=500;

static uint16_t msDesplegar2=500;
static uint16_t msRetraer2=500;
static uint16_t msEsperar2=500;

static uint8_t disparosMax=10;

void AppCinta_Init(void){
    HCSR04_SetMode(true);

    // Inicializamos los Ring Buffers
    RingBuffer_Init(&rbEstacion1, mem_rb_estacion1, sizeof(_sCaja), MAX_CAJAS_E1);
    RingBuffer_Init(&rbEstacion2, mem_rb_estacion2, sizeof(_sCaja), MAX_CAJAS_E2);
    RingBuffer_Init(&rbEstacion3, mem_rb_estacion3, sizeof(_sCaja), MAX_CAJAS_E3);

    // Inicializamos todos los debouncers
    Debounce_Init(&debounceIR0, 50, true);
    Debounce_Init(&debounceIR1, 50, true);
    Debounce_Init(&debounceIR2, 50, true);
    Debounce_Init(&debounceIR3, 50, true);
}

void AppCinta_Task(void){
    Debounce_Update(&debounceIR0, GPIO_LeerSensor(0));
    Debounce_Update(&debounceIR1, GPIO_LeerSensor(1));
    Debounce_Update(&debounceIR2, GPIO_LeerSensor(2));
    Debounce_Update(&debounceIR3, GPIO_LeerSensor(3));
    
    uint8_t cajaA, cajaB, cajaC, i;
    uint32_t sumaAlturas = 0;
    uint8_t disparosValidos = 0; 

    // ---------------------------------------------------------
    // SUBSISTEMA DE MEDICIÓN (S0 - PRODUCTOR)
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
            
            // 1. FASE DE CLASIFICACIÓN Y VOTACIÓN
            for(i = 0; i < nDisparosHcsr04; i++){
                if(distanciaBase >= disparosMm[i]){
                    uint16_t alturaTemporal = distanciaBase - disparosMm[i];
                    
                    // Evaluamos usando los límites paramétricos
                    if(alturaTemporal >= hMinA && alturaTemporal <= hMaxA){
                        cajaA++; sumaAlturas += alturaTemporal; disparosValidos++;
                    }else if(alturaTemporal >= hMinB && alturaTemporal <= hMaxB){
                        cajaB++; sumaAlturas += alturaTemporal; disparosValidos++;
                    }else if(alturaTemporal >= hMinC && alturaTemporal <= hMaxC){
                        cajaC++; sumaAlturas += alturaTemporal; disparosValidos++;
                    }
                }
            }
            
            if(disparosValidos > 0){
                uint16_t alturaPromedio = (uint16_t)(sumaAlturas / disparosValidos);
                char claseGanadora = '?';
                
                // Determinamos quién ganó la votación
                if(cajaA > cajaB && cajaA > cajaC) claseGanadora = 'A';
                else if(cajaB > cajaA && cajaB > cajaC) claseGanadora = 'B';
                else if(cajaC > cajaA && cajaC > cajaB) claseGanadora = 'C';

                if (claseGanadora != '?') {
                    
                    // 2. FASE DE ENRUTAMIENTO LÓGICO
                    uint8_t estacionDestino = 0;
                    if (claseGanadora == 'A') estacionDestino = destinoClaseA;
                    else if (claseGanadora == 'B') estacionDestino = destinoClaseB;
                    else if (claseGanadora == 'C') estacionDestino = destinoClaseC;

                    // 3 & 4. ASIGNACIÓN E INYECCIÓN (HÍBRIDO)
                    if (estacionDestino != 0) {
                        _sCaja nuevaCaja;
                        nuevaCaja.alturaPromedio = alturaPromedio;
                        nuevaCaja.idClase = claseGanadora;
                        nuevaCaja.estacionDestino = estacionDestino; // Guardamos el pasaje
                        
                        if (ciego) {
                            // MODO CIEGO: Calculo cinemático directo al bucket final
                            uint16_t coordDestino = (estacionDestino == 1) ? coordenadaEstacion1 : 
                                                    (estacionDestino == 2) ? coordenadaEstacion2 : coordenadaEstacion3;
                            
                            RingBuffer_t* rbDestino = (estacionDestino == 1) ? &rbEstacion1 : 
                                                      (estacionDestino == 2) ? &rbEstacion2 : &rbEstacion3;
                            
                            uint16_t msEsperar = (coordDestino * (msFinalDePasada - msInicioDePasada)) / 100;
                            Temp_IniciarMS(&nuevaCaja.timerViaje, msEsperar);
                            RingBuffer_Push(rbDestino, &nuevaCaja);
                            
                            sprintf(msgBuffer, "CIEGO: %c -> E%u | ETA: %u ms", claseGanadora, estacionDestino, msEsperar);
                        } else {
                            // MODO NORMAL (SENSOR): Inyectamos SIEMPRE en el Tramo 1
                            // El timerViaje no se usa para ETA acá.
                            RingBuffer_Push(&rbEstacion1, &nuevaCaja);
                            sprintf(msgBuffer, "SENSOR: %c -> E%u encolada en Tramo 1", claseGanadora, estacionDestino);
                        }
                        Comandos_EnviarLog(msgBuffer);
                    } else {
                        sprintf(msgBuffer, "Caja %c descartada (Destino 0)", claseGanadora);
                        Comandos_EnviarLog(msgBuffer);
                    }
                }
            } else {
                Comandos_EnviarLog("Objeto ignorado (Fuera de tolerancia)");
            }
            eEstacion0 = eEstacion0_Libre;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMAS DE EYECCIÓN
    // ---------------------------------------------------------

    ProcesarEstacionFSM(&eEstacion1, &rbEstacion1, &rbEstacion2, 1, 0, &servo1, &debounceIR1, 
                        msEsperar0, msDesplegar0, msRetraer0, ServoMax0, ServoMin0);
                        
    ProcesarEstacionFSM(&eEstacion2, &rbEstacion2, &rbEstacion3, 2, 1, &servo2, &debounceIR2, 
                        msEsperar1, msDesplegar1, msRetraer1, ServoMax1, ServoMin1);
                        
    ProcesarEstacionFSM(&eEstacion3, &rbEstacion3, NULL,         3, 2, &servo3, &debounceIR3, 
                        msEsperar2, msDesplegar2, msRetraer2, ServoMax2, ServoMin2);
    
}

void AppCinta_Iniciar(void){
    GPIO_SetCinta(true);
    eEstacion0 = eEstacion0_Libre;
}

void AppCinta_Detener(void){
    GPIO_SetCinta(false);
    eEstacion0 = eEstacion0_Detenido;
}

void AppCinta_SetVariable(_eSetVariables idVariable, uint32_t valor){
    static const char* nombresVariables[] = {
        "ServoMin0", "ServoMax0", "ServoMin1", "ServoMax1", "ServoMin2", "ServoMax2",
        "Ciego", "Delta", "CoordEstacion1", "CoordEstacion2", "CoordEstacion3",
        "msDesplegar0", "msRetraer0", "msEsperar0",
        "msDesplegar1", "msRetraer1", "msEsperar1",
        "msDesplegar2", "msRetraer2", "msEsperar2",
        "DistanciaBase", "DisparosMax",
        "HMinA", "HMaxA",
        "HMinB", "HMaxB",
        "HMinC", "HMaxC",
        "DestinoA", "DestinoB", "DestinoC"
    };
    switch(idVariable){
        case eSetServoMin0: ServoMin0 = valor; break;
        case eSetServoMax0: ServoMax0 = valor; break;
        case eSetServoMin1: ServoMin1 = valor; break;
        case eSetServoMax1: ServoMax1 = valor; break;
        case eSetServoMin2: ServoMin2 = valor; break;
        case eSetServoMax2: ServoMax2 = valor; break;
        
        // Las siguiente variables espacial (Delta, Limites) ya NO afectan al control,
        // pero las mantenemos para no romper la compatibilidad con el HMI de Qt.
        case eSetDelta: break;

        case eSetCiego: ciego = (bool)valor; break;
            
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

        case eSetHMinA: hMinA = valor; break;
        case eSetHMaxA: hMaxA = valor; break;
        case eSetHMinB: hMinB = valor; break;
        case eSetHMaxB: hMaxB = valor; break;
        case eSetHMinC: hMinC = valor; break;
        case eSetHMaxC: hMaxC = valor; break;
        
        case eSetDestinoA: destinoClaseA = (uint8_t)valor; break;
        case eSetDestinoB: destinoClaseB = (uint8_t)valor; break;
        case eSetDestinoC: destinoClaseC = (uint8_t)valor; break;
        default: return;
    }
    sprintf(msgBuffer, "SET: %s = %lu", nombresVariables[idVariable], valor);
    Comandos_EnviarLog(msgBuffer);
}

static void ProcesarEstacionFSM(_eEstaciones* estado,
                                RingBuffer_t* rbPropio,
                                RingBuffer_t* rbSiguiente,
                                uint8_t idEstacion,
                                uint8_t servoId,
                                Temporizador* tempServo,
                                Debouncer_t* sensorIR,
                                uint16_t msEsperar,
                                uint16_t msDesplegar,
                                uint16_t msRetraer,
                                uint8_t anguloMax,
                                uint8_t anguloMin) 
{
    switch(*estado) {
        case eEstaciones_Libre: {
            if (ciego) {
                // MODO CIEGO: Lazo abierto evaluando la integral de tiempo (ETA)
                _sCaja* proximaCaja = (_sCaja*) RingBuffer_Peek(rbPropio);
                if (proximaCaja != NULL && Temp_Expiro(&(proximaCaja->timerViaje))) {
                    RingBuffer_Pop(rbPropio, NULL); 
                    HAL_Servo_SetAngle(servoId, anguloMax);
                    Temp_IniciarMS(tempServo, msDesplegar);
                    *estado = eEstaciones_Desplegando;
                }
            } else {
                // MODO SENSOR: Lazo cerrado por interrupción óptica
                if (sensorIR->flanco_subida) {
                    _sCaja cajaActual;
                    if (RingBuffer_Pop(rbPropio, &cajaActual)) {
                        if (cajaActual.estacionDestino == idEstacion) {
                            Temp_IniciarMS(tempServo, msEsperar);
                            *estado = eEstaciones_Esperando;
                        } else if (rbSiguiente != NULL) {
                            // Enrutamiento en cascada: Pasa a la siguiente zona física
                            RingBuffer_Push(rbSiguiente, &cajaActual);
                        }
                    }
                }
            }
            break;
        }
        case eEstaciones_Esperando:
            if(Temp_Expiro(tempServo)){
                HAL_Servo_SetAngle(servoId, anguloMax);
                Temp_IniciarMS(tempServo, msDesplegar);
                *estado = eEstaciones_Desplegando;
            }
            break;
        case eEstaciones_Desplegando:
            if(Temp_Expiro(tempServo)){
                HAL_Servo_SetAngle(servoId, anguloMin);
                Temp_IniciarMS(tempServo, msRetraer);
                *estado = eEstaciones_Retrayendo;
            }
            break;
        case eEstaciones_Retrayendo:
            if(Temp_Expiro(tempServo)){
                *estado = eEstaciones_Libre;
            }
            break;
    }
}