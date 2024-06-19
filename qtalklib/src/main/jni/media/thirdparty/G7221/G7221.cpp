#include "../AudioCodec.h"
#include "common/defs.h"
#include "common/count.h"
#include "G7221.h"

CG7221::CG7221(int bit_rate,short bandwidth)
{
    m_g7221Encoder = new G7221Encoder(bit_rate, bandwidth);
    m_g7221Decoder = new G7221Decoder(bit_rate, bandwidth);
}

CG7221::~CG7221(void)
{
    if(m_g7221Encoder != NULL)
        delete m_g7221Encoder;

    if(m_g7221Decoder != NULL)
        delete m_g7221Decoder;
}

int CG7221::Encode(unsigned char* pDataIn, unsigned long dataLength,unsigned char* pBuffer,unsigned long bufferLen)
{
    return m_g7221Encoder->Encode(pDataIn,dataLength,pBuffer,bufferLen);     
}

int CG7221::Decode(unsigned char* pDataIn, unsigned long dataLength,unsigned char* pBuffer,unsigned long bufferLen)
{
    return m_g7221Decoder->Decode(pDataIn,dataLength,pBuffer,bufferLen);     
}
