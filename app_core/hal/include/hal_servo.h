/**
 * @file hal_servo.h
 * @brief Controlador seguro de Servomotores por interrupciones (Software PWM secuencial).
 * @author LeoM320
 * @date 15/05/2026
 *
 * @details
 * Implementa un generador de señales PWM secuencial utilizando un único 
 * temporizador de hardware de 16 bits (Timer1). Este enfoque multiplexado en el 
 * tiempo permite controlar múltiples servomotores utilizando pines de propósito general (GPIO), 
 * superando la restricción de canales PWM físicos del hardware.
 */

#ifndef HAL_SERVO_H_
#define HAL_SERVO_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Identificador lógico para el servo 1. */
#define SERVO_1 0
/** @brief Identificador lógico para el servo 2. */
#define SERVO_2 1
/** @brief Identificador lógico para el servo 3. */
#define SERVO_3 2

/**
 * @brief Inicializa el hardware del Timer1 y los pines de salida para los servos.
 * 
 * @note Asume una configuración de reloj (F_CPU) de 16 MHz. Configura el temporizador 
 *       con un prescaler de 8, otorgando una resolución de 0.5 microsegundos por tick.
 */
void HAL_Servo_Init(void);

/**
 * @brief Configura el ancho del pulso PWM para controlar la posición exacta.
 * 
 * @param[in] servo_id Identificador del servo destino (SERVO_1, SERVO_2 o SERVO_3).
 * @param[in] pulse_us Ancho del pulso en microsegundos (usualmente entre 500 y 2400).
 */
void HAL_Servo_SetPulse(uint8_t servo_id, uint16_t pulse_us);

/**
 * @brief Establece la posición del servo especificando un ángulo.
 * 
 * Es una función de conveniencia que interpola el ángulo solicitado a su 
 * correspondiente ancho de pulso en microsegundos.
 * 
 * @param[in] servo_id Identificador del servo destino.
 * @param[in] angle    Ángulo deseado en grados (0 a 180).
 */
void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle);

/**
 * @brief Activa la transmisión de la señal PWM hacia el servo.
 * 
 * @param[in] servo_id Identificador del servo a habilitar.
 */
void HAL_Servo_Enable(uint8_t servo_id);

/**
 * @brief Interrumpe la señal PWM y fuerza el pin del servo a estado lógico BAJO.
 * 
 * @details Cortar los pulsos relaja el motor internamente (deja de hacer fuerza 
 *          activa para mantener la posición), lo cual es útil para ahorrar 
 *          energía térmica y eléctrica si la carga mecánica es estable.
 * 
 * @param[in] servo_id Identificador del servo a deshabilitar.
 */
void HAL_Servo_Disable(uint8_t servo_id);

#endif /* HAL_SERVO_H_ */