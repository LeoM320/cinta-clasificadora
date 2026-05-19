/**
 * @file main.c
 * @brief Orquestador principal (Super Loop) de la cinta clasificadora.
 * @author LeoM320
 * @date 19 de Mayo de 2026
 *
 * @details
 * Punto de entrada del firmware del ATmega328P. 
 * Implementa una arquitectura Master-Slave asíncrona. El microcontrolador 
 * evalúa continuamente el hardware (sensores y comunicaciones) mediante 
 * Máquinas de Estados Finitos (FSM) y delega la ejecución de órdenes 
 * al despachador de comandos (`comandos.c`), sin detener jamás la CPU.
 */

// ==========================================
// 1. CONFIGURACIÓN FÍSICA Y BSP
// ==========================================
#include "config/hardware.h"
#include "config/gpio.h"

// ==========================================
// 2. CAPA DE ABSTRACCIÓN DE HARDWARE (HAL)
// ==========================================
#include "hal/include/hal_gpio.h"  // <--- SOLUCIÓN: Agregado para usar HAL_GPIO_READ
#include "hal/include/hal_timer.h"
#include "hal/include/hal_uart.h"
#include "hal/include/hal_adc.h"
#include "hal/include/hal_servo.h"

// ==========================================
// 3. MIDDLEWARE Y DRIVERS
// ==========================================
#include "drivers/hcsr04.h"
#include "utils/heartbeat.h"
#include "utils/temporizador.h"
#include "utils/debounce.h"
#include "utils/uner_protocol.h"

// ==========================================
// 4. LÓGICA DE APLICACIÓN
// ==========================================
#include "app/comandos.h"

/** @brief Instancia global del motor de protocolo UNER para telemetría. */
UnerProtocol_t comms;

/** @brief Array de filtros anti-rebote para los 4 sensores infrarrojos TCRT5000. */
Debouncer_t ir_debouncers[4];

/** @brief Temporizador para disparar el sensor ultrasónico de forma segura. */
Temporizador timer_ultrasonico;

/**
 * @brief Rutina principal de inicialización y bucle infinito.
 * 
 * @return int Retorno de compatibilidad estándar (nunca se alcanza).
 */
int main(void)
{
    // ==========================================
    // FASE 1: SECCIÓN CRÍTICA DE INICIALIZACIÓN
    // ==========================================
    // Se apagan las interrupciones para evitar que un periférico a medio 
    // configurar dispare una ISR y corrompa la memoria o el flujo del programa.
    HAL_DISABLE_INTERRUPTS();

    // 1.1 Configuración de pines (Safe-State por defecto)
    GPIO_Init();

    // 1.2 Inicialización de Periféricos del Silicio (HAL)
    HAL_Timer0_Init();             // Reloj base (SysTick de 1ms y alta resolución)
    HAL_UART_Init(115200);         // Bus serie para Qt (Doble velocidad habilitada)
    HAL_ADC_Init();                // Conversor A/D preparado a 125 KHz
    HAL_Servo_Init();              // PWM Multiplexado en Timer1 para los SG90

    // 1.3 Inicialización de Controladores de Software
    HCSR04_Init();
    
    // Iniciar el reloj de disparos del ultrasónico a 60ms (Recomendación del datasheet)
    Temp_IniciarMS(&timer_ultrasonico, 60);
    
    // Filtros anti-rebote para los IR: 15ms de estabilidad exigida, reposo en LOW (0)
    Debounce_Init(&ir_debouncers[0], 15, false);
    Debounce_Init(&ir_debouncers[1], 15, false);
    Debounce_Init(&ir_debouncers[2], 15, false);
    Debounce_Init(&ir_debouncers[3], 15, false);

    // Heartbeat: Doble destello corto (0b10100000 = 0xA0) cada 100ms por bit
    Heartbeat_Init(100, 0xA0); 
    
    // Inicializar la FSM del protocolo serial
    Uner_Init(&comms);

    // ==========================================
    // FASE 2: ARRANQUE DEL SISTEMA
    // ==========================================
    // Todos los registros están listos. Abrimos la compuerta de eventos de hardware.
    HAL_ENABLE_INTERRUPTS();

    // ==========================================
    // FASE 3: SUPER LOOP NO BLOQUEANTE
    // ==========================================
    while (1)
    {
        // 1. Capturar la marca de tiempo base para las FSMs
        uint32_t current_ms = HAL_GetMillis();

        // ------------------------------------------
        // TAREAS CRÍTICAS DE TIEMPO REAL
        // ------------------------------------------
        Heartbeat_Task();
        HCSR04_Task();

        // Disparar el sensor de forma autónoma y asíncrona
        if (Temp_Expiro(&timer_ultrasonico)) {
            HCSR04_Trigger();
            Temp_Reiniciar(&timer_ultrasonico);
        }

        // Actualización de los filtros de los sensores ópticos
        Debounce_Update(&ir_debouncers[0], HAL_GPIO_READ(IR0_PIN_REG, IR0_PIN) != 0);
        Debounce_Update(&ir_debouncers[1], HAL_GPIO_READ(IR1_PIN_REG, IR1_PIN) != 0);
        Debounce_Update(&ir_debouncers[2], HAL_GPIO_READ(IR2_PIN_REG, IR2_PIN) != 0);
        Debounce_Update(&ir_debouncers[3], HAL_GPIO_READ(IR3_PIN_REG, IR3_PIN) != 0);

        // ------------------------------------------
        // COMUNICACIONES Y DESPACHO DE ÓRDENES
        // ------------------------------------------
        // Extraer datos de la UART (Ring Buffer) e inyectarlos a la FSM
        Uner_Recibir(&comms, current_ms);
        
        // Si hay un paquete validado, procesarlo y responder al Host
        Comandos_Procesar(&comms);

        // ------------------------------------------
        // LÓGICA DE CLASIFICACIÓN AUTÓNOMA (Futuro)
        // ------------------------------------------
        /*
        // Ejemplo de uso para cuando la cinta deba tomar decisiones por sí misma:
        if (ir_debouncers[0].estado_validado) {
            // Pieza detectada en la estación 1, frenar cinta y mover servo.
            HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
            HAL_Servo_SetAngle(SERVO_1, 90);
        }
        */
    }

    return 0;
}