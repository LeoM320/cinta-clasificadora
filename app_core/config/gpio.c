/**
 * @file gpio.c
 * @brief Implementación de la rutina de inicialización de pines del sistema.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 */

#include "gpio.h"
#include "hardware.h"
#include "../hal/include/hal_gpio.h"

/**
 * @brief Prepara eléctricamente los puertos del microcontrolador al arrancar.
 * 
 * @details 
 * La estrategia de configuración para los pines de salida sigue un patrón "Glitch-Free":
 * Primero se fuerza el registro de datos (`PORTx`) a un nivel bajo, y luego se abre 
 * la compuerta de dirección (`DDRx`). Esto evita que los actuadores o servomotores 
 * den un "tirón" indeseado al encender el equipo.
 * 
 * Configuraciones aplicadas:
 * - **HC-SR04:** Pin Trigger como Salida (estado BAJO); Pin Echo como Entrada (alta impedancia).
 * - **Servomotores (PWM):** Pines forzados a Salida en estado BAJO.
 * - **Indicadores:** LED de diagnóstico como Salida.
 * - **Sensores IR (TCRT5000):** Pines digitales configurados como Entradas limpias (sin pull-up).
 * - **Potencia:** Cinta transportadora apagada (Salida en BAJO).
 * 
 * @note Como optimización de bajo consumo, se deshabilita el buffer de entrada 
 *       digital en los canales analógicos utilizados por los sensores IR mediante 
 *       el registro DIDR0 (Digital Input Disable Register 0).
 */
void GPIO_Init(void)
{
    // ==========================================
    // Sensor Ultrasónico HC-SR04
    // ==========================================
    HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN); // 1. Asegurar estado BAJO
    HAL_GPIO_SET_OUTPUT(TRIGGER_DDR, TRIGGER_PIN); // 2. Abrir como SALIDA

    HAL_GPIO_WRITE_LOW(ECHO_PORT, ECHO_PIN);       // 1. Desactivar resistencia Pull-up
    HAL_GPIO_SET_INPUT(ECHO_DDR, ECHO_PIN);        // 2. Configurar como ENTRADA de alta impedancia

    // ¡ESTO ES VITAL PARA QUE FUNCIONE EL SENSOR!
    PCICR |= (1 << PCIE0);       // Habilita interrupciones en el Puerto B
    PCMSK0 |= (1 << ECHO_PIN);   // Desenmascara específicamente el pin del Eco (PB2)

    // ==========================================
    // Servomotores (Actuadores de clasificación)
    // ==========================================
    HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO1_DDR, SERVO1_PIN);
    
    HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO2_DDR, SERVO2_PIN);
    
    HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO3_DDR, SERVO3_PIN);
    
    // ==========================================
    // Telemetría / LED de Estado
    // ==========================================
    HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
    HAL_GPIO_SET_OUTPUT(STATUS_LED_DDR, STATUS_LED_PIN);

    // ==========================================
    // Sensores Infrarrojos TCRT5000 (Modo Digital)
    // ==========================================
    HAL_GPIO_WRITE_LOW(IR0_PORT, IR0_PIN);
    HAL_GPIO_SET_INPUT(IR0_DDR, IR0_PIN);
    
    HAL_GPIO_WRITE_LOW(IR1_PORT, IR1_PIN);
    HAL_GPIO_SET_INPUT(IR1_DDR, IR1_PIN);
    
    HAL_GPIO_WRITE_LOW(IR2_PORT, IR2_PIN);
    HAL_GPIO_SET_INPUT(IR2_DDR, IR2_PIN);
    
    HAL_GPIO_WRITE_LOW(IR3_PORT, IR3_PIN);
    HAL_GPIO_SET_INPUT(IR3_DDR, IR3_PIN);
    
    // Optimización de energía:
    // Si los canales ADC se usan para leer la versión analógica de los IR, 
    // deshabilitamos el comparador digital en esos pines físicos (A2, A3, A4, A5) 
    // para evitar que niveles intermedios de voltaje generen consumo estático 
    // oscilante en los transistores de entrada del microcontrolador.
    DIDR0 |= (1 << IR0_ADC_CHANNEL) | (1 << IR1_ADC_CHANNEL) | 
             (1 << IR2_ADC_CHANNEL) | (1 << IR3_ADC_CHANNEL);

    // ==========================================
    // Cinta Transportadora
    // ==========================================
    HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN); 
    HAL_GPIO_SET_OUTPUT(CINTA_DDR, CINTA_PIN);
}

//DAR VUELTA POR DIOS
void GPIO_SetCinta(bool estado) {
    if (estado) {
        // TRUE = Encender -> Ponemos el pin en LOW (Lógica Negativa)
        HAL_GPIO_WRITE_HIGH(CINTA_PORT, CINTA_PIN);
    } else {
        // FALSE = Apagar -> Ponemos el pin en HIGH
        HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
    }
}

bool GPIO_LeerSensor(uint8_t sensor_id) {
    // Si el hardware lee un valor mayor a 0, retorna true (ALTO)
    switch(sensor_id) {
        case 0: return HAL_GPIO_READ(IR0_PIN_REG, IR0_PIN) > 0;
        case 1: return HAL_GPIO_READ(IR1_PIN_REG, IR1_PIN) > 0;
        case 2: return HAL_GPIO_READ(IR2_PIN_REG, IR2_PIN) > 0;
        case 3: return HAL_GPIO_READ(IR3_PIN_REG, IR3_PIN) > 0;
        default: return true; // Fail-Safe (asumiendo Pull-Up para sensores inexistentes)
    }
}

void GPIO_ToggleHeartbeat(void) {
    HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
}