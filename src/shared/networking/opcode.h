#ifndef OPCODE_H
#define OPCODE_H

#include "packetfilter.h"
#include "../session/sessionbase.h"

class SessionBase;

enum MessageTypes
{ 
    MSG_NULL_ACTION             = 0x000,
    NUM_MSG_TYPES               = 0x001,
};

enum SessionStatus
{
    STATUS_LOGGING = 0,                         // Collegato               
    STATUS_LOGGED,                              // Loggato           
    STATUS_TRANSFER,                            // Trasferimento di Cell       
    STATUS_LOGOUT,                              // Sta per sloggarsi
    STATUS_NEVER,                               // Non accettato 
    STATUS_UNHANDLED                            // Non valido
};

enum PacketProcessing
{
    PROCESS_INPLACE = 0,                         // processiamo il pacchetto dove si riceve
    PROCESS_THREADUNSAFE,                        // il pacchetto non è thread-safe
    PROCESS_THREADSAFE                           // il pacchetto è thread-safe
};

struct OpcodeHandler
{
    char const* name;
    SessionStatus status;
    PacketProcessing packetProcessing;
    void (SessionBase::*handler)(Packet& recvPacket);
};

extern OpcodeHandler opcodeTable[NUM_MSG_TYPES];

#endif