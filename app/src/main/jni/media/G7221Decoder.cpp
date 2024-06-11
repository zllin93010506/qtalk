#include "G7221Decoder.h"
#include <netinet/in.h>

G7221Decoder::G7221Decoder(int bit_rate,short bandwidth)
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
    
    /* initialize the coefs history */
    for (int i=0;i<m_frame_size;i++)
        m_old_decoder_mlt_coefs[i] = 0;    

    for (int i = 0;i < (m_frame_size >> 1);i++)
  	    m_old_samples[i] = 0;

    /* initialize the random number generator */
    m_Randobj.seed0 = 1;
    m_Randobj.seed1 = 1;
    m_Randobj.seed2 = 1;
    m_Randobj.seed3 = 1;

    m_frame_error_flag = 0;
    m_old_mag_shift=0;

    #if MEASURE_WMOPS
    Init_WMOPS_counter ();
    #endif
}

G7221Decoder::~G7221Decoder(void)
{
    #if MEASURE_WMOPS
    WMOPS_output (0);
    #endif 
}
//Bitrate:48K
//	datalength : 120 => 640
//  datalength : 240 => 1280
//Bitrate
int G7221Decoder::Decode(unsigned char* pDataIn,const unsigned long dataLength,short* pBuffer,const unsigned long bufferLen)
{
    unsigned short *p_input_buffer = (unsigned short *)pDataIn;
    //Convert the input array from network order to host order
    for (int i=0;i<dataLength/2;i++)
    {
        p_input_buffer[i] = ntohs(p_input_buffer[i]);
    }
    /* reinit the current word to point to the start of the buffer */
    m_Bitobj.code_word_ptr = (Word16*)pDataIn;
    m_Bitobj.current_word =  (Word16)*pDataIn;
    m_Bitobj.code_bit_count = 0;
    m_Bitobj.number_of_bits_left = m_number_of_bits_per_frame;
        
    #if MEASURE_WMOPS
    Reset_WMOPS_counter ();
    #endif
    /* process the out_words into decoder_mlt_coefs */
    decoder(&m_Bitobj, &m_Randobj,m_number_of_regions,m_mlt_coefs, &m_mag_shift, &m_old_mag_shift,m_old_decoder_mlt_coefs,m_frame_error_flag);
           
    /* convert the decoder_mlt_coefs to samples */
    rmlt_coefs_to_samples(m_mlt_coefs, m_old_samples, (Word16*)pBuffer, m_frame_size, m_mag_shift);

    return m_frame_size*2;
}
