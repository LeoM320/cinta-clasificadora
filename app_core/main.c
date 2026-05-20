#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/include/hal_timer.h"
#include "hal/include/hal_uart.h"
#include "app_cinta/app_cinta.h"

#include "utils/uner_protocol.h"
#include "app/comandos.h"

// Variable global del protocolo para que el callback la pueda usar
UnerProtocol_t protocolo_uart; 

// Esta es la función que reacciona al grito de la cinta
void ManejarErrorCinta(uint8_t codigo_error) {
    // Usamos la API pública de UNER para enviar un paquete asíncrono
    Uner_AbrirCarga(&protocolo_uart, 1);
    Uner_Agregar8(&protocolo_uart, codigo_error);
    Uner_CerrarCarga(&protocolo_uart);
    
    // Lo despachamos al hardware
    Uner_Transmitir(&protocolo_uart);
}

int main(void)
{
    HAL_DISABLE_INTERRUPTS();
    GPIO_Init();
    HAL_Timer0_Init();
    HAL_UART_Init(115200); // <-- Baudios corregidos
    App_Cinta_Init();

    // Vinculamos (Inyectamos) la función al sistema de la cinta
    App_Cinta_SetErrorCallback(ManejarErrorCinta); 

    Uner_Init(&protocolo_uart);
    HAL_ENABLE_INTERRUPTS();

    while (1)
    {
        // 2. Extraer bytes de la UART "al vuelo" (Zero-Copy)
        Uner_Recibir(&protocolo_uart, HAL_GetMillis());
        
        // 3. Procesar las tramas listas y ejecutar acciones de hardware
        Comandos_Procesar(&protocolo_uart);
        
        // 4. Ejecutar la FSM de la cinta y los subsistemas asíncronos
        App_Cinta_Task();
    }

    return 0;
}