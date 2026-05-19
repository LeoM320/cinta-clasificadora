/**
 * @file hcsr04.h
 * @brief Driver asíncrono para sensor ultrasónico HC-SR04 mediante FSM.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 *
 * @details
 * Proporciona una interfaz no bloqueante para la medición de distancias.
 * A diferencia de los drivers convencionales, este módulo no detiene la CPU 
 * esperando el retorno del eco. Utiliza una Máquina de Estados Finitos (FSM) 
 * controlada por microsegundos, permitiendo que el microcontrolador atienda 
 * otras tareas críticas (como multiplexado de displays, comunicaciones o PID) 
 * mientras el sonido viaja por el aire.
 */
#ifndef DRIVERS_HCSR04_H_
#define DRIVERS_HCSR04_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el estado lógico del driver y los pines de control.
 * 
 * @note Asegura que el pin de Trigger comience en estado lógico BAJO para 
 *       prevenir disparos accidentales al arrancar el sistema.
 */
void HCSR04_Init(void);

/**
 * @brief Solicita el inicio de una nueva medición ultrasónica.
 * 
 * Inyecta el pulso inicial de 10 microsegundos en la FSM si el sensor 
 * se encuentra en estado de reposo (IDLE).
 * 
 * @return true  Si la solicitud fue aceptada y el pulso comenzó a emitirse.
 * @return false Si el sensor está ocupado resolviendo una medición previa.
 */
bool HCSR04_Trigger(void);

/**
 * @brief Máquina de estados principal del sensor ultrasónico.
 * 
 * @warning Esta función es el motor del driver y debe ser despachada 
 *          continuamente dentro del Super Loop o en un RTOS con alta frecuencia 
 *          para garantizar la precisión en la lectura de microsegundos del eco.
 */
void HCSR04_Task(void);

/**
 * @brief Extrae la última distancia calculada de la memoria del driver.
 * 
 * @note La llamada a esta función limpia automáticamente la bandera interna de 
 *       "dato nuevo" (`data_ready = false`).
 * 
 * @return uint16_t Distancia medida en centímetros (cm).
 */
uint16_t HCSR04_GetDistance(void);

/**
 * @brief Consulta si la FSM ha finalizado exitosamente una nueva conversión.
 * 
 * @return true  Hay un dato fresco listo para ser consumido.
 * @return false No hay datos nuevos desde la última lectura.
 */
bool HCSR04_IsDataReady(void);

#endif /* DRIVERS_HCSR04_H_ */