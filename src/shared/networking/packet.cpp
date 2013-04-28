#include "packet.h"

//WIP
// usare bytebuffer anche per plaintext ( da cambiare funzioni di crypto.cpp )

int Packet::Encrypt(ByteBuffer key)
{
    ByteBuffer ciphertext;
    int ret=0;

    ret = AesEncrypt(key, (ByteBuffer)(*this), ciphertext);
    ciphertext.hexlike();
    
    if(!ret)
    {
        INFO("debug", "PACKET: encrypted\n");
        m_encrypted = true;
        this->clear();
        if (ciphertext.size())
            this->append(ciphertext.contents(), ciphertext.size());
    }
    else
        INFO("debug", "PACKET: encryption failed\n");
    
    return ret;
}

int Packet::Decrypt(ByteBuffer key)
{
    ByteBuffer *ciphertext = (ByteBuffer*) this; // Mother Of God
    ByteBuffer plaintext;
    int ret=0;

    INFO("debug", "PACKET: decrypting\n");
    ciphertext->hexlike();

    ret = AesDecrypt(key, *ciphertext, plaintext);
    
    if (!ret)
    {
        INFO("debug", "PACKET: decrypted\n");
        m_encrypted = false;
        this->clear();
        this->append(plaintext);
    }
    else
        INFO("debug", "PACKET: decryption failed\n");
     
    return ret;
}

void Packet::Incapsulate(Packet& pkt)
{
    *this<<pkt.GetOpcode();
    *this<<pkt.size();
    if (pkt.size())
        this->append(pkt.contents(), pkt.size());
}
