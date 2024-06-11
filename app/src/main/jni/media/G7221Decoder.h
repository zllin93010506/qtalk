#pragma once
/************************************************************************************
 Constant definitions                                                    
*************************************************************************************/
#define MAX_SAMPLE_RATE 32000
#define MAX_FRAMESIZE   (MAX_SAMPLE_RATE/50)
#define MEASURE_WMOPS   1
#define WMOPS           1

#include "thirdparty/G7221/common/defs.h"
#include "thirdparty/G7221/common/count.h"
class G7221Decoder
{
public:
    G7221Decoder(int bit_rate,short bandwidth);
    ~G7221Decoder(void);    
    int Decode(unsigned char* pDataIn,const  unsigned long dataLength,short* pBuffer,const unsigned long bufferLen);
    
    Word32 m_BitRate;
    Word16 m_Bandwidth;        
    Word16 m_frame_size;    //output sample number  320
    Word16 m_number_of_regions;
    //processing data info
    Word16 m_number_of_bits_per_frame;  //m_BitRate/50 = 960
    Word16 m_SampleNumber;//Word16 m_number_of_16bit_words_per_frame; input sample number 60
    Word16 m_ByteNumber;    //120

    Word16 m_old_decoder_mlt_coefs[MAX_DCT_LENGTH];
    Word16 m_old_samples[MAX_DCT_LENGTH>>1];
    Word16 m_old_mag_shift;      

    Bit_Obj m_Bitobj;
    Rand_Obj m_Randobj;

    Word16 m_frame_error_flag;
    Word16 m_mlt_coefs[MAX_DCT_LENGTH];
    Word16 m_mag_shift;     
};
