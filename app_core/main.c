/**
 * @file main.c
 * @brief Control reactivo con filtrado anti-rebote en IR y Media Móvil en Ultrasonido.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */

#include <stdint.h>
#include <stdbool.h>

/* Configuración y HAL */
#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/include/hal_timer.h"
#include "hal/include/hal_gpio.h"
#include "hal/include/hal_servo.h"

/* Utilidades y Drivers */
#include "utils/debounce.h"
#include "utils/temporizador.h"
#include "drivers/hcsr04.h"

#define IR_DEBOUNCE_TIME_MS 50   /**< Filtro de 50ms para eliminar jitter del LM393 */
#define SENSOR_US_POLL_RATE 150  /**< Frecuencia de disparo del HC-SR04 (150ms) */
#define US_FILTER_SAMPLES 4      /**< Cantidad de muestras para el filtro de media móvil */

int main(void) {
    // ==========================================================
    // 1. INICIALIZACIÓN DE HARDWARE Y PERIFÉRICOS
    // ==========================================================
    HAL_Timer0_Init();      
    GPIO_Init();            
    HAL_Servo_Init();       
    HCSR04_Init();          
    
    HAL_ENABLE_INTERRUPTS();

    // ==========================================================
    // 2. CONFIGURACIÓN INICIAL DE ACTUADORES
    // ==========================================================
    HAL_Servo_Enable(SERVO_1);
    HAL_Servo_Enable(SERVO_2);
    
    HAL_Servo_SetAngle(SERVO_1, 90);
    HAL_Servo_SetAngle(SERVO_2, 90);
    
    HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
    bool cinta_encendida = false;
    
    // ==========================================================
    // 3. INICIALIZACIÓN DE VARIABLES Y FILTROS
    // ==========================================================
    Debouncer_t filtro_ir1;
    Debouncer_t filtro_ir2;
    Debounce_Init(&filtro_ir1, IR_DEBOUNCE_TIME_MS, false);
    Debounce_Init(&filtro_ir2, IR_DEBOUNCE_TIME_MS, false);

    bool estado_anterior_ir1 = false; 
    bool estado_anterior_ir2 = false;
    
    Temporizador timer_ultrasonico;
    Temp_IniciarMS(&timer_ultrasonico, SENSOR_US_POLL_RATE);

    // Buffer circular para el filtro de Media Móvil del HC-SR04
    uint16_t us_buffer[US_FILTER_SAMPLES];
    uint8_t us_index = 0;
    
    // Pre-cargar el buffer con un valor fuera del rango de activación (ej. 100 cm)
    // para evitar que la cinta arranque sola por los ceros iniciales en memoria.
    for (uint8_t i = 0; i < US_FILTER_SAMPLES; i++) {
        us_buffer[i] = 100;
    }

    // ==========================================================
    // 4. BUCLE PRINCIPAL (SUPER LOOP)
    // ==========================================================
    while (1) {
        
        HCSR04_Task();
        
        /* ----------------------------------------------------------
         * ESTACIÓN 1 (IR1 -> SERVO_1)
         * ---------------------------------------------------------- */
        bool crudo_ir1 = (HAL_GPIO_READ(IR1_PIN_REG, IR1_PIN) == 0);
        bool detectado_ir1 = Debounce_Update(&filtro_ir1, crudo_ir1);
        
        if (detectado_ir1 != estado_anterior_ir1) {
            HAL_Servo_SetAngle(SERVO_1, detectado_ir1 ? 0 : 90);
            estado_anterior_ir1 = detectado_ir1;
        }
        
        /* ----------------------------------------------------------
         * ESTACIÓN 2 (IR2 -> SERVO_2)
         * ---------------------------------------------------------- */
        bool crudo_ir2 = (HAL_GPIO_READ(IR2_PIN_REG, IR2_PIN) == 0);
        bool detectado_ir2 = Debounce_Update(&filtro_ir2, crudo_ir2);
        
        if (detectado_ir2 != estado_anterior_ir2) {
            HAL_Servo_SetAngle(SERVO_2, detectado_ir2 ? 0 : 90);
            estado_anterior_ir2 = detectado_ir2;
        }
        
        /* ----------------------------------------------------------
         * ESTACIÓN DE ENTRADA (ULTRASONIDO -> CINTA)
         * ---------------------------------------------------------- */
         
        if (Temp_Expiro(&timer_ultrasonico)) {
            HCSR04_Trigger();
            Temp_Reiniciar(&timer_ultrasonico);
        }
        
        if (HCSR04_IsDataReady()) {
            uint16_t nueva_distancia = HCSR04_GetDistance();
            
            // 1. Ingresar el nuevo dato al buffer circular
            us_buffer[us_index] = nueva_distancia;
            us_index = (us_index + 1) % US_FILTER_SAMPLES;
            
            // 2. Calcular el promedio actual
            uint32_t suma = 0;
            for (uint8_t i = 0; i < US_FILTER_SAMPLES; i++) {
                suma += us_buffer[i];
            }
            uint16_t distancia_filtrada = suma / US_FILTER_SAMPLES;
            
            // 3. Evaluar la condición con la señal ya suavizada
            bool en_rango = (distancia_filtrada >= 5 && distancia_filtrada <= 10);
            
            if (en_rango && !cinta_encendida) {
                HAL_GPIO_WRITE_HIGH(CINTA_PORT, CINTA_PIN);
                cinta_encendida = true;
            } 
            else if (!en_rango && cinta_encendida) {
                HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
                cinta_encendida = false;
            }
        }
    }
    
    return 0; 
}