#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "hal/include/hal_uart.h"
#include "hal/include/hal_timer.h"
#include "utils/temporizador.h"
#include "utils/uner_protocol.h"

UnerProtocol_t pc_com;
Temporizador timer_heartbeat; 

// --- Variables de estado del sistema ---
uint8_t conexion_activa = 0;
uint8_t heartbeat_habilitado = 0;
uint16_t heartbeat_periodo = 1000; // Default: 1 segundo
uint16_t heartbeat_contador = 0;

int main(void) {
    // 1. Inicialización de Hardware y Protocolo
    HAL_UART_Init(115200);
    HAL_Timer0_Init(); 
    Uner_Init(&pc_com);
    Temp_IniciarMS(&timer_heartbeat, heartbeat_periodo);
    sei(); 

    while(1) {
        uint32_t ahora = HAL_GetMillis();
        
        // 2. Escuchar el puerto serie y procesar bytes entrantes
        Uner_Recibir(&pc_com, ahora);

        // 3. Máquina de estados de Comandos
        if (Uner_Comando(&pc_com)) {
            uint8_t id = Uner_IDComando(&pc_com);
            
            switch (id) {
                case 0xAB: // COMANDO: Alive / Conectar
                    conexion_activa = 1;
                    heartbeat_habilitado = 1; // Arranca por defecto al conectar
                    
                    // Enviamos ACK (Acknowledge) para que Qt confirme el Handshake
                    Uner_AbrirCarga(&pc_com, 1);
                    Uner_Agregar8(&pc_com, 0xAB);
                    Uner_CerrarCarga(&pc_com);
                    break;
                    
                case 0xAC: // COMANDO: Desconectar
                    conexion_activa = 0;
                    heartbeat_habilitado = 0;
                    // Opcional: Apagar motores o poner el sistema en modo seguro acá
                    break;
                    
                case 0xAD: // COMANDO: Configurar Heartbeat
                    // Estructura esperada desde la PC:
                    // [ID: 0xAD] [1 byte: Habilitar (1/0)] [2 bytes: Periodo ms]
                    if (conexion_activa) {
                        heartbeat_habilitado = Uner_Obtener8(&pc_com, 1);
                        heartbeat_periodo = Uner_Obtener16(&pc_com, 2);
                        
                        // Reiniciamos el timer con la nueva frecuencia
                        Temp_IniciarMS(&timer_heartbeat, heartbeat_periodo);
                    }
                    break;
            }
        }

        // 4. Envío automático de Heartbeat / Telemetría
        // Solo transmite si la conexión se validó y el envío está habilitado
        if (conexion_activa && heartbeat_habilitado) {
            if (Temp_Expiro(&timer_heartbeat)) {
                Temp_Reiniciar(&timer_heartbeat);
                
                Uner_AbrirCarga(&pc_com, 3); // 1 byte ID + 2 bytes contador
                Uner_Agregar8(&pc_com, 0xAA); // ID del mensaje de latido
                Uner_Agregar16(&pc_com, heartbeat_contador);
                Uner_CerrarCarga(&pc_com);
                
                heartbeat_contador++; 
            }
        }

        // 5. Despachar los bytes por hardware
        Uner_Transmitir(&pc_com);
    }
    
    return 0;
}