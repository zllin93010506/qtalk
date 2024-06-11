#include "X264Encoder.h"
#include <string.h> //memcpy
#include <stdlib.h>  //for malloc 
#include <stdio.h>
#include <math.h>

#define PL330_COMPATIBLE 1
//#define ENABLE_BITRATE_COUNT 1

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "libx264d.lib") 
#else
#pragma comment(lib, "libx264.lib") 
#endif
#endif

//for get second
#ifdef WIN32

#include <windows.h>
#include "gettimeofday.h"
#else
#include <sys/time.h>
#include "logging.h"
//for Android debug
#define TAG "CX264Encoder"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#define dxprintf LOGD	//steven hard
#endif

uint64_t currentTimeMs()
{
    struct timeval clock;
    memset(&clock,0,sizeof(clock));
    gettimeofday(&clock, 0);
    return (uint64_t)(clock.tv_sec * 1000 +clock.tv_usec/1000);
}
//// parsing I slice and P slice header to get frame number
//void h264_getframenum_pl330only(unsigned char *buf)
//{
//    int index = 0;    
//    int frame_type = 0;
//    unsigned short frame_num = 0;    
//    
//    if(buf[index+4] == 0x67) // H264 Sequence Parameter Set (0x67), I slice (2)
//    {        
//        index += 108;           // jump to I slice
//        
//        index += 5;             // jump to frame number byte
//        frame_num = buf[index];
//        frame_num <<= 8;        
//        frame_num |= buf[index+1];       
//        frame_num >>= 1;
//        frame_type = 2;
//        printf("I  %d\n",frame_num& 0x3FF);
//    }
//    else { // P slice (0)
//        index += 5;      // jump to frame number byte
//        frame_num = buf[index];
//        frame_num <<= 8;        
//        frame_num |= buf[index+1];       
//        frame_num >>= 3;
//        frame_type = 0;
//        printf("P  %d\n",frame_num& 0x3FF);
//    }   
//}

CX264Encoder::CX264Encoder(void)
{
	m_Codec         = NULL;
	m_frameWidth    = 0;
	m_frameHeight   = 0;
	m_frameSize     = 0;

    //cal bitrate and adjust it
    m_iFrameNumber  = 3;
    m_iFrameCount   = 0;
    m_sizeCount     = 0;
    m_t1 = m_t2     = (uint64_t)0.0;
    m_desireSize    = 300.0;
    m_enableAdjust  = false;
    m_setting       = false;
    m_remoteIsPL330 = true;    
    
}

CX264Encoder::~CX264Encoder(void)
{
	ReleaseX264Codec();
}

