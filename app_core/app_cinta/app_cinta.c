#include "../app_cinta/app_cinta.h"
#include "../app/comandos.h"
#include "../utils/temporizador.h"
#include "../hal/include/hal_servo.h"
#include "../hal/include/hal_timer.h"
#include <stdio.h>  

static _eEstacion0 eEstacion0 = eEstacion0_Libre;

static Debouncer_t debounceIR0;
static Debouncer_t debounceIR1;
static Debouncer_t debounceIR2;
static Debouncer_t debounceIR3;

static uint8_t nDisparosHcsr04;
static uint16_t disparosMm[20]; 
static uint8_t idMuestraAnterior = 0;
static char msgBuffer[80];

//Lista de cajas a procesar
static uint8_t cajasEnCola = 0;
static uint32_t velocidadGlobal = 0;

//static _sCajas colaCajas[10];
static _eEstaciones eEstacion1 = eEstaciones_Libre;
static _eEstaciones eEstacion2 = eEstaciones_Libre;
static _eEstaciones eEstacion3 = eEstaciones_Libre;

static Temporizador trasladarMm;
static uint32_t msInicioDePasada;
static uint32_t msFinalDePasada;

static Temporizador servo1;
static Temporizador servo2;
static Temporizador servo3;


static Temporizador cajas[30];

static uint8_t ServoMin0=90;
static uint8_t ServoMax0=0;
static uint8_t ServoMin1=90;
static uint8_t ServoMax1=0;
static uint8_t ServoMin2=90;
static uint8_t ServoMax2=0;
static bool ciego=false;
static uint16_t delta=4; //mm
static uint16_t coordenadaEstacion1=400; //mm Restandole 50mm de la caja
static uint16_t limiteInferiorEstacion1;
static uint16_t limiteSuperiorEstacion1;
static uint16_t coordenadaEstacion2=830; //mm
static uint16_t limiteInferiorEstacion2;
static uint16_t limiteSuperiorEstacion2;
static uint16_t coordenadaEstacion3=1310; //mm
static uint16_t limiteInferiorEstacion3;
static uint16_t limiteSuperiorEstacion3;
static uint16_t msDesplegar0=500;
static uint16_t msRetraer0=500;
static uint16_t msEsperar0=500;
static uint16_t msDesplegar1=500;
static uint16_t msRetraer1=500;
static uint16_t msEsperar1=500;
static uint16_t msDesplegar2=500;
static uint16_t msRetraer2=500;
static uint16_t msEsperar2=500;
static uint16_t distanciaBase=180;
static uint8_t disparosMax=10;

void AppCinta_Init(void){
    Debounce_Init(&debounceIR0, 50, true);
    Debounce_Init(&debounceIR1, 50, true);
    Debounce_Init(&debounceIR2, 50, true);
    Debounce_Init(&debounceIR3, 50, true);
    HCSR04_SetMode(true);
    limiteSuperiorEstacion1=coordenadaEstacion1+delta;
    limiteInferiorEstacion1=coordenadaEstacion1-delta;
    limiteSuperiorEstacion2=coordenadaEstacion2+delta;
    limiteInferiorEstacion2=coordenadaEstacion2-delta;
    limiteSuperiorEstacion3=coordenadaEstacion3+delta;
    limiteInferiorEstacion3=coordenadaEstacion3-delta;
    Temp_IniciarMS(&trasladarMm, 100);
}

