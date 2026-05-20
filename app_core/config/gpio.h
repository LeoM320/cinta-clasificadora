/**
 * @file gpio.h
 * @brief Interfaz principal para la inicialización y ruteo físico de la placa (GPIO).
 * @author LeoM320
 * @date 14 de Mayo de 2026
 *
 * @details 
 * Este archivo actúa como el orquestador de la configuración de hardware.
 * Permite que la aplicación principal prepare todos los pines del microcontrolador 
 * (entradas, salidas y estados de reposo) mediante una única invocación, 
 * abstrayendo la complejidad de los registros DDRx y PORTx.
 */

#include <stdbool.h>
#include <stdint.h>

#ifndef CONFIG_GPIO_H_
#define CONFIG_GPIO_H_

/**
 * @brief Configura la dirección y el estado eléctrico inicial de todos los pines.
 * 
 * @details 
 * Aplica la configuración definida en `hardware.h` para establecer los estados 
 * seguros (Fail-Safe) de los actuadores (cinta y servos) y prepara las entradas 
 * de los sensores (ultrasónico e infrarrojos).
 * 
 * @warning Esta función debe ser la primera en ejecutarse dentro de `main()`, 
 *          estrictamente antes de habilitar las interrupciones globales (`sei()`) 
 *          y antes de inicializar periféricos complejos como UART o Timers.
 */
void GPIO_Init(void);

// Nuevas abstracciones de hardware
void GPIO_SetCinta(bool estado);
bool GPIO_LeerSensor(uint8_t sensor_id);
void GPIO_ToggleHeartbeat(void);

#endif /* CONFIG_GPIO_H_ */