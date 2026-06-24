#ifndef APP_COMANDOS_H_
#define APP_COMANDOS_H_

#include <stdint.h>
#include "../utils/uner_protocol.h"

#define CMD_HANDSHAKE       0x01    
#define CMD_GET_VERSION     0x02    
#define CMD_SET_SERVO       0x03    
#define CMD_GET_DISTANCE    0x04    
#define CMD_GET_IR_STATES   0x05    
#define CMD_SET_BELT        0x06    
#define CMD_SEND_LOG        0x09    
#define CMD_PING            0x15    
#define CMD_CLOSE           0x16    

#define CMD_SET_VARIABLE    0x25
#define CMD_GET_VARIABLE    0x26

#define ACK_HANDSHAKE       0x81    
#define ACK_GET_VERSION     0x82    
#define ACK_SET_SERVO       0x83    
#define ACK_GET_DISTANCE    0x84    
#define ACK_GET_IR_STATES   0x85    
#define ACK_SET_BELT        0x86    
#define ACK_PONG            0x95    

#define ERR_UNKNOWN_CMD     0xFF    
#define ERR_BAD_PAYLOAD     0xFE

typedef enum {
    eSetServoMin0=0,
    eSetServoMax0,
    eSetServoMin1,
    eSetServoMax1,
    eSetServoMin2,
    eSetServoMax2,
    eSetCiego,
    eSetDelta,
    eSetCoordenadaEstacion1,
    eSetCoordenadaEstacion2,
    eSetCoordenadaEstacion3,
    eSetMsDesplegar0,
    eSetMsRetraer0,
    eSetMsEsperar0,
    eSetMsDesplegar1,
    eSetMsRetraer1,
    eSetMsEsperar1,
    eSetMsDesplegar2,
    eSetMsRetraer2,
    eSetMsEsperar2,
    eSetDistanciaBase,
    eSetDisparosMax
}_eSetVariables;

void Comandos_Procesar(UnerProtocol_t *u);
void Comandos_EnviarLog(const char* mensaje);

#endif /* APP_COMANDOS_H_ */