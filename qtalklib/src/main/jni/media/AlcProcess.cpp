/**
 * @file AlcTransform.m Auto Level Control
 * audio_mediator
 * Input a audio data to detect active and adjust volume
 *
 * @author Bill Tocute on 2012/3/6.
 * Copyright 2012 Quanta. All rights reserved.
 */

#include "AlcProcess.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#ifdef IOS
#ifdef DEBUG
#define LOGD printf
#else 
#define LOGD //printf
#endif
#elif WIN32
#ifdef DEBUG
#define LOGD printf
#else 
#define LOGD //printf
#endif
#else   //Android
#include "logging.h"
#define TAG "AlcProcess"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) //__logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#endif

#define MAX_VALUE   0x7FFF  //2^15
#define ALC_HIGH_LVL -20
#define ALC_LOW_LVL -50     //-35  ALC_LOW_LVL > ALC_NOISE_LVL
#define ALC_NOISE_LVL -60   //-45
#define ALC_ATTACK_HIGH_CNT 6 //4
#define ALC_ATTACK_LOW_CNT 4 //7
//#define ALC_RELEASE_HIGH_CNT 500
//#define ALC_RELEASE_LOW_CNT 50
//#define ALC_MIC_GAIN_LOCK_CNT 50        //lock time
const int MIN_VALUE = (int)pow(2.0, sizeof(unsigned short)*8.0 -1)*-1;

#pragma mark -
/**
 @brief AlcProcess construction.
 Start WebRtcVad.
 
 @param[in]  sample_rate     The sample rate of the audio.
 */
AlcProcess::AlcProcess(int sample_rate)
{
    mSampleRate = 0;

    mVadinst    = NULL;
    mIsStarted  = false;
    mHasVoice   = false;
    mIsEnable   = false;
    
    m_alc_high_cnt  = 0;
    m_alc_low_cnt   = 0;
    mScale          = 0;

    //m_b_alc_gain_lock   = false;
    //m_b_alc_mic_gain_change_lock    = false;
    m_volume_cnt    = 0;
    //m_alc_lock_cnt  = 0;
    
    bool result = true;
    if(sample_rate != 0)
    {
        if(mVadinst != NULL)
        {
            WebRtcVad_Free(mVadinst);
            mVadinst = NULL;
        }
        
        if(WebRtcVad_Create(&mVadinst))
        {
            LOGD("vad create error\n");
            result = false;
        }
        if(result && WebRtcVad_Init(mVadinst))
        {
            LOGD("vad init error\n");
            result = false;
        }
        if(result && WebRtcVad_set_mode(mVadinst, 3)) // mode:3, very aggressive mode
        {
            LOGD("set vad mode error\n");
            result = false;
        }
        else
        {
            LOGD("VAD in aggressive mode\n");
        }
        
        if(result)
        {
            mSampleRate = sample_rate;
            setEnable(true);
            mIsStarted  = true;
        }
        else if(mVadinst != NULL)
        {
            WebRtcVad_Free(mVadinst);
            mVadinst = NULL;
        }
    }
}

/**
 @brief AlcProcess destruction.
  Stop WebRtcVad.
 */
AlcProcess::~AlcProcess()
{
    if(mIsStarted == true)
    {
        mIsStarted = false;
        if(mVadinst != NULL)
        {
            WebRtcVad_Free(mVadinst);
            mVadinst = NULL;
        }
    }
}


#pragma mark - IAudioSink
/**
 @brief AlcProcess process.
 Process auto level control.
 
 @param[in]  data     The audio data.
 @param[in]  len      The length of the audio.
 @return     if do ALC process.
 */
bool AlcProcess::process(short* data ,int length)
{    
    if(mIsStarted == true && mIsEnable == true)
    {
        mHasVoice = false;
        
        //if([self preAdjustVoice])
        {
            if(WebRtcVad_Process(mVadinst, mSampleRate,data, length))
            {
                mHasVoice = true;
                
                //to get now voice (db)
                double x_rms = sw_aec_RMS(data, length);
                
                //modify scale
                adjustScale(x_rms);
                if (mScale != 0)
                    changeVoice(data,length);

                double x_rms_old = x_rms;

                x_rms = sw_aec_RMS(data, length);
                if(x_rms >= ALC_HIGH_LVL)
                    mScale--;
                else if (x_rms <= ALC_LOW_LVL && x_rms >= ALC_NOISE_LVL)
                    mScale++;
                
                /*static int count = 0;
                //if(count %25 == 0)
                {
                    LOGD("x_rms : %.2f->%.2f scale : %d\n",x_rms_old ,x_rms ,mScale);
                }
                count++;*/
            }
        }
        return true;
    }
    else
        return false;
}

/**
 @brief AlcProcess setEnable.
 Enable alc.
 
 @param[in]  enable     The adjust volume is enable.
 */
void AlcProcess::setEnable(bool enable)
{
    mIsEnable = enable;
    mScale = 0;
}

/**
 @brief AlcProcess adjustScale.
 To cal the rms of the audio.
 
 @param[in]  data   The audio data.
 @param[in]  len    The length of the audio.
 @return     The rms of audio data.
 */
