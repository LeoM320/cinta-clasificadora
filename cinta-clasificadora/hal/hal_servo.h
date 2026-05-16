/**
 * @file hal_servo.h
 * @brief Controlador seguro de Servomotores por interrupciones.
 * @author LeoM320
 * @date 15/05/2026
 */

#ifndef HAL_SERVO_H_
#define HAL_SERVO_H_

#include <stdint.h>
#include <stdbool.h>

#define SERVO_1 0
#define SERVO_2 1
#define SERVO_3 2

/**
 * @brief Inicializa el Timer1 para el control secuencial de servos.
 */
void HAL_Servo_Init(void);

/**
 * @brief Establece la posición del servo especificando el ancho de pulso.
 * @param servo_id ID del servo (SERVO_1, SERVO_2, SERVO_3).
 * @param pulse_us Ancho de pulso en microsegundos (600 a 2400).
 */
void HAL_Servo_SetPulse(uint8_t servo_id, uint16_t pulse_us);

/**
 * @brief Establece el ángulo del servo (0 a 180 grados).
 * @param servo_id ID del servo (SERVO_1, SERVO_2, SERVO_3).
 * @param angle Ángulo en grados (0 a 180).
 */
void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle);

/**
 * @brief Habilita el envío de pulsos PWM a un servo específico.
 * @param servo_id ID del servo a activar.
 */
void HAL_Servo_Enable(uint8_t servo_id);

/**
 * @brief Deshabilita de forma segura el servo, forzando el pin a nivel BAJO.
 * @param servo_id ID del servo a desactivar.
 */
void HAL_Servo_Disable(uint8_t servo_id);

#endif /* HAL_SERVO_H_ */