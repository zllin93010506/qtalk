#include "SoundClientService.h"
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"
#include "pointers.h"


JNIEXPORT void JNICALL Java_com_quanta_qtalk_sr2_SR2_1SoundClient_SPEEXInit(JNIEnv *env, jobject thiz)
{

    
    int sampleRate = 16000;
    echo_state = speex_echo_state_init(frame_size, filter_length);
	TxMsgQ = (TxMsgQue*)InitTxMsgQue();
	LOGD("echo state:%x 1", echo_state);
	den = speex_preprocess_state_init(frame_size, sampleRate);
    speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
    /*
    int i;
    float f;
	i=1;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_DENOISE, &i);
	i=0;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC, &i);
	i=8000;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
	i=0;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_DEREVERB, &i);
	f=.0;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
	f=.0;
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);
    */  
    isAlive = 1;
    
}

JNIEXPORT void JNICALL Java_com_quanta_qtalk_sr2_SR2_1SoundClient_SPEEXDestory(JNIEnv *env, jobject thiz)
{
	isAlive = 0;
	DestoryTxQue( (TxMsgQue *)TxMsgQ);
	
	speex_echo_state_destroy( echo_state);
	speex_preprocess_state_destroy(den);
	LOGD("echo state Destory");
}

JNIEXPORT void JNICALL Java_com_quanta_qtalk_sr2_SR2_1SoundClient_SpeexEchoCapture(JNIEnv *env, jobject thiz, jshortArray input_frame, jshortArray output_frame)
{
	jshort* jinput_frame = (*env)->GetShortArrayElements(env, input_frame, JNI_FALSE);
	jshort* joutput_frame = (*env)->GetShortArrayElements(env, output_frame, JNI_FALSE);
	//jsize datasize = (*env)->GetArrayLength(env, input_frame);
	//LOGD("datasize:%d",datasize);
	//memcpy( joutput_frame, jinput_frame, sizeof(short)*frame_size/2);
	
	TxMsg audio_msg;
	memset( &audio_msg, 0, sizeof(TxMsg));
	RecvTxMsg( TxMsgQ, &audio_msg);
	
	short audio_data_r[frame_size];
	memset( &audio_data_r, 0, sizeof(short)*frame_size);
	//memcpy( audio_data_r, audio_msg.audio_data, sizeof(short)*frame_size);
	
	speex_echo_cancellation(echo_state, jinput_frame, audio_data_r, joutput_frame);
	//speex_preprocess_run(den, joutput_frame);
	//memcpy( joutput_frame, jinput_frame, sizeof(short)*frame_size);

    (*env)->ReleaseShortArrayElements(env, input_frame, jinput_frame, 0);
	(*env)->ReleaseShortArrayElements(env, output_frame, joutput_frame, 0);
}
