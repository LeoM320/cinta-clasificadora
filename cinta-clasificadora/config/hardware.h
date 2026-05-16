/**
 * @file hardware.h
 * @brief Configuración de hardware y mapeo de pines para el sistema clasificador.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 * * @details Define la frecuencia del reloj, puertos, registros de dirección y pines 
 * del ATmega328P correspondientes a la distribución física del proyecto.
 * Este archivo actúa como el nexo entre el hardware real y la capa HAL.
 */

#ifndef CONFIG_HARDWARE_H_
#define CONFIG_HARDWARE_H_

#include <avr/io.h>

/** * @brief Frecuencia del CPU (16 MHz) 
 */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/**
 * @defgroup Ultrasonic_Config Sensor Ultrasónico HC-SR04
 * @brief Configuración de pines para el sensor de distancia.
 * @{
 */
#define TRIGGER_DDR     DDRB    /**< Registro de dirección para el Trigger */
#define TRIGGER_PORT    PORTB   /**< Puerto de salida para el Trigger */
#define TRIGGER_PIN     PB1     /**< Pin físico del Trigger (Arduino D9) */

#define ECHO_DDR        DDRB    /**< Registro de dirección para el Echo */
#define ECHO_PORT       PORTB   /**< Puerto de salida para el Echo */
#define ECHO_PIN_REG    PINB    /**< Registro de lectura para el Echo */
#define ECHO_PIN        PB2     /**< Pin físico del Echo (Arduino D10) */
/** @} */

/**
 * @defgroup Servo_Config Servomotores SG90
 * @brief Configuración de pines para los actuadores del clasificador.
 * @{
 */
#define SERVO1_DDR      DDRD    /**< Registro de dirección para el Servo 1 */
#define SERVO1_PORT     PORTD   /**< Puerto de salida para el Servo 1 */
#define SERVO1_PIN      PD7     /**< Pin físico del Servo 1 (Arduino D7) */

#define SERVO2_DDR      DDRB    /**< Registro de dirección para el Servo 2 */
#define SERVO2_PORT     PORTB   /**< Puerto de salida para el Servo 2 */
#define SERVO2_PIN      PB3     /**< Pin físico del Servo 2 (Arduino D11) */

#define SERVO3_DDR      DDRB    /**< Registro de dirección para el Servo 3 */
#define SERVO3_PORT     PORTB   /**< Puerto de salida para el Servo 3 */
#define SERVO3_PIN      PB4     /**< Pin físico del Servo 3 (Arduino D12) */
/** @} */

/**
 * @defgroup IR_Config Sensores Infrarrojos TCRT5000 (Modo Digital)
 * @brief Configuración del puerto D para la lectura digital de los sensores IR.
 * @{
 */
#define IR0_DDR         DDRD    /**< Registro de dirección para IR0 */
#define IR0_PORT        PORTD   /**< Puerto de salida para IR0 */
#define IR0_PIN_REG     PIND    /**< Registro de entrada para IR0 */
#define IR0_PIN         PD5     /**< Pin físico para IR0 (Arduino D5) */

#define IR1_DDR         DDRD    /**< Registro de dirección para IR1 */
#define IR1_PORT        PORTD   /**< Puerto de salida para IR1 */
#define IR1_PIN_REG     PIND    /**< Registro de entrada para IR1 */
#define IR1_PIN         PD2     /**< Pin físico para IR1 (Arduino D2) */

#define IR2_DDR         DDRD    /**< Registro de dirección para IR2 */
#define IR2_PORT        PORTD   /**< Puerto de salida para IR2 */
#define IR2_PIN_REG     PIND    /**< Registro de entrada para IR2 */
#define IR2_PIN         PD3     /**< Pin físico para IR2 (Arduino D3) */

#define IR3_DDR         DDRD    /**< Registro de dirección para IR3 */
#define IR3_PORT        PORTD   /**< Puerto de salida para IR3 */
#define IR3_PIN_REG     PIND    /**< Registro de entrada para IR3 */
#define IR3_PIN         PD4     /**< Pin físico para IR3 (Arduino D4) */
/** @} */

/**
 * @defgroup IR_ADC_Channels Canales ADC para TCRT5000 (Modo Analógico)
 * @brief Mapeo de canales ADC por si se requiere lectura analógica de los IR.
 * @{
 */
#define IR0_ADC_CHANNEL 5       /**< Canal ADC para IR0 */
#define IR1_ADC_CHANNEL 2       /**< Canal ADC para IR1 */
#define IR2_ADC_CHANNEL 3       /**< Canal ADC para IR2 */
#define IR3_ADC_CHANNEL 4       /**< Canal ADC para IR3 */
/** @} */

/**
 * @defgroup System_Config Periféricos de Estado y Control
 * @brief Configuración para indicadores de estado y control de potencia.
 * @{
 */
#define STATUS_LED_DDR  DDRB    /**< Registro de dirección para el LED de estado */
#define STATUS_LED_PORT PORTB   /**< Puerto de salida para el LED de estado */
#define STATUS_LED_PIN  PB5     /**< Pin físico del LED (Arduino D13) */

#define CINTA_DDR       DDRC    /**< Registro de dirección para el control de la cinta */
#define CINTA_PORT      PORTC   /**< Puerto de salida para la cinta */
#define CINTA_PIN       PC0     /**< Pin físico del control de cinta (Arduino A0) */
/** @} */

#endif // CONFIG_HARDWARE_H_