/**
 * @file hardware.h
 * @brief Configuración de hardware y mapeo de pines para el sistema clasificador.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 *
 * @details 
 * Define la frecuencia del reloj, puertos, registros de dirección y pines 
 * del ATmega328P correspondientes a la distribución física del proyecto.
 * Este archivo actúa como el nexo entre el hardware físico (Board Support Package) 
 * y la Capa de Abstracción de Hardware (HAL), centralizando las conexiones 
 * físicas para facilitar el mantenimiento y futuras migraciones de plataforma.
 */

#ifndef CONFIG_HARDWARE_H_
#define CONFIG_HARDWARE_H_

#include <avr/io.h>

/**
 * @defgroup Interrupt_Config Control de Interrupciones Globales
 * @brief Abstracción para el manejo del bit de interrupción global (I-bit).
 * @{
 */
#include <avr/interrupt.h>

/** @brief Frecuencia principal del CPU configurada a 16 MHz. */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/** @brief Habilita las interrupciones globales de hardware mediante instrucción SEI. */
#define HAL_ENABLE_INTERRUPTS()  sei()

/** @brief Deshabilita las interrupciones globales de hardware mediante instrucción CLI. */
#define HAL_DISABLE_INTERRUPTS() cli()
/** @} */

/**
 * @defgroup Ultrasonic_Config Sensor Ultrasónico HC-SR04
 * @brief Configuración de pines para el módulo de medición de distancia.
 * @{
 */
#define TRIGGER_DDR      DDRB    /**< Registro de dirección para el pin Trigger */
#define TRIGGER_PORT     PORTB   /**< Puerto de salida de datos para el Trigger */
#define TRIGGER_PIN      PB1     /**< Pin físico del Trigger (equivalente a Arduino D9) */

#define ECHO_DDR         DDRB    /**< Registro de dirección para el pin Echo */
#define ECHO_PORT        PORTB   /**< Puerto de configuración Pull-Up para el Echo */
#define ECHO_PIN_REG     PINB    /**< Registro de lectura de estado físico para el Echo */
#define ECHO_PIN         PB2     /**< Pin físico del Echo (equivalente a Arduino D10) */
/** @} */

/**
 * @defgroup Servo_Config Servomotores SG90
 * @brief Configuración de pines para los actuadores mecánicos del clasificador.
 * @{
 */
#define SERVO1_DDR       DDRD    /**< Registro de dirección para el Servo 1 */
#define SERVO1_PORT      PORTD   /**< Puerto de salida para el pulso PWM del Servo 1 */
#define SERVO1_PIN       PD7     /**< Pin físico del Servo 1 (equivalente a Arduino D7) */

#define SERVO2_DDR       DDRB    /**< Registro de dirección para el Servo 2 */
#define SERVO2_PORT      PORTB   /**< Puerto de salida para el pulso PWM del Servo 2 */
#define SERVO2_PIN       PB3     /**< Pin físico del Servo 2 (equivalente a Arduino D11) */

#define SERVO3_DDR       DDRB    /**< Registro de dirección para el Servo 3 */
#define SERVO3_PORT      PORTB   /**< Puerto de salida para el pulso PWM del Servo 3 */
#define SERVO3_PIN       PB4     /**< Pin físico del Servo 3 (equivalente a Arduino D12) */
/** @} */

/**
 * @defgroup IR_Config Sensores Infrarrojos TCRT5000 (Modo Digital)
 * @brief Configuración del puerto D para la lectura lógica de los sensores ópticos.
 * @{
 */
#define IR0_DDR          DDRD    /**< Registro de dirección para el sensor IR0 */
#define IR0_PORT         PORTD   /**< Puerto de salida/Pull-Up para IR0 */
#define IR0_PIN_REG      PIND    /**< Registro de entrada física para IR0 */
#define IR0_PIN          PD5     /**< Pin físico para IR0 (equivalente a Arduino D5) */

#define IR1_DDR          DDRD    /**< Registro de dirección para el sensor IR1 */
#define IR1_PORT         PORTD   /**< Puerto de salida/Pull-Up para IR1 */
#define IR1_PIN_REG      PIND    /**< Registro de entrada física para IR1 */
#define IR1_PIN          PD2     /**< Pin físico para IR1 (equivalente a Arduino D2) */

#define IR2_DDR          DDRD    /**< Registro de dirección para el sensor IR2 */
#define IR2_PORT         PORTD   /**< Puerto de salida/Pull-Up para IR2 */
#define IR2_PIN_REG      PIND    /**< Registro de entrada física para IR2 */
#define IR2_PIN          PD3     /**< Pin físico para IR2 (equivalente a Arduino D3) */

#define IR3_DDR          DDRD    /**< Registro de dirección para el sensor IR3 */
#define IR3_PORT         PORTD   /**< Puerto de salida/Pull-Up para IR3 */
#define IR3_PIN_REG      PIND    /**< Registro de entrada física para IR3 */
#define IR3_PIN          PD4     /**< Pin físico para IR3 (equivalente a Arduino D4) */
/** @} */

/**
 * @defgroup IR_ADC_Channels Canales ADC para TCRT5000 (Modo Analógico)
 * @brief Mapeo del multiplexor analógico para lectura de reflectancia en bruto.
 * @{
 */
#define IR0_ADC_CHANNEL  5       /**< Canal multiplexor ADC para evaluar IR0 (A5) */
#define IR1_ADC_CHANNEL  2       /**< Canal multiplexor ADC para evaluar IR1 (A2) */
#define IR2_ADC_CHANNEL  3       /**< Canal multiplexor ADC para evaluar IR2 (A3) */
#define IR3_ADC_CHANNEL  4       /**< Canal multiplexor ADC para evaluar IR3 (A4) */
/** @} */

/**
 * @defgroup System_Config Periféricos de Estado y Control
 * @brief Hardware general para el control de la planta y telemetría visual.
 * @{
 */
#define STATUS_LED_DDR   DDRB    /**< Registro de dirección para el LED de diagnóstico */
#define STATUS_LED_PORT  PORTB   /**< Puerto de salida para el LED de diagnóstico */
#define STATUS_LED_PIN   PB5     /**< Pin físico del LED (equivalente a Arduino D13) */

#define CINTA_DDR        DDRC    /**< Registro de dirección para el relé/transistor de la cinta */
#define CINTA_PORT       PORTC   /**< Puerto de salida de potencia para la cinta */
#define CINTA_PIN        PC0     /**< Pin físico de control de la cinta (equivalente a Arduino A0) */
/** @} */

#endif // CONFIG_HARDWARE_H_