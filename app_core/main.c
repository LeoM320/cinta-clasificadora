#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/include/hal_timer.h"
#include "hal/include/hal_uart.h"
#include "hal/include/hal_servo.h"

#include "utils/uner_protocol.h"
#include "app/comandos.h"
#include "utils/heartbeat.h"
#include "utils/temporizador.h" 
#include "app_cinta/app_cinta.h" 

// Variables globales
UnerProtocol_t protocolo_uart;

// ==========================================
// CALLBACKS DE RED Y ESTADO VISUAL
// ==========================================

void Evento_ConexionEstablecida(void) {
    // 0xAA = 10101010 (Parpadeo rápido a 4 Hz indicando tráfico de red)
    Heartbeat_SetSequence(0xAA);
    AppCinta_Iniciar();
}

void Evento_ConexionPerdida(void) {
    // 0x80 = 10000000 (Flash muy corto una vez por segundo indicando Standby)
    Heartbeat_SetSequence(0x80);
    Comandos_EnviarLog("FATAL: PC DESCONECTADA.");
    
    AppCinta_Detener();
    
    // Alineación segura de servos
    for(uint8_t i = 0; i < 3; i++) {
        HAL_Servo_SetAngle(i, 90);
    }
}

// ==========================================
// FUNCIÓN PRINCIPAL
// ==========================================

int main(void)
{
    // 1. SECCIÓN CRÍTICA
    HAL_DISABLE_INTERRUPTS();

    // 2. Inicialización de Hardware
    GPIO_Init();
    HAL_Timer0_Init();      
    HAL_UART_Init(115200);  

    // 3. Inicialización de Periféricos y Actuadores
    HCSR04_InitContinuous(false);
    HAL_Servo_Init();
    
    for(uint8_t i = 0; i < 3; i++) {
        HAL_Servo_Enable(i);       
        HAL_Servo_SetAngle(i, 90);  
    }

    // Arrancamos el Heartbeat en modo Desconectado (0x80) a 125ms/bit
    Heartbeat_Init(125, 0x80); 
    
    Uner_Init(&protocolo_uart);
    
    // Inyección de dependencias (Callbacks)
    Uner_RegistrarCallbackConexion(&protocolo_uart, Evento_ConexionEstablecida);
    Uner_RegistrarCallbackDesconexion(&protocolo_uart, Evento_ConexionPerdida);

    // 4. Fin de Sección Crítica
    HAL_ENABLE_INTERRUPTS();
    
    AppCinta_Init();
    Comandos_EnviarLog("SISTEMA INICIADO. ESPERANDO A QT...");

    // 5. SUPER LOOP ASÍNCRONO NO BLOQUEANTE
    while (1)
    {
        uint32_t ahora = HAL_GetMillis();
        
        // A. Gestión de Protocolo
        Uner_Recibir(&protocolo_uart, ahora);
        Comandos_Procesar(&protocolo_uart);
        Uner_Transmitir(&protocolo_uart);
        
        // B. Supervisor de Seguridad (Watchdog de hardware)
        Uner_VerificarLatido(&protocolo_uart, ahora);

        // C. Motores asíncronos de bajo nivel
        HCSR04_TaskContinuous();
        Heartbeat_Task();
        
        // D. Lógica principal de la aplicación
        AppCinta_Task();
    }

    return 0;
}

// ==========================================
// ISR
// ==========================================
ISR(PCINT0_vect) {
    HCSR04_EXTI_Handler();
}