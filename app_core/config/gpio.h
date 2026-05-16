/**
 * @file gpio.h
 * @brief Definiciones y prototipos para la inicialización de GPIO.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 *
 * @details Este archivo actúa como la interfaz de configuración de hardware
 * para el proyecto, permitiendo que la aplicación principal configure todos
 * los pines de la placa con una sola llamada a función.
 */

#ifndef CONFIG_GPIO_H_
#define CONFIG_GPIO_H_

/**
 * @brief Configura la dirección y el estado inicial de todos los pines del sistema.
 * * @details Utiliza las macros de hal_gpio.h y las definiciones de hardware.h 
 * para establecer los estados de los sensores ultrasónicos, infrarrojos, 
 * servomotores y actuadores de la cinta.
 * * @note Esta función debe ser llamada al inicio del main(), antes de 
 * inicializar cualquier otro periférico que dependa de los pines (como UART o Timers).
 */
void GPIO_Init(void);

#endif /* CONFIG_GPIO_H_ */