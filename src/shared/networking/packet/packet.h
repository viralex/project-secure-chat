#ifndef _PACKET_H
#define _PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../../common.h"
using namespace std;


class Packet
{
    public:
        
		Packet(uint16 opcode) : m_opcode(opcode) 
        {
            gettimeofday(&m_createTime, NULL);
        }
        
		// costruttore di copia
        Packet(const Packet &packet) : m_opcode(packet.m_opcode), m_createTime(packet.m_createTime), 
                                       m_data(packet.m_data){}

        uint16 GetOpcode() const { return m_opcode; }
        void SetOpcode(uint16 opcode) { m_opcode = opcode; }
        void ResetTime() { gettimeofday(&m_createTime, NULL); }        
        uint32 GetTime() // In millisecondi
        {
            uint32 seconds  = m_createTime.tv_sec;  // - start.tv_sec;  TODO Quando è stato avviato il programma
            uint32 useconds = m_createTime.tv_usec; // - start.tv_usec; TODO Quando è stato avviato il programma
            return ((seconds) * 1000 + useconds/1000.0) + 0.5;
        }

        string m_data;       

    protected:
        uint16 m_opcode;        
        timeval m_createTime;
};
#endif
