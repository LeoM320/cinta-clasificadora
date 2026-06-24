#include "comandos.h"
#include "../config/hardware.h"
#include "../config/gpio.h"
#include "../hal/include/hal_timer.h"
#include "../hal/include/hal_servo.h"
#include "../drivers/hcsr04.h"
#include "../app_cinta/app_cinta.h"
#include <string.h>

extern UnerProtocol_t protocolo_uart;
static const uint8_t FIRMWARE_VERSION[3] = {1, 0, 0};

void Comandos_Procesar(UnerProtocol_t *u) {
    if (Uner_Comando(u)) {
        uint8_t cmd_id = Uner_IDComando(u);

        switch(cmd_id) {
            
            case CMD_HANDSHAKE:
            {
                Uner_AbrirCarga(u, 2);
                Uner_Agregar8(u, ACK_HANDSHAKE); 
                Uner_Agregar8(u, 0x00); 
                Uner_CerrarCarga(u);
                
                // Disparamos el callback de Conexión
                if (u->on_conexion != NULL) {
                    u->on_conexion();
                }
                break;
            }

            case CMD_PING: 
            {
                uint8_t contador = 0;
                if (u->rx.length >= 2) {
                    contador = Uner_Obtener8(u, 1);
                }

                Uner_AbrirCarga(u, 2);               
                Uner_Agregar8(u, ACK_PONG);          
                Uner_Agregar8(u, contador);          
                Uner_CerrarCarga(u);
                break;
            }

            case CMD_CLOSE: 
            {
                // Si el host cierra intencionalmente, disparamos la desconexión
                if (u->on_desconexion != NULL) {
                    u->on_desconexion();
                }
                break;
            }

            case CMD_GET_VERSION:
            {
                Uner_AbrirCarga(u, 4);
                Uner_Agregar8(u, ACK_GET_VERSION);
                Uner_Agregar8(u, FIRMWARE_VERSION[0]);
                Uner_Agregar8(u, FIRMWARE_VERSION[1]);
                Uner_Agregar8(u, FIRMWARE_VERSION[2]);
                Uner_CerrarCarga(u);
                break;
            }

            case CMD_SET_SERVO:
            {
                if (u->rx.length >= 3) {
                    uint8_t servo_id = Uner_Obtener8(u, 1);
                    uint8_t angulo = Uner_Obtener8(u, 2);
                    
                    HAL_Servo_SetAngle(servo_id, angulo);
                    
                    Uner_AbrirCarga(u, 2);
                    Uner_Agregar8(u, ACK_SET_SERVO);
                    Uner_Agregar8(u, servo_id); 
                    Uner_CerrarCarga(u);
                }
                break;
            }

            case CMD_SET_BELT:
            {
                if (u->rx.length >= 2) {
                    uint8_t estado = Uner_Obtener8(u, 1);
                    
                    GPIO_SetCinta(estado > 0);

                    Uner_AbrirCarga(u, 2);
                    Uner_Agregar8(u, ACK_SET_BELT);
                    Uner_Agregar8(u, estado > 0 ? 1 : 0);
                    Uner_CerrarCarga(u);
                }
                break;
            }

            case CMD_GET_DISTANCE:
            {
                uint16_t dist_mm = HCSR04_GetDistance(); 

                Uner_AbrirCarga(u, 3);
                Uner_Agregar8(u, ACK_GET_DISTANCE);
                Uner_Agregar16(u, dist_mm); 
                Uner_CerrarCarga(u);
                break;
            }

            case CMD_GET_IR_STATES:
            {
                uint8_t pack = 0;
                if (GPIO_LeerSensor(0)) pack |= (1 << 0);
                if (GPIO_LeerSensor(1)) pack |= (1 << 1);
                if (GPIO_LeerSensor(2)) pack |= (1 << 2);
                if (GPIO_LeerSensor(3)) pack |= (1 << 3);
                
                Uner_AbrirCarga(u, 2);
                Uner_Agregar8(u, ACK_GET_IR_STATES);
                Uner_Agregar8(u, pack);
                Uner_CerrarCarga(u);
                break;
            }

            case CMD_SET_VARIABLE:
            {
                _eSetVariables idVariable = Uner_Obtener8(u,1);
                uint32_t valor = Uner_Obtener32(u,2);
                AppCinta_SetVariable(idVariable, valor);
                break;
            }

            default:
            {
                Uner_AbrirCarga(u, 2);
                Uner_Agregar8(u, ERR_UNKNOWN_CMD);
                Uner_Agregar8(u, cmd_id);
                Uner_CerrarCarga(u);
                break;
            }
        }
        
        //Uner_Transmitir(u);
    }
}

void Comandos_EnviarLog(const char* mensaje) {
    uint8_t len = strlen(mensaje);
    if (len > 60) len = 60; 

    Uner_AbrirCarga(&protocolo_uart, len + 2); 
    Uner_Agregar8(&protocolo_uart, CMD_SEND_LOG); 
    
    for(uint8_t i = 0; i < len; i++) {
        Uner_Agregar8(&protocolo_uart, (uint8_t)mensaje[i]);
    }
    
    Uner_Agregar8(&protocolo_uart, '\0'); 
    Uner_CerrarCarga(&protocolo_uart);
    //Uner_Transmitir(&protocolo_uart);
}