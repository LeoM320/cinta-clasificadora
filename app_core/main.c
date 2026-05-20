#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/include/hal_timer.h"
#include "hal/include/hal_uart.h"

// Apuntamos al nuevo módulo incremental paso a paso
#include "app_cinta/app_cinta_final.h"

#include "utils/uner_protocol.h"
#include "app/comandos.h"

// Variable global del protocolo para el manejo de tramas
UnerProtocol_t protocolo_uart; 

/**
 * @brief Callback de error para reaccionar a alarmas de la cinta.
 * @details Inyectamos esta función en el módulo de la cinta para mantener
 * separada la lógica de control de la lógica de comunicación.
 */
void ManejarErrorCinta(uint8_t codigo_error) {
    Uner_AbrirCarga(&protocolo_uart, 1);
    Uner_Agregar8(&protocolo_uart, codigo_error);
    Uner_CerrarCarga(&protocolo_uart);
    Uner_Transmitir(&protocolo_uart);
}

int main(void)
{
    // 1. SECCIÓN CRÍTICA: Desactivar interrupciones durante la inicialización
    HAL_DISABLE_INTERRUPTS();

    // 2. Inicialización Eléctrica y del Sistema de Baja Capa
    GPIO_Init();
    HAL_Timer0_Init();      // Inicializa el Systick de 1ms
    HAL_UART_Init(115200);  // Configura la UART a la velocidad de la HMI en Qt

    // 3. Inicialización del Módulo de Control Final
    App_CintaFinal_Init();

    // 4. Inyección del Callback de Errores (Arquitectura limpia)
    App_CintaFinal_SetErrorCallback(ManejarErrorCinta); 

    // 5. Inicialización del Buffer de Protocolo UNER
    Uner_Init(&protocolo_uart);

    // 6. Fin de Sección Crítica: Habilitamos interrupciones globales
    HAL_ENABLE_INTERRUPTS();

    // 7. SUPER LOOP ASÍNCRONO NO BLOQUEANTE
    while (1)
    {
        // Extraer bytes de la UART "al vuelo" y alimentar la FSM del protocolo
        Uner_Recibir(&protocolo_uart, HAL_GetMillis());
        
        // Procesar tramas listas y ejecutar comandos (ej: CMD_SET_BELT)
        Comandos_Procesar(&protocolo_uart);
        
        // MANTENER VIVO EL DRIVER ULTRASÓNICO (Faltaba esto)
        HCSR04_Task(); 
        
        // Ejecutar nuestra máquina de estados incremental de la cinta
        App_CintaFinal_Task();
    }

    return 0;
}