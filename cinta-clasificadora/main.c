#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/include/hal_uart.h"

int main(void)
{
    GPIO_Init();
    
    // Inicializar UART a 115200 baudios (¡Asegurate de poner la terminal de la PC a esta misma velocidad!)
    HAL_UART_Init(115200);

    sei(); // ¡Vital para que funcione la recepción RX!

    // Mensaje de bienvenida
    HAL_UART_TxString("Sistema de Cinta Clasificadora Iniciado.\r\n");
    HAL_UART_TxString("Esperando comandos...\r\n");

    while(1)
    {
        // Revisar si llegó algo al buffer de recepción de forma asíncrona
        if (HAL_UART_RxDataAvailable())
        {
            uint8_t byte_recibido = HAL_UART_RxRead();
            
            // Hacer un simple Eco: devolver lo que recibimos
            HAL_UART_TxString("Recibi: ");
            HAL_UART_TxByte(byte_recibido);
            HAL_UART_TxString("\r\n");
            
            // Acá podrías agregar un simple parser:
            // if (byte_recibido == 'a') { HAL_Servo_SetAngle(SERVO_1, 90); }
        }
        
        // Tus servos, ADC, y temporizadores siguen funcionando perfectamente acá...
    }
}