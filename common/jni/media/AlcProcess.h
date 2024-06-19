/**
 * @file AlcTransform.h Auto Level Control
 * audio_mediator
 * Input a audio data to detect active and adjust volume
 *
 * @author Bill Tocute on 2012/3/6.
 * Copyright 2012 Quanta. All rights reserved.
 */
#include "webrtc_vad.h"

class AlcProcess
{
public:
    AlcProcess(int sample_rate);
    ~AlcProcess();
    bool process(short* data ,int length);
    void setEnable(bool enable);
    
private:
    double sw_aec_RMS(short *x, int len);
    void adjustScale(double x_rms);
    void changeVoice(short* data,int len);
    
    int         mSampleRate;    //< sample rate of audio (8000/16000)
    bool        mIsStarted;     //< the action is runable
    bool        mIsEnable;      //< the adjust volume is enable
    bool        mHasVoice;      //< detect voice active
    VadInst     *mVadinst;      //< web_rtc
    int         m_alc_high_cnt; //< high voice count depend on ALC_ATTACK_HIGH_CNT
    int         m_alc_low_cnt;  //< low voice count depend on ALC_ATTACK_LOW_CNT
    int         mScale;         //< the scale about djust volume
    
    //bool            m_b_alc_gain_lock;
    //bool            m_b_alc_mic_gain_change_lock;
    int         m_volume_cnt;
    //int             m_alc_lock_cnt; //lock time
};