bool CX264Encoder::InitX264Codec(int nWidth, int nHeight, unsigned short usMaxKeyframe, int nVideoQuality, int nGop, int nMaxBnumber,int nColorSpace,bool bIsPL330)
{
    m_frameWidth    = nWidth;
    m_frameHeight   = nHeight;
    m_frameSize     = nWidth*nHeight;
    m_colorSpace    = nColorSpace;
    m_setting       = false;
    m_t1            = 0;
    m_sizeCount     = 0;
    m_iFrameCount   = 0;
    m_GOPnumber     = nGop;
    m_pFrameCount   = 1;
    m_lastBRCount   = 0;
    m_balanceQuality= 0;
    m_remoteIsPL330 = bIsPL330;

     /* Destroy previous handle */
     if (m_Codec != NULL)
     {
          ReleaseX264Codec();
     }

     /* Get default param */
    x264_param_default( &m_param );       

    if(m_remoteIsPL330)
    {
     #if X264_BUILD > 104    //Bill Tuen fast
        if(x264_param_default_preset(&m_param,x264_preset_names[0],x264_tune_names[6]) < 0)
            return false;
    #else       
        m_param.i_scenecut_threshold = 0;       //profile baseline
        m_param.b_deblocking_filter = 0;        //profile baseline
        //m_param.b_cabac = 0;                    //profile baseline
        //m_param.analyse.b_transform_8x8 = 0;    //profile main
        //motion estimation method, using umh if higher quality is essential.
        m_param.analyse.i_me_method = X264_ME_DIA;
        // adjust fast speed(0) and small size(7)
        m_param.analyse.i_subpel_refine = 0;     //size  // 0..5 -> 1..6
        m_param.analyse.i_trellis = 0;           //time
        //m_param.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
     #endif
        // if it set 0, it will multi thread every slice will spilt two space , but PL330 only decoder one thread
        m_param.i_threads       = 1; 
    }
    else //for profile high
    {                        
        //m_param.analyse.b_transform_8x8 = 0;    //profile main
        //motion estimation method, using umh if higher quality is essential.
        m_param.analyse.i_me_method = X264_ME_DIA;//X264_ME_HEX?X264_ME_DIA
        // adjust fast speed(0) and small size(7)
        m_param.analyse.i_subpel_refine = 5;        //size  // 0..5 -> 1..6
        m_param.analyse.i_trellis       = 0;        //time
        m_param.analyse.i_weighted_pred = X264_WEIGHTP_NONE; 
        m_param.i_threads               = X264_THREADS_AUTO;
    }

     m_param.i_csp           = nColorSpace;
     m_param.i_log_level     = X264_LOG_NONE;
     //m_param.pf_log = x264_log_vfw;
     //m_param.p_log_private   = NULL;

     // Set params: TODO to complete
     m_param.i_width         = nWidth;
     m_param.i_height        = nHeight;
     m_param.i_fps_num       = usMaxKeyframe;
     m_param.i_fps_den       = 1;

     m_param.i_keyint_min    = nGop/2;
     m_param.i_keyint_max    = nGop;        //VIDEO_GOP_SIZE
     m_param.i_bframe        = nMaxBnumber; //insert B-frame  seen decide how long did encoder buffer delay

     /*//m_param.i_bframe_pyramid = 2;
    //m_param.analyse.b_weighted_bipred = 0;
     //m_param.b_bframe_adaptive = 0;
     //m_param.analyse.b_bidir_me = 0;
     //m_param.i_bframe_bias = 0;
    //motion estimation method, using umh if higher quality is essential.
     m_param.analyse.i_me_method = X264_ME_DIA;//X264_ME_HEX?X264_ME_DIA
    //m_param.analyse.i_me_range  = 16;  
     m_param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;*/

    //zerolatency  
     m_param.b_vfr_input = 0;
     #if X264_BUILD > 69
         m_param.b_sliced_threads    = 1;  //no lantence for thread > 1
         m_param.rc.i_lookahead      = 0;  //removing lookahead might reduce latency?
     #endif
     #if X264_BUILD > 74
         m_param.i_sync_lookahead    = 0;
     #endif

     //m_param.b_version_info = 0;    //remove SEI for ET1
     switch (0)
     {
     case 0: // 1 PASS ABR
          m_param.rc.f_qcompress = 1.0;               //set this, remove latency of gop
          m_param.rc.i_rc_method = X264_RC_ABR;
          m_param.rc.i_bitrate   = nVideoQuality*2;     //x264_param_parse(&m_param,"bitrate","1000");
          break;
     case 1: // 1 PASS CQP
          //m_param.rc.i_qp_min      = 10;//20;
          //m_param.rc.i_qp_max      = 51;
          //m_param.rc.i_qp_step     = 1;
          m_param.rc.i_rc_method   = X264_RC_CQP;
          m_param.rc.i_qp_constant = nVideoQuality;//10 - 51
          break;
     }

     // for PL330
    if(m_remoteIsPL330)
    {
         // set frame reference to 1 to reduce encoding latency
          x264_param_parse(&m_param,"ref","2");           //m_param.i_frame_reference = 2;
          x264_param_parse(&m_param,"mvrange","11");      //m_param.analyse.i_mv_range = 11;
          x264_param_parse(&m_param,"partitions","i4x4"); //m_param.analyse.inter = X264_ANALYSE_I4x4;   //DISALBE PARTITION MODE                     
          #if X264_BUILD > 104                            //Bill Tuen fast
               x264_param_apply_fastfirstpass(&m_param);
               x264_param_apply_profile(&m_param,x264_profile_names[0]);
          #else  //for set profile baseline in x264_68
            m_param.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
            m_param.analyse.b_transform_8x8 = 0;    //profile main
            m_param.i_cqm_preset = X264_CQM_FLAT;   //profile main
            m_param.b_cabac  = 0;
            m_param.i_bframe = 0;
        #endif
    }
    else
    {    
        // set frame reference to 1 to reduce encoding latency
        x264_param_parse(&m_param,"ref","1");   //m_param.i_frame_reference = 1;
        //m_param.analyse.intra = X264_ANALYSE_I4x4;
        x264_param_parse(&m_param,"partitions","i4x4"); //m_param.analyse.inter = X264_ANALYSE_I4x4;// | X264_ANALYSE_PSUB16x16 | X264_ANALYSE_BSUB16x16;
    }

	/* Open the encoder */
	m_Codec = x264_encoder_open( &m_param );

	if (m_Codec == NULL)
		return false;

	/* Init the picture */
	x264_picture_alloc(&m_pic,m_param.i_csp, m_param.i_width, m_param.i_height);
	m_pic.i_pts = 0;
        
    //QueryPerformanceFrequency(&m_feq); //CPU jump time every sec 
	return true;
}
int CX264Encoder::EncodeX264(unsigned char* pInputBuffer, int nInputSize,unsigned char* pOutputBuffer,  bool& bKeyFrame, int frameType)
{
	if (m_Codec == NULL)
	    return -1;
    
    //set critical section
    if(m_setting)
    {
        x264_encoder_close(m_Codec);
	    m_Codec = x264_encoder_open( &m_param );
	    m_setting   = false;
        m_GOPnumber = m_param.i_keyint_max;
    }

	// set x264_picture_t
	m_pic.i_type    = frameType;
	
	if(m_param.rc.i_rc_method   == X264_RC_CQP)
		m_pic.i_qpplus1 = m_param.rc.i_qp_constant+1; //instead of x264_encoder_reconfig

	switch( m_pic.img.i_csp & X264_CSP_MASK )
	{
	case X264_CSP_I420:
	case X264_CSP_YV12:
		//m_pic.img.i_plane = 3;
		//m_pic.img.i_stride[0] = m_width;
		//m_pic.img.i_stride[1] = m_pic.img.i_stride[2] = m_width / 2;
        //memcpy(m_pic.img.plane[0],(uint8_t*)pInputBuffer,m_sizeImage*3/2);
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		m_pic.img.plane[1]    = m_pic.img.plane[0] + m_frameSize;
		m_pic.img.plane[2]    = m_pic.img.plane[1] + m_frameSize/ 4;
		break;

#if X264_BUILD > 76
	case X264_CSP_NV12:
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		m_pic.img.plane[1]    = m_pic.img.plane[0] + m_frameSize;
		break;
#else
	case X264_CSP_YUYV:
		//m_pic.img.i_plane = 1;
		//m_pic.img.i_stride[0] = 2 * m_frameWidth;
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		break;

	case X264_CSP_RGB:
		//m_pic.img.i_plane = 1;
		//m_pic.img.i_stride[0] = 3 * m_frameWidth;
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		break;

	case X264_CSP_BGR:
		//m_pic.img.i_plane = 1;
		//m_pic.img.i_stride[0] = 3 * m_frameWidth;
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		break;

	case X264_CSP_BGRA://?
		//m_pic.img.i_plane = 1;
		//m_pic.img.i_stride[0] = 4 * m_frameWidth;
		m_pic.img.plane[0]    = (uint8_t*)pInputBuffer;
		break;
#endif
	default:
		return -1;
	}
#ifdef ENABLE_BITRATE_COUNT
    //if(m_enableAdjust)
    {
        m_t2 = currentTimeMs(); 
        if(m_t1 == 0.0)
            m_t1 = currentTimeMs();    //QueryPerformanceCounter(&m_t1);    //CPU jump time before encode
    }
#endif
    int i_nal = 0;
	x264_nal_t* nal;
	x264_picture_t pic_out;
	/* encode it */
	if( x264_encoder_encode( m_Codec, &nal, &i_nal, &m_pic, &pic_out) < 0)
		return -1;    

    /* create bitstream, unless we're dropping it in 1st pass */
	int nOutputSize = 0;
	for(int i = 0; i < i_nal; i++ )
	{
		#if X264_BUILD < 76
			int packetSize = m_sizeImage - nOutputSize;
			x264_nal_encode( (uint8_t*)pOutputBuffer + nOutputSize, &packetSize, 1, &nal[i] );
			nOutputSize += packetSize;
		#else
			// to conform to new x264 API: data within the payload is already
			// nal-encoded, so we should just be able to grab
			int packetSize = (int)nal[i].i_payload;
			memcpy((uint8_t*)pOutputBuffer + nOutputSize,(uint8_t*)nal[i].p_payload, packetSize);
			nOutputSize += packetSize;
		#endif
	}    

	/* Set key frame only for IDR, as they are real synch point, 
       I frame aren't always synch point (ex: with multi refs, ref marking) */
	if( pic_out.i_type == X264_TYPE_IDR )
    {        
        bKeyFrame = true; 
#ifdef ENABLE_BITRATE_COUNT
        //if(m_enableAdjust)
        {       
            //dxprintf("I frame");
            if(m_iFrameCount < m_iFrameNumber)
            {
                m_sizeCount += nOutputSize;
                m_iFrameCount++;
            }
        }
#endif
    }
	else    // P frame
    {
        bKeyFrame = false;
        if(m_enableAdjust) //CQF
        {       
//            dxprintf("P frame");
            if(m_iFrameCount == m_iFrameNumber)
            {
                if(++m_pFrameCount == m_GOPnumber)//the last p frame in the GOP
                {
                    double temp_bitrate = (double)(m_sizeCount*8)/(m_t2 - m_t1);     //*8 for btye to bit bit/time kps  *1000     
                    double br_diff = (temp_bitrate - m_desireSize)/m_desireSize;   
                    if(br_diff > 0.12)
                    {
                        int step = (int)(fabs(br_diff)*5) +1;
                        if(step > 5)
                            step = 5;
                        dxprintf("%f kps-- step %d  quality %d m_desireSize:%f\n",temp_bitrate,step,m_param.rc.i_qp_constant,m_desireSize);
                        SetX264VideoQuality(m_param.rc.i_qp_constant+step);//down bit rate                         
                    }
                    else if(br_diff < -0.08)
                    {
                        int step = (int)(fabs(br_diff)*5) +1;
                        if(step > 5)
                            step = 5;
                        dxprintf("%f kps++ step %d  quality %d m_desireSize:%f\n",temp_bitrate,step,m_param.rc.i_qp_constant,m_desireSize);
                        SetX264VideoQuality(m_param.rc.i_qp_constant-step);//up bit rate
                    }                
                    else
                    {
                        dxprintf("%f kps quality %d\n",temp_bitrate,m_param.rc.i_qp_constant);
                        //first I frame in next loop
                        m_lastBRCount = 0;
                        m_balanceQuality = m_param.rc.i_qp_constant;
                    }
                    //first I frame in next loop
                    m_iFrameCount = 0;
                    m_pFrameCount = 1;
                    m_sizeCount = 0;
                    m_t1 = (uint64_t)0.0;
                }
            }
            m_sizeCount += nOutputSize;
        }
		else
		{
            //dxprintf("P frame");
#ifdef ENABLE_BITRATE_COUNT
            if(m_iFrameCount == m_iFrameNumber)
            {                
                if(++m_pFrameCount == m_GOPnumber)//the last p frame in the GOP
                {
                    double temp_bitrate = (double)(m_sizeCount*8)/(m_t2 - m_t1);     //*8 for btye to bit bit/time kps  *1000                     
					dxprintf("%.3f kps quality %d",temp_bitrate,m_param.rc.i_bitrate);                        
                    m_lastBRCount = 0;
                    m_balanceQuality = m_param.rc.i_qp_constant;
       
                    //first I frame in next loop
                    m_iFrameCount = 0;
                    m_pFrameCount = 1;
                    m_sizeCount = 0;
                    m_t1 = (uint64_t)0.0;
                }
            }
            m_sizeCount += nOutputSize;
#endif
		}
    }      
	return nOutputSize;
}

