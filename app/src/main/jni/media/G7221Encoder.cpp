#include "G7221Encoder.h"
#include <netinet/in.h>

G7221Encoder::G7221Encoder(Word32 bit_rate,Word16 bandwidth)
{    
    m_BitRate   = bit_rate;
    m_Bandwidth = bandwidth;
    
    if ((m_BitRate < 16000) || (m_BitRate > 48000) || ((m_BitRate/800)*800 != m_BitRate))
    {
        //printf("codec: Error. bit-rate must be multiple of 800 between 16000 and 48000\n");
        exit(1);
    }

    m_number_of_bits_per_frame = (Word16)(m_BitRate/50);     
    m_SampleNumber = m_number_of_bits_per_frame/16;
    m_ByteNumber= m_number_of_bits_per_frame/8;
    
    if (m_Bandwidth == 7000)
    {
        m_number_of_regions = NUMBER_OF_REGIONS;
        m_frame_size = MAX_FRAMESIZE >> 1;
    }
    else if (m_Bandwidth == 14000)
    {
        m_number_of_regions = MAX_NUMBER_OF_REGIONS;
        m_frame_size = MAX_FRAMESIZE;
    }
         
    /* initialize the mlt history buffer */
    for (int i=0;i<m_frame_size;i++)
        m_History[i] = 0;

    #if MEASURE_WMOPS
    Init_WMOPS_counter ();
    #endif
}

G7221Encoder::~G7221Encoder(void)
{
    #if MEASURE_WMOPS
    WMOPS_output (0);
    #endif 
}

int G7221Encoder::Encode(short* pDataIn,const unsigned long dataLength,unsigned char* pBuffer,const unsigned long bufferLen)
{
    #if MEASURE_WMOPS
    Reset_WMOPS_counter ();
    #endif
    Word16* result_buffer = (Word16*)pBuffer;
    /* Convert input samples to rmlt coefs */
    m_mag_shift = samples_to_rmlt_coefs((Word16*)pDataIn, m_History, m_mlt_coefs, (Word16)dataLength);

    /* Encode the mlt coefs */
    encoder(m_number_of_bits_per_frame, m_number_of_regions, m_mlt_coefs, m_mag_shift, result_buffer);

    int result = 0;
    if( m_BitRate == 48000 )
	{
        result = G722_1_ENCODE_BUF_LEN_48K;
	}
	else if( m_BitRate == 32000 )
	{
	    result = G722_1_ENCODE_BUF_LEN_32K;
	}
	else if( m_BitRate == 24000 )
	{
	    result = G722_1_ENCODE_BUF_LEN_24K;
	}
	else if( m_BitRate == 16000 )
	{
	    result = G722_1_ENCODE_BUF_LEN_16K;
	}
    //Convert the output array to network order
    for (int i=0;i<result/2;i++)
    {
        result_buffer[i] = htons(result_buffer[i]);
    }
    return result;
}
