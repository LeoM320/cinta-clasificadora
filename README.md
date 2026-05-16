# cinta-clasificadora
Firmware en C (ATMega328P) y HMI en C++ nativo con Qt para un sistema automatizado de clasificación de paquetes por altura. Trabajo Práctico Nº 4 - Microcontroladores, Universidad Nacional de Entre Ríos (UNER).

## Descripción del Proyecto
El presente repositorio contiene el diseño, desarrollo e implementación del firmware y la interfaz gráfica de usuario (GUI) para un sistema de clasificación de objetos en una cinta transportadora. El dispositivo está diseñado para clasificar tres tipos de prismas rectangulares (cajas) según su altura: pequeña (6 cm), mediana (8 cm) y grande (10 cm).

## Arquitectura de Hardware
El hardware del proyecto está compuesto por los siguientes componentes:
* **Microcontrolador:** ATMega328P de 8 bits.
* **Unidad de Medición:** Sensor ultrasónico HC-SR04 posicionado cenitalmente para determinar la altura de las cajas por diferencia de distancia.
* **Detección de Flujo:** Cuatro sensores infrarrojos de reflexión TCRT5000. El sensor **S0** actúa como trigger para detectar la entrada a la zona de medición. Los sensores **S1**, **S2** y **S3** detectan la posición de salida para activar los actuadores. Soporta tanto lectura digital directa como acondicionamiento analógico por canales ADC independientes.
* **Actuadores:** Tres servomotores SG90 equipados con palancas de eyección mecánica y una etapa de potencia para el motor de la cinta transportadora.
* **Interfaz de Estado:** LED testigo integrado para debug y señalización de estados del sistema.

### Pinout Oficial del Proyecto
A continuación se detalla la asignación de pines real utilizada en el microcontrolador ATMega328P y su equivalencia física en la distribución de la placa:

| Descripción | Componente / Rol | Pin Arduino | Pin ATMega328P | Registro Digital | Canal ADC |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TRIGGER** | HC-SR04 (Disparo) | D9 | PB1 | PORTB1 | - |
| **ECHO** | HC-SR04 (Eco) | D10 | PB2 | PINB2 | - |
| **SERVO 1** | Eyector Caja Pequeña | D7 | PD7 | PORTD7 | - |
| **SERVO 2** | Eyector Caja Mediana | D11 | PB3 | PORTB3 | - |
| **SERVO 3** | Eyector Caja Grande | D12 | PB4 | PORTB4 | - |
| **IR 0 (S0)** | Trigger de Entrada | D5 | PD5 | PIND5 | ADC5 |
| **IR 1 (S1)** | Sensor de Salida 1 | D2 | PD2 | PIND2 | ADC2 |
| **IR 2 (S2)** | Sensor de Salida 2 | D3 | PD3 | PIND3 | ADC3 |
| **IR 3 (S3)** | Sensor de Salida 3 | D4 | PD4 | PIND4 | ADC4 |
| **STATUS LED** | LED de Diagnóstico | D13 | PB5 | PORTB5 | - |
| **CINTA** | Control de Motor | A0 | PC0 | PORTC0 | - |

## Arquitectura de Software (Firmware)
El código está diseñado bajo un patrón de capas fuertemente desacopladas, garantizando el agnosticismo de hardware (Hardware-Agnostic) y permitiendo su portabilidad a plataformas de 32 bits (ej. STM32) sin alterar la lógica de aplicación:

* **`config/` (BSP - Board Support Package):** Configuración centralizada de la placa. Define el mapeo definitivo de pines, registros de dirección (`DDR`), puertos (`PORT`/`PIN`) y la frecuencia de reloj (`F_CPU = 16MHz`), actuando como la única fuente de verdad del hardware.
* **`hal/` (Hardware Abstraction Layer):** Controladores de bajo nivel que interactúan con los registros del silicio (UART, Timers, PWM, Input Capture).
* **`utils/` (Middleware):** Lógica pura independiente del hardware, incluyendo la gestión de temporizadores de software y el motor del protocolo.

## Requerimientos y Optimizaciones del Firmware
El firmware está desarrollado en lenguaje C bajo estrictas normas de tiempo real y modularidad:
* **Procesamiento Asíncrono No Bloqueante:** Utiliza librerías temporales de software ancladas a una base de milisegundos por hardware (`HAL_GetMillis()`), prohibiendo el uso de `_delay_ms` para garantizar la ejecución fluida de la Máquina de Estados Finitos (FSM).
* **Control de Sensores y Actuadores:** Lectura del HC-SR04 mediante captura de entrada (Input Capture) para máxima precisión temporal y generación de señales PWM para el posicionamiento angular estricto de los servos SG90.
* **Modos de Operación:** * *Modo Normal (Feedback):* La eyección depende de la lectura en tiempo real de los sensores infrarrojos de salida (**S1**, **S2**, **S3**).
  * *Modo Estimado (Open Loop):* Calcula probabilísticamente el tiempo de arribo físico mediante temporizadores internos de software ante una falla o bloqueo en los sensores infrarrojos.
* **Recepción UART Lock-Free:** Implementa un patrón Productor-Consumidor con un Ring Buffer en la interrupción RX, evitando la pérdida de bytes a altas velocidades de transmisión (115200 baudios).

## Interfaz de Usuario (HMI) y Comunicaciones
La aplicación de escritorio está desarrollada utilizando el framework Qt (C++ nativo) y se comunica con la placa a través del **Protocolo UNER**:

* **Zero-Copy Parsing:** El microcontrolador decodifica las tramas entrantes "al vuelo" directamente desde el buffer circular del periférico de hardware, eliminando buffers de software intermedios en la capa de aplicación y reduciendo drásticamente el consumo de memoria RAM.
* **Seguridad de Trama:** Validación estricta de integridad mediante compuerta XOR (Checksum) y empaquetado estructurado Little Endian nativo para enteros de 16 y 32 bits mediante operaciones de máscara.
* **Monitoreo y Control:** Visualización dinámica y gráfica del estado de la cinta, sensores activos y conteo estadístico de producción por tipo de caja.
* **Seguridad Activa:** Restricción lógica por software que bloquea de forma segura cualquier reconfiguración del sistema (asignación de alturas a salidas o calibración de umbrales métricos) mientras existan procesos de transporte o cajas activas sobre la cinta transportadora.