void AppCinta_Task(void){
    Debounce_Update(&debounceIR0, GPIO_LeerSensor(0));
    Debounce_Update(&debounceIR1, GPIO_LeerSensor(1));
    Debounce_Update(&debounceIR2, GPIO_LeerSensor(2));
    Debounce_Update(&debounceIR3, GPIO_LeerSensor(3));
    uint8_t cajaA, cajaB, cajaC, i;
    uint32_t sumaAlturas = 0;
    uint8_t disparosValidos = 0; //Ver si reiniciar
    uint32_t coordenadaEvaluada;

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
            
            // Registramos TODAS las muestras para salir del estado rápido
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
            
            if(disparosValidos > 0){
                uint16_t alturaPromedio = (uint16_t)(sumaAlturas / disparosValidos);
                char charCaja = '?';
                uint16_t msEsperar;
                if(cajaA > cajaB && cajaA > cajaC){
                    msEsperar = (coordenadaEstacion1*(msFinalDePasada - msInicioDePasada))/100;
                    Temp_IniciarMS(&cajas[cajasEnCola++], msEsperar);
                    charCaja = 'A';
                    sprintf(msgBuffer, "FIN LECTURA | Caja %c | Altura: %u mm | Ms: %lu", charCaja, alturaPromedio, msEsperar);
                    Comandos_EnviarLog(msgBuffer);
                }else if(cajaB > cajaA && cajaB > cajaC){
                    msEsperar = (coordenadaEstacion2*(msFinalDePasada - msInicioDePasada))/100;
                    Temp_IniciarMS(&cajas[cajasEnCola++], msEsperar);
                    charCaja = 'B';
                    sprintf(msgBuffer, "FIN LECTURA | Caja %c | Altura: %u mm | Ms: %lu", charCaja, alturaPromedio, msEsperar);
                    Comandos_EnviarLog(msgBuffer);
                }else if(cajaC > cajaA && cajaC > cajaB){
                    msEsperar = (coordenadaEstacion3*(msFinalDePasada - msInicioDePasada))/100; //mm/ms
                    Temp_IniciarMS(&cajas[cajasEnCola++], msEsperar);
                    charCaja = 'C';
                    sprintf(msgBuffer, "FIN LECTURA | Caja %c | Altura: %u mm | Ms: %lu", charCaja, alturaPromedio, msEsperar);
                    Comandos_EnviarLog(msgBuffer);
                }
            } else {
                Comandos_EnviarLog("Objeto ignorado (Fuera de rango)");
            }
            eEstacion0 = eEstacion0_Libre;
            break;
        default:
            break;
    }
    if(ciego){
        switch(eEstacion1){
            case eEstaciones_Libre:
                for(i=0;i<cajasEnCola;i++){
                    if(Temp_Expiro(&cajas[i])){
                        AppCinta_QuitarDeCola(i);
                        HAL_Servo_SetAngle(0, ServoMax0);
                        Temp_IniciarMS(&servo1, msDesplegar0);
                        eEstacion1 = eEstaciones_Desplegando;
                        break;
                    }
                }
                break;
            case eEstaciones_Esperando:
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
            default:
                break;
        }
        switch(eEstacion2){
            case eEstaciones_Libre:
                for(i=0;i<cajasEnCola;i++){
                    if(colaCajas[i].id==1){
                        coordenadaEvaluada=colaCajas[i].coordX;
                        if(coordenadaEvaluada>=limiteInferiorEstacion2&&coordenadaEvaluada<=limiteSuperiorEstacion2){
                            AppCinta_QuitarDeCola(i);
                            HAL_Servo_SetAngle(1, ServoMax1);
                            Temp_IniciarMS(&servo2, msDesplegar1);
                            eEstacion2 = eEstaciones_Desplegando;
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
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
            default:
                break;
        }
        switch(eEstacion3){
            case eEstaciones_Libre:
                for(i=0;i<cajasEnCola;i++){
                    if(colaCajas[i].id==2){
                        coordenadaEvaluada=colaCajas[i].coordX;
                        if(coordenadaEvaluada>=limiteInferiorEstacion3&&coordenadaEvaluada<=limiteSuperiorEstacion3){
                            AppCinta_QuitarDeCola(i);
                            HAL_Servo_SetAngle(2, ServoMax2);
                            Temp_IniciarMS(&servo3, msDesplegar2);
                            eEstacion3 = eEstaciones_Desplegando;
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
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
            default:
                break;
        }
    }else{
        switch(eEstacion1){
            case eEstaciones_Libre:
                if(debounceIR1.flanco_subida){
                    for(i=0;i<cajasEnCola;i++){
                        if(colaCajas[i].e1==0){
                            if(colaCajas[i].id==0){
                                //Patear
                                AppCinta_QuitarDeCola(i);
                                Temp_IniciarMS(&servo1, msEsperar0);
                                eEstacion1 = eEstaciones_Esperando;
                            }else{
                                colaCajas[i].e1 = 1;
                            }
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
                if(Temp_Expiro(&servo1)){
                    HAL_Servo_SetAngle(0, ServoMax0);
                    Temp_IniciarMS(&servo1, msDesplegar0);
                    eEstacion1 = eEstaciones_Desplegando;
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
            default:
                break;
        }
        switch(eEstacion2){
            case eEstaciones_Libre:
                if(debounceIR2.flanco_subida){
                    for(i=0;i<cajasEnCola;i++){
                        if(colaCajas[i].e2==0){
                            if(colaCajas[i].id==1){
                                //Patear
                                AppCinta_QuitarDeCola(i);
                                Temp_IniciarMS(&servo2, msEsperar1);
                                eEstacion2 = eEstaciones_Esperando;
                            }else{
                                colaCajas[i].e2 = 1;
                            }
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
                if(Temp_Expiro(&servo2)){
                    HAL_Servo_SetAngle(1, ServoMax1);
                    Temp_IniciarMS(&servo2, msDesplegar1);
                    eEstacion2 = eEstaciones_Desplegando;
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
            default:
                break;
        }
        switch(eEstacion3){
            case eEstaciones_Libre:
                if(debounceIR3.flanco_subida){
                    for(i=0;i<cajasEnCola;i++){
                        if(colaCajas[i].e3==0){
                            if(colaCajas[i].id==2){
                                //Patear
                                AppCinta_QuitarDeCola(i);
                                Temp_IniciarMS(&servo3, msEsperar2);
                                eEstacion3 = eEstaciones_Esperando;
                            }else{
                                colaCajas[i].e3 = 1;
                            }
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
                if(Temp_Expiro(&servo3)){
                    HAL_Servo_SetAngle(2, ServoMax2);
                    Temp_IniciarMS(&servo3, msDesplegar2);
                    eEstacion3 = eEstaciones_Desplegando;
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
            default:
                break;
        }
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
    for(uint8_t i=pos;i<(cajasEnCola-1);i++){
        cajas[i]

        colaCajas[i].id=colaCajas[i+1].id;
        colaCajas[i].coordX=colaCajas[i+1].coordX;
        colaCajas[i].e1=colaCajas[i+1].e1;
        colaCajas[i].e2=colaCajas[i+1].e2;
        colaCajas[i].e3=colaCajas[i+1].e3;
    }
    colaCajas[cajasEnCola - 1].id=0;
    colaCajas[cajasEnCola - 1].coordX=0;
    colaCajas[cajasEnCola - 1].e1=0;
    colaCajas[cajasEnCola - 1].e2=0;
    colaCajas[cajasEnCola - 1].e3=0;
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
        case eSetServoMin0:
            ServoMin0 = valor;
            break;
        case eSetServoMax0:
            ServoMax0 = valor;
            break;
        case eSetServoMin1:
            ServoMin1 = valor;
            break;
        case eSetServoMax1:
            ServoMax1 = valor;
            break;
        case eSetServoMin2:
            ServoMin2 = valor;
            break;
        case eSetServoMax2:
            ServoMax2 = valor;
            break;
        case eSetCiego:
            ciego = valor;
            break;
        case eSetDelta:
            delta = valor;
            limiteSuperiorEstacion1 = coordenadaEstacion1 + delta;
            limiteInferiorEstacion1 = coordenadaEstacion1 - delta;
            limiteSuperiorEstacion2 = coordenadaEstacion2 + delta;
            limiteInferiorEstacion2 = coordenadaEstacion2 - delta;
            limiteSuperiorEstacion3 = coordenadaEstacion3 + delta;
            limiteInferiorEstacion3 = coordenadaEstacion3 - delta;
            break;
        case eSetCoordenadaEstacion1:
            coordenadaEstacion1 = valor;
            limiteSuperiorEstacion1 = coordenadaEstacion1 + delta;
            limiteInferiorEstacion1 = coordenadaEstacion1 - delta;
            break;
        case eSetCoordenadaEstacion2:
            coordenadaEstacion2 = valor;
            limiteSuperiorEstacion2 = coordenadaEstacion2 + delta;
            limiteInferiorEstacion2 = coordenadaEstacion2 - delta;
            break;
        case eSetCoordenadaEstacion3:
            coordenadaEstacion3 = valor;
            limiteSuperiorEstacion3 = coordenadaEstacion3 + delta;
            limiteInferiorEstacion3 = coordenadaEstacion3 - delta;
            break;
        case eSetMsDesplegar0:
            msDesplegar0 = valor;
            break;
        case eSetMsRetraer0:
            msRetraer0 = valor;
            break;
        case eSetMsEsperar0:
            msEsperar0 = valor;
            break;
        case eSetMsDesplegar1:
            msDesplegar1 = valor;
            break;
        case eSetMsRetraer1:
            msRetraer1 = valor;
            break;
        case eSetMsEsperar1:
            msEsperar1 = valor;
            break;
        case eSetMsDesplegar2:
            msDesplegar2 = valor;
            break;
        case eSetMsRetraer2:
            msRetraer2 = valor;
            break;
        case eSetMsEsperar2:
            msEsperar2 = valor;
            break;
        case eSetDistanciaBase:
            distanciaBase = valor;
            break;
        case eSetDisparosMax:
            disparosMax = valor;
            break;
        default:
            return;
    }
    sprintf(msgBuffer, "SET: %s = %lu", nombresVariables[idVariable], valor);
    Comandos_EnviarLog(msgBuffer);
}