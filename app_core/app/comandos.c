// ==========================================
// app/comandos.c
// ==========================================

/**
 * @file comandos.c
 * @brief Implementación lógica del procesador de comandos de la aplicación.
 */

#include "comandos.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_timer.h"
#include "../hal/include/hal_servo.h"
#include "../drivers/hcsr04.h"

static const uint8_t FIRMWARE_VERSION[3] = {1, 0, 0};

void Comandos_Procesar(UnerProtocol_t *u) {
    
    // Validar si hay una nueva trama lista desde la FSM
    if (Uner_Comando(u)) {
        
        uint8_t cmd_id = Uner_IDComando(u);

        switch(cmd_id) {
            
            // ------------------------------------------
            // SISTEMA Y TELEMETRÍA BASE
            // ------------------------------------------
            case CMD_ALIVE:
            {
                uint32_t uptime = HAL_GetMillis();
                Uner_AbrirCarga(u, 5);
                Uner_Agregar8(u, ACK_ALIVE);
                Uner_Agregar32(u, uptime);
                Uner_CerrarCarga(u);
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

            // ------------------------------------------
            // CONTROL DE ACTUADORES
            // ------------------------------------------
            case CMD_SET_SERVO:
            {
                // Validación de seguridad: Requiere Servo_ID y Ángulo
                if (u->rx.length >= 3) {
                    uint8_t servo_id = Uner_Obtener8(u, 1);
                    uint8_t angulo = Uner_Obtener8(u, 2);
                    
                    HAL_Servo_Enable(servo_id);
                    HAL_Servo_SetAngle(servo_id, angulo);
                    
                    Uner_AbrirCarga(u, 2);
                    Uner_Agregar8(u, ACK_SET_SERVO);
                    Uner_Agregar8(u, servo_id); // Confirmamos qué servo se movió
                    Uner_CerrarCarga(u);
                } else {
                    Uner_AbrirCarga(u, 2);
                    Uner_Agregar8(u, ERR_BAD_PAYLOAD);
                    Uner_Agregar8(u, cmd_id);
                    Uner_CerrarCarga(u);
                }
                break;
            }

            case CMD_SET_BELT:
            {
                // Comando para la Cinta: 1 (Arrancar), 0 (Detener)
                if (u->rx.length >= 2) {
                    uint8_t estado = Uner_Obtener8(u, 1);
                    
                    if (estado > 0) {
                        HAL_GPIO_WRITE_HIGH(CINTA_PORT, CINTA_PIN);
                    } else {
                        HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
                    }

                    Uner_AbrirCarga(u, 2);
                    Uner_Agregar8(u, ACK_SET_BELT);
                    Uner_Agregar8(u, estado > 0 ? 1 : 0); // Confirmamos estado aplicado
                    Uner_CerrarCarga(u);
                }
                break;
            }

            // ------------------------------------------
            // LECTURA DE SENSORES
            // ------------------------------------------
            case CMD_GET_DISTANCE:
            {
                // Retorna la última distancia validada por la FSM no bloqueante
                uint16_t distancia = HCSR04_GetDistance();
                
                Uner_AbrirCarga(u, 3);
                Uner_Agregar8(u, ACK_GET_DISTANCE);
                Uner_Agregar16(u, distancia);
                Uner_CerrarCarga(u);
                break;
            }

            case CMD_GET_IR_STATES:
            {
                // Bit-Packing: Condensar 4 lecturas de pines en 1 solo byte
                // Formato: 0000|IR3|IR2|IR1|IR0
                uint8_t ir_pack = 0;
                
                // Si el sensor lee ALTO (asumiendo lógica directa), levantamos el bit correspondiente
                if (HAL_GPIO_READ(IR0_PIN_REG, IR0_PIN)) ir_pack |= (1 << 0);
                if (HAL_GPIO_READ(IR1_PIN_REG, IR1_PIN)) ir_pack |= (1 << 1);
                if (HAL_GPIO_READ(IR2_PIN_REG, IR2_PIN)) ir_pack |= (1 << 2);
                if (HAL_GPIO_READ(IR3_PIN_REG, IR3_PIN)) ir_pack |= (1 << 3);

                Uner_AbrirCarga(u, 2);
                Uner_Agregar8(u, ACK_GET_IR_STATES);
                Uner_Agregar8(u, ir_pack);
                Uner_CerrarCarga(u);
                break;
            }

            // ------------------------------------------
            // MANEJO DE ERRORES
            // ------------------------------------------
            default:
            {
                Uner_AbrirCarga(u, 2);
                Uner_Agregar8(u, ERR_UNKNOWN_CMD);
                Uner_Agregar8(u, cmd_id);
                Uner_CerrarCarga(u);
                break;
            }
        }
        
        // Ejecutar transmisión física al Host
        Uner_Transmitir(u);
    }
}