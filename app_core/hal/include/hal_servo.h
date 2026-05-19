// ==========================================
// hal/include/hal_servo.h
// ==========================================

/**
 * @file hal_servo.h
 * @brief Controlador seguro de Servomotores por interrupciones con perfil de velocidad.
 * @author LeoM320
 *
 * @details
 * Implementa un generador de señales PWM secuencial utilizando el Timer1.
 * Soporta saltos instantáneos o barridos suaves (Smooth Sweeps) interpolando 
 * los anchos de pulso en cada ciclo de 20ms, previniendo el estrés mecánico.
 */

#ifndef HAL_SERVO_H_
#define HAL_SERVO_H_

#include <stdint.h>
#include <stdbool.h>

#define SERVO_1 0
#define SERVO_2 1
#define SERVO_3 2

void HAL_Servo_Init(void);

/**
 * @brief Salto instantáneo: Establece el ancho de pulso directamente.
 * @param servo_id ID del servo.
 * @param pulse_us Ancho de pulso en microsegundos (500 a 2400).
 */
void HAL_Servo_SetPulse(uint8_t servo_id, uint16_t pulse_us);

/**
 * @brief Salto instantáneo: Establece el ángulo máximo a la máxima velocidad mecánica.
 * @param servo_id ID del servo.
 * @param angle Ángulo destino en grados (0 a 180).
 */
void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle);

/**
 * @brief Transición suave: Interpola el movimiento hacia el ángulo objetivo.
 * 
 * @details
 * Calcula dinámicamente un incremento/decremento que se aplicará en la ISR 
 * cada 20 milisegundos, logrando que el eje rote de manera fluida hasta el 
 * punto objetivo en el tiempo solicitado.
 * 
 * @param servo_id ID del servo.
 * @param target_angle Ángulo destino en grados (0 a 180).
 * @param transition_time_ms Tiempo deseado para completar el recorrido en milisegundos.
 */
void HAL_Servo_SetAngleSmooth(uint8_t servo_id, uint8_t target_angle, uint16_t transition_time_ms);

void HAL_Servo_Enable(uint8_t servo_id);
void HAL_Servo_Disable(uint8_t servo_id);

#endif /* HAL_SERVO_H_ */