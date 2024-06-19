#pragma once
#include "G7221Encoder.h"
#include "G7221Decoder.h"

class CG7221 : public CAudioCodec
{
public:
    CG7221(int bit_rate,short bandwidth);
    ~CG7221(void);
    int Encode(unsigned char* pDataIn, unsigned long dataLength,unsigned char* pBuffer,unsigned long bufferLen);
    int Decode(unsigned char* pDataIn, unsigned long dataLength,unsigned char* pBuffer,unsigned long bufferLen);

private:
    G7221Encoder  *m_g7221Encoder;
    G7221Decoder  *m_g7221Decoder;
};
