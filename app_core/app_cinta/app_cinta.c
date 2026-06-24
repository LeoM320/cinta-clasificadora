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

static uint16_t distanciaBase = 180; 

static uint8_t nDisparosHcsr04;
static uint16_t disparosMm[20]; 
static uint8_t idMuestraAnterior = 0;
static char msgBuffer[80];

//Lista de cajas a procesar
static uint8_t cajasEnCola = 0;
static uint32_t velocidadGlobal = 0;

static _sCajas colaCajas[10];
static _eEstaciones eEstacion1 = eEstaciones_Libre;
static _eEstaciones eEstacion2 = eEstaciones_Libre;
//static _eEstaciones eEstacion3;
// caja ->

static Temporizador trasladarMm;
static uint32_t msInicioDePasada;
static uint32_t msFinalDePasada;

static uint16_t delta=4; //mm
static uint16_t coordenadaEstacion1=210; //mm Restandole 50mm de la caja
static uint16_t limiteInferiorEstacion1;
static uint16_t limiteSuperiorEstacion1;
static uint16_t coordenadaEstacion2=340; //mm
static uint16_t limiteInferiorEstacion2;
static uint16_t limiteSuperiorEstacion2;

static Temporizador servo1;
static Temporizador servo2;
//static Temporizador servo3;

static bool ciego=false;

void AppCinta_Init(void){
    Debounce_Init(&debounceIR0, 50, true);
    Debounce_Init(&debounceIR1, 50, true);
    Debounce_Init(&debounceIR2, 50, true);
    HCSR04_SetMode(true);
    limiteSuperiorEstacion1=coordenadaEstacion1+delta;
    limiteInferiorEstacion1=coordenadaEstacion1-delta;
    limiteSuperiorEstacion2=coordenadaEstacion2+delta;
    limiteInferiorEstacion2=coordenadaEstacion2-delta;
    Temp_IniciarMS(&trasladarMm, 100);
}

