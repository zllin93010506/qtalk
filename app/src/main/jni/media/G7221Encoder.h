#pragma once
/************************************************************************************
 Constant definitions                                                    
*************************************************************************************/
#define MAX_SAMPLE_RATE 32000
#define MAX_FRAMESIZE   (MAX_SAMPLE_RATE/50)    //640
#define MEASURE_WMOPS   1
#define WMOPS           1

#define G722_1_ENCODE_BUF_LEN_48K	120	//7K: 640B->120B->640B; 14K: 1280B->120B->1280B
#define G722_1_ENCODE_BUF_LEN_32K	80	//7K: 640B-> 80B->640B; 14K: 1280B-> 80B->1280B
#define G722_1_ENCODE_BUF_LEN_24K	60	//7K: 640B-> 60B->640B; 14K: 1280B-> 60B->1280B
#define G722_1_ENCODE_BUF_LEN_16K	40	//7K: 640B-> 40B->640B; 14K: 1280B-> 40B->1280B

#include "thirdparty/G7221/common/defs.h"
#include "thirdparty/G7221/common/count.h"

class G7221Encoder
{
public:
    G7221Encoder(Word32 bit_rate,Word16 bandwidth);
    ~G7221Encoder(void);
    int Encode(short* pDataIn,const unsigned long dataLength,unsigned char* pBuffer,const unsigned long bufferLen);

    Word32 m_BitRate;
    Word16 m_Bandwidth;
    Word16 m_frame_size;    //input sample number   320
    Word16 m_number_of_regions;
    //processed data info
    Word16 m_number_of_bits_per_frame;  //m_BitRate/50 = 960
    Word16 m_SampleNumber;//Word16 m_number_of_16bit_words_per_frame;  output sample number 60
    Word16 m_ByteNumber;    //120
                
    Word16 m_History[MAX_FRAMESIZE];
    Word16 m_mlt_coefs[MAX_FRAMESIZE];
    Word16 m_mag_shift;
};