double AlcProcess::sw_aec_RMS(short *x, int len)
{
    long long x_sum =0;
    double x_rms = 0;

    //square
    for(int i=0; i<len; i++)
    {
        x_sum += (*x)*(*x);
        x++;
    }
    
    //mean
    x_rms = (double)x_sum/(double)len;
    
    //root
    x_rms = sqrt(x_rms);
    
    if(x_rms != 0)
        x_rms = 20*log10(x_rms/MAX_VALUE);
    else
        x_rms = ALC_NOISE_LVL-15;   //60 should consider more in detail, how much is noise level ???
    
    return x_rms;
}


/**
 @brief AlcProcess adjustScale.
 Adjust voice volume to cal the mScale.
 
 @param[in]  x_rms     The rms of audio data.
 */
void AlcProcess::adjustScale(double x_rms)
{
    if (m_volume_cnt > 0) 
    {
        m_volume_cnt--;
        return;
    }
    
    //chack audio level (rms) if too bigger or to  smaller
    if(x_rms >= ALC_HIGH_LVL)
    {   
        m_alc_high_cnt++;
        m_alc_low_cnt = 0;
    }
    else if (x_rms <= ALC_LOW_LVL && x_rms >= ALC_NOISE_LVL)
    {
        m_alc_low_cnt++;
        m_alc_high_cnt = 0;
    }
    else
    {
        m_alc_high_cnt = 0;
        m_alc_low_cnt = 0;
    }
    
    if(m_alc_high_cnt > ALC_ATTACK_HIGH_CNT)
    {
        m_alc_high_cnt = 0;
        LOGD("voice too large\n");
        mScale -= 10;
        m_volume_cnt = 100;
        
        /*if(m_volume_cnt < 3)
         m_volume_cnt++;
         
         if(!m_b_alc_gain_lock)
         {
         m_b_alc_mic_gain_change_lock = true;
         m_alc_lock_cnt = ALC_MIC_GAIN_LOCK_CNT;
         if(m_volume_cnt == 3)
         {
         m_b_alc_gain_lock = true;
         m_alc_lock_cnt = ALC_RELEASE_HIGH_CNT;
         printf("\n\nALC lock for 10 secs, gain set to 15dB\n\n");
         }
         //sw_aec_ALC_mic_ctrl(x_rms);
         }*/
    }
    else if(m_alc_low_cnt > ALC_ATTACK_LOW_CNT)
    {
        m_alc_low_cnt = 0;
        LOGD("voice too small\n");
        mScale += 10;
        m_volume_cnt = 100;
        
        /*if(m_volume_cnt > -2)
         m_volume_cnt--;
         
         if(!m_b_alc_gain_lock)
         {
         m_b_alc_mic_gain_change_lock = true;
         m_alc_lock_cnt = ALC_MIC_GAIN_LOCK_CNT;
         if(m_volume_cnt == 3)
         {
         m_b_alc_gain_lock = true;
         m_alc_lock_cnt = ALC_RELEASE_HIGH_CNT;
         printf("\n\nALC lock for 10 secs, gain set to 15dB\n\n");
         }
         //sw_aec_ALC_mic_ctrl(x_rms);
         }*/
    }
}


/**
 @brief AlcProcess changeVoice.
 Change voice volume by mScale.
 
 @param[in]  data    The audio data.
 @param[in]  len     The length of the audio.
 */
void AlcProcess::changeVoice(short* data,int len)
{
    int temp = 0;
    short sign = 0;
    int slide1 = 240;
    int slide2 = 700;
    for (int i =0; i<len; i++)
    {   
        if(data[i] > 0)
            sign = 1;
        else if(data[i] < 0)
            sign = -1;
        else 
            sign = 0;
        
        if(abs(data[i]) < slide1)
            temp = data[i];
        else if(abs(data[i]) < slide2)
            temp = data[i] + sign*mScale/2;
        else if(abs(data[i]) < MAX_VALUE - slide2)
            temp = data[i] + sign*mScale;
        else if(abs(data[i]) < MAX_VALUE - slide1)
            temp = data[i] + sign*mScale/2;
        else if(abs(data[i]) < MAX_VALUE)
            temp = data[i];
        
        if(temp > MAX_VALUE - 1)//up bound
        {
            data[i] = MAX_VALUE - 1;
        }
        else if(temp < MIN_VALUE) //low bound
        {
            data[i] = MIN_VALUE;
        }
        else
            data[i] = temp;
    }
}

/*
 -(bool)preAdjust
 {
 if(m_b_alc_gain_lock)
 {
 if(m_alc_lock_cnt)
 m_alc_lock_cnt--;
 if(m_alc_lock_cnt == 0){
 m_b_alc_gain_lock = false;
 printf("ALC unlock!!!!\n");    
 }
 return  true;
 }
 else if(m_b_alc_mic_gain_change_lock)
 {  // do not change mic gain too fast, must have to lock for a specific time period.
 if(m_alc_lock_cnt)
 m_alc_lock_cnt--;
 if(m_alc_lock_cnt == 0){
 m_b_alc_mic_gain_change_lock = false;          
 }
 return  false;
 }
 return  true;
 }*/
