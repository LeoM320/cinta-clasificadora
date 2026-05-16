#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "hal/include/hal_uart.h"
#include "hal/include/hal_timer.h"
#include "utils/temporizador.h"
#include "utils/uner_protocol.h"

UnerProtocol_t pc_com;
Temporizador timer_envio; 

int main(void) {
    HAL_UART_Init(115200);
    HAL_Timer0_Init(); 
    Uner_Init(&pc_com);
    Temp_IniciarMS(&timer_envio, 1000);
    sei(); 

    uint16_t contador_prueba = 0;

    while(1) {
        uint32_t ahora = HAL_GetMillis();
        
        // 1. Escuchar el puerto serie y procesar los bytes entrantes
        Uner_Recibir(&pc_com, ahora);

        // 2. ¿Llegó un paquete completo y válido desde la PC?
        if (Uner_Comando(&pc_com)) {
            uint8_t id = Uner_IDComando(&pc_com);
            
            // Si el ID es 0xBB (Comando de prueba RX)
            if (id == 0xBB) {
                // Obtenemos el entero de 16 bits (arranca en la posición 1 del payload)
                uint16_t valor_recibido = Uner_Obtener16(&pc_com, 1);
                
                // Respuesta inmediata: Multiplicamos por 2 y devolvemos con ID 0xCC
                Uner_AbrirCarga(&pc_com, 3); // 1 byte ID + 2 bytes del uint16
                Uner_Agregar8(&pc_com, 0xCC);
                Uner_Agregar16(&pc_com, valor_recibido * 2);
                Uner_CerrarCarga(&pc_com);
            }
        }

        // 3. Envío automático de telemetría (El que armamos antes)
        if (Temp_Expiro(&timer_envio)) {
            Temp_Reiniciar(&timer_envio);
            Uner_AbrirCarga(&pc_com, 7);
            Uner_Agregar8(&pc_com, 0xAA);
            Uner_Agregar16(&pc_com, contador_prueba);
            Uner_Agregar32(&pc_com, 0xDEADBEEF);
            Uner_CerrarCarga(&pc_com);
            contador_prueba++; 
        }

        // 4. Despachar bytes de salida
        Uner_Transmitir(&pc_com);
    }
    
    return 0;
}