void AppCinta_Task(void){
    Debounce_Update(&debounceIR0, GPIO_LeerSensor(0));
    Debounce_Update(&debounceIR1, GPIO_LeerSensor(1));
    Debounce_Update(&debounceIR2, GPIO_LeerSensor(2));
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
            // Ahora cambia de estado apenas junte 10 muestras físicas o el sensor IR se libere
            if(debounceIR0.flanco_subida){
                msFinalDePasada = HAL_GetMillis();
                Comandos_EnviarLog("Fin de lectura de caja");
                eEstacion0 = eEstacion0_Despachando;
            }
            
            // Registramos TODAS las muestras para salir del estado rápido
            if(HCSR04_GetMuestraID() != idMuestraAnterior && nDisparosHcsr04 < 10){
                idMuestraAnterior = HCSR04_GetMuestraID();                
                disparosMm[nDisparosHcsr04] = HCSR04_GetDistance();
                nDisparosHcsr04++;
            }
            break;
            
        case eEstacion0_Despachando:
            cajaA = 0; cajaB = 0; cajaC = 0;
            sumaAlturas = 0;
            disparosValidos = 0;
            
            // Aquí es donde aplicamos el filtro estricto
            for(i = 0; i < nDisparosHcsr04; i++){
                if(distanciaBase >= disparosMm[i]){
                    uint16_t alturaTemporal = distanciaBase - disparosMm[i];
                    
                    // Solo contabilizamos la muestra si pertenece a las dimensiones buscadas
                    if((alturaTemporal >= 52 && alturaTemporal <= 68) ||
                       (alturaTemporal > 72 && alturaTemporal <= 88) ||
                       (alturaTemporal > 92 && alturaTemporal <= 108)){
                        
                        sumaAlturas += alturaTemporal;
                        disparosValidos++;
                        
                        // Votación de tipo de caja
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
                char charCaja = '?'; // Letra para simplificar el log
                
                if(cajaA > cajaB && cajaA > cajaC){
                    colaCajas[cajasEnCola].id = 0;
                    charCaja = 'A';
                }else if(cajaB > cajaA && cajaB > cajaC){
                    colaCajas[cajasEnCola].id = 1;
                    charCaja = 'B';
                }else if(cajaC > cajaA && cajaC > cajaB){
                    colaCajas[cajasEnCola].id = 2;
                    charCaja = 'C';
                }else{
                    colaCajas[cajasEnCola].id = 3;
                }
                
                // Calculamos la velocidad antes de armar el mensaje
                velocidadGlobal = 10000UL / (msFinalDePasada - msInicioDePasada); 
                
                // UN SOLO SPRINTF CON TODA LA INFO:
                sprintf(msgBuffer, "FIN LECTURA | Caja %c | Altura: %u mm | Vel: %lu", charCaja, alturaPromedio, velocidadGlobal);
                Comandos_EnviarLog(msgBuffer); 
                
                colaCajas[cajasEnCola].coordX = 0;
                colaCajas[cajasEnCola].e1 = 0;
                colaCajas[cajasEnCola].e2 = 0;
                colaCajas[cajasEnCola].e3 = 0;
                cajasEnCola++; 
                
            } else {
                Comandos_EnviarLog("Objeto ignorado (Fuera de rango)");
            }
            
            eEstacion0 = eEstacion0_Libre;
            break;
            
        default:
            break;
    }
    if(ciego){
        if(cajasEnCola>0&&Temp_Expiro(&trasladarMm)){
            Temp_Reiniciar(&trasladarMm);
            for(i=0;i<cajasEnCola;i++){
                colaCajas[i].coordX += velocidadGlobal;
                
                //Log de debug para ver la posición en tiempo real
                //sprintf(msgBuffer, "Caja[%u] (Tipo %u) -> X = %u mm", i, colaCajas[i].id, colaCajas[i].coordX);
                //Comandos_EnviarLog(msgBuffer);
            }
        }
        switch(eEstacion1){
            case eEstaciones_Libre:
                for(i=0;i<cajasEnCola;i++){
                    if(colaCajas[i].id==0){
                        coordenadaEvaluada=colaCajas[i].coordX;
                        if(coordenadaEvaluada>=limiteInferiorEstacion1&&coordenadaEvaluada<=limiteSuperiorEstacion1){
                            AppCinta_QuitarDeCola(i);
                            HAL_Servo_SetAngle(0, 0);
                            Temp_IniciarMS(&servo1, 300);
                            eEstacion1 = eEstaciones_Desplegando;
                            break;
                        }
                    }
                }
                break;
            case eEstaciones_Esperando:
                break;
            case eEstaciones_Desplegando:
                if(Temp_Expiro(&servo1)){
                    HAL_Servo_SetAngle(0, 90);
                    Temp_IniciarMS(&servo1, 300);
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
                            HAL_Servo_SetAngle(1, 0);
                            Temp_IniciarMS(&servo2, 300);
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
                    HAL_Servo_SetAngle(1, 90);
                    Temp_IniciarMS(&servo2, 300);
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
    }else{
        switch(eEstacion1){
            case eEstaciones_Libre:
                if(debounceIR1.flanco_subida){
                    for(i=0;i<cajasEnCola;i++){
                        if(colaCajas[i].e1==0){
                            if(colaCajas[i].id==0){
                                //Patear
                                AppCinta_QuitarDeCola(i);
                                Temp_IniciarMS(&servo1, 500);
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
                    HAL_Servo_SetAngle(0, 0);
                    Temp_IniciarMS(&servo1, 300);
                    eEstacion1 = eEstaciones_Desplegando;
                }
                break;
            case eEstaciones_Desplegando:
                if(Temp_Expiro(&servo1)){
                    HAL_Servo_SetAngle(0, 90);
                    Temp_IniciarMS(&servo1, 300);
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
                                Temp_IniciarMS(&servo2, 500);
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
                    HAL_Servo_SetAngle(1, 0);
                    Temp_IniciarMS(&servo2, 300);
                    eEstacion2 = eEstaciones_Desplegando;
                }
                break;
            case eEstaciones_Desplegando:
                if(Temp_Expiro(&servo2)){
                    HAL_Servo_SetAngle(1, 90);
                    Temp_IniciarMS(&servo2, 300);
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