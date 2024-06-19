#ifndef __X264Encoder_h__
#define __X264Encoder_h__

extern "C"
{
	#include "stdint.h"
	#include "x264.h"
}

//#define MAX_PATH 260

class CX264Encoder
{
public:
	CX264Encoder(void);
	~CX264Encoder(void);
	bool InitX264Codec(int nWidth, int nHeight, unsigned short usMaxKeyframe, int nVideoQuality, int nGop, int nMaxBnumber,int nColorSpace,bool bIsPL330);
	int  EncodeX264(unsigned char* pInputBuffer, int nInputSize,unsigned char* pOutputBuffer, bool& bKeyFrame, int frameType);
    bool SetX264VideoQuality(int nQuality);
    bool SetX264Bitrate(int nBitrate);
    bool SetX264GOP(int nGop);
    void EnableX264AdjustBitrate(bool nEnable);
	void ReleaseX264Codec();	
	int  			m_colorSpace;   //input pixel format

private:
	x264_t*         m_Codec;
	x264_picture_t  m_pic;          //input x264 buffer
    x264_param_t    m_param;        //x264 setting paramater
    int             m_frameSize;    //frameWidth * frameHeight
    int             m_frameWidth;
	int             m_frameHeight;
    bool            m_setting;     //critical section

    //cal bitrate and adjust it
    uint64_t m_t1,m_t2;             
    int m_iFrameNumber;            
    int m_iFrameCount;
    int m_sizeCount;
    double m_desireSize;
    bool m_enableAdjust;
    bool m_remoteIsPL330;
    int  m_lastBRCount;
    int  m_balanceQuality;
    int  m_GOPnumber;            
    int  m_pFrameCount;
};


#endif // __X264Encoder_h__
