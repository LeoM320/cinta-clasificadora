# cinta-clasificadora
Firmware en C (ATMega328P) y HMI en Qt para un sistema automatizado de clasificación de paquetes por altura. Trabajo Práctico Nº 4 - Microcontroladores, UNER.

## Descripción del Proyecto
El presente repositorio contiene el diseño, desarrollo e implementación del firmware y la interfaz gráfica de usuario (GUI) para un sistema de clasificación de objetos en una cinta transportadora. El dispositivo está diseñado para clasificar tres tipos de prismas rectangulares (cajas) según su altura: pequeña (6 cm), mediana (8 cm) y grande (10 cm).

## Arquitectura de Hardware
El hardware del proyecto está compuesto por los siguientes componentes:
* **Microcontrolador:** ATMega328P de 8 bits.
* **Unidad de Medición:** Sensor ultrasónico HC-SR04 posicionado cenitalmente para determinar la altura de las cajas por diferencia de distancia.
* **Detección de Flujo:** Cuatro sensores infrarrojos de reflexión TCRT5000. El sensor $S_{0}$ actúa como trigger para detectar la entrada a la zona de medición. Los sensores $S_{1}$, $S_{2}$ y $S_{3}$ detectan la posición de salida para activar los actuadores.
* **Actuadores:** Tres servomotores SG90 equipados con palancas de eyección mecánica.

## Requerimientos del Firmware
El firmware está desarrollado en lenguaje C bajo estrictas normas de modularidad y tiempo real:
* Utiliza librerías agnósticas al hardware y no bloqueantes, prohibiendo el uso de la función `_delay_ms`.
* Implementa la lectura del sensor HC-SR04 mediante el manejo de disparo (trigger) y captura de eco (input capture).
* Genera señales PWM con resolución adecuada para el control posicional de los servomotores SG90.
* Gestiona la comunicación bidireccional con la interfaz de usuario a través del periférico UART.
* Soporta un **Modo Normal (Feedback)** donde la eyección depende de los sensores de salida, y un **Modo Estimado (Open Loop)** que calcula el tiempo de arribo por temporizadores ante una falla en los sensores.

## Interfaz de Usuario (HMI)
La aplicación de escritorio está desarrollada utilizando el framework Qt y cumple con las siguientes características:
* Implementa el protocolo UNER para la comunicación entre la máquina y la computadora.
* Visualiza en tiempo real el estado de la cinta, los sensores activos y el conteo de cajas clasificadas.
* Incluye un panel de configuración que permite asignar qué altura de caja corresponde a cada salida y ajustar la calibración de umbrales.
* Cuenta con una restricción de seguridad que bloquea la reconfiguración del sistema mientras existan procesos de transporte activos.

## Pinout 
A continuación se detalla la asignación de pines utilizada en el microcontrolador ATMega328P y su equivalencia en la placa Arduino:

| Descripción | Pin Arduino | Pin ATMega328P |
| :--- | :--- | :--- |
| TRIGGER | 9 | PB1 |
| ECHO | 10 | PB2 |
| SERVO 1 | 7 | PD7 |
| SERVO 2 | 11 | PB4 |
| SERVO 3 | 12 | PB3 |
| IR 0 (HSCR04) - $S_{0}$ | 5 | PD2 |
| IR 1 - $S_{1}$ | 2 | PD3 |
| IR 2 - $S_{2}$ | 3 | PD4 |
| IR 3 - $S_{3}$ | 4 | PD5 |
