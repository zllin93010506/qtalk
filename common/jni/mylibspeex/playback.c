#include "SoundServerService.h"
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"
#include "pointers.h"

JNIEXPORT void JNICALL Java_com_quanta_qtalk_sr2_SR2_1SoundServer_SpeexEchoPlayback(JNIEnv *env, jobject thiz, jshortArray playback_data)
{
	jshort* jplayback_data = (*env)->GetShortArrayElements(env, playback_data, JNI_FALSE);
	
	
	TxMsg audio_msg;
	memcpy( audio_msg.audio_data, jplayback_data, sizeof(short)*frame_size);
	audio_msg.data_len = frame_size;
	if(isAlive == 1)
	{
		SendTxMsg( TxMsgQ, audio_msg);
		//speex_preprocess_run(den, jplayback_data);
	}
    (*env)->ReleaseShortArrayElements(env, playback_data, jplayback_data, 0);
}