bool CX264Encoder::SetX264VideoQuality(int nQuality)
{
    if(m_Codec == NULL)
          return false;
     if(m_param.rc.i_rc_method == X264_RC_CQP)
     {
          if(nQuality < m_param.rc.i_qp_min )
               nQuality = m_param.rc.i_qp_min;
          else if(nQuality > m_param.rc.i_qp_max )
               nQuality = m_param.rc.i_qp_max;
          //Steven: Let quality between 20~30
        if(nQuality < 20 )
            nQuality = 20;
        else if(nQuality > 30 )
            nQuality = 30;

        if(abs(m_balanceQuality - nQuality) < 3)
        {          
            m_lastBRCount++;
            if(m_lastBRCount < 3)                 
            {
                //printf("give it a chance");
                return true;  
            }
            else if(m_lastBRCount < 5)
            {
                //printf("get average");             
                nQuality = (m_balanceQuality + nQuality)/2;
            }
            
        }
        if(m_param.rc.i_qp_constant != nQuality)
        {
            m_param.rc.i_qp_constant = nQuality;//10 - 51
	        m_setting = true;
        }

		//x264_encoder_reconfig(m_Codec,&m_param);
		return true;
	}
	else
		return false;
}
bool CX264Encoder::SetX264Bitrate(int nBitrate)
{
    if(m_Codec == NULL)
		return false;
    if(nBitrate == 0)
        nBitrate = 1;
    if(m_param.rc.i_rc_method == X264_RC_ABR)
     {
          m_param.rc.i_bitrate = nBitrate; // max = 5000
          m_setting = true;
          m_desireSize = nBitrate;  //kbps -> kBps (convert bit to byte)
          EnableX264AdjustBitrate(false);
          //x264_encoder_reconfig(m_Codec,&m_param);
          return true;
     }
     else if(m_param.rc.i_rc_method == X264_RC_CQP)
     {
        m_desireSize = nBitrate;  //kbps -> kBps (convert bit to byte)
        EnableX264AdjustBitrate(true);
		return true;
	}
	else
		return false;
}
void CX264Encoder::EnableX264AdjustBitrate(bool nEnable)
{
    m_enableAdjust = nEnable;
}
bool CX264Encoder::SetX264GOP(int nGop)
{
    if(m_Codec == NULL)
		return false;
    else
    {
        if(m_param.i_keyint_max != nGop)
        {
            m_param.i_keyint_min    = nGop/2;
            m_param.i_keyint_max    = nGop;        //VIDEO_GOP_SIZE
            m_GOPnumber             = nGop;
            m_param.i_fps_num       = nGop;        //for Steven want to set Gop equal fps
            //set critical section
            m_setting = true;
        }
         //x264_encoder_close(m_Codec);
         //m_Codec = x264_encoder_open( &m_param );
        //x264_encoder_reconfig(m_Codec,&m_param);
         return true;
    }
}

void CX264Encoder::ReleaseX264Codec()
{
     if (m_Codec!=NULL)
     {
        #if X264_BUILD > 104
            dxprintf("x264_encoder_delayed_frames %d\n",x264_encoder_delayed_frames(m_Codec));
        #endif
        x264_encoder_close(m_Codec);
          m_Codec = NULL;

        //if(m_pic.img.plane[0] != NULL)
        //    x264_picture_clean( &m_pic );
     }
}
