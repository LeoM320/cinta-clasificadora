/**
 * @file hcsr04.h
 * @brief Driver asíncrono para sensor ultrasónico HC-SR04 mediante FSM.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */
#ifndef DRIVERS_HCSR04_H_
#define DRIVERS_HCSR04_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el estado del driver ultrasónico y asegura el pin de trigger en BAJO.
 */
void HCSR04_Init(void);

/**
 * @brief Dispara una nueva medición de distancia si el sensor está inactivo.
 * @return true si la solicitud fue aceptada, false si ya hay una lectura en curso.
 */
bool HCSR04_Trigger(void);

/**
 * @brief Máquina de estados del sensor. Debe llamarse continuamente en el main loop.
 */
void HCSR04_Task(void);

/**
 * @brief Obtiene la última distancia calculada.
 * @return Distancia en centímetros.
 */
uint16_t HCSR04_GetDistance(void);

/**
 * @brief Indica si hay una nueva medición válida disponible.
 * @return true si el dato es fresco (la bandera se limpia al leer la distancia).
 */
bool HCSR04_IsDataReady(void);

#endif /* DRIVERS_HCSR04_H_ */