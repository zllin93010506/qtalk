
#ifndef _SOFTWARE_AEC_H_
#define _SOFTWARE_AEC_H_

#include <jni.h>
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ANDROID_AEC_DEBUG
int sw_aec_init(int sample_rate, JavaVM *vm, jclass _cls);
#else
int sw_aec_init(int sample_rate);
#endif
int sw_aec_push(char *pcm, int len);
int sw_aec_pop(char *pcm, int len);
int sw_aec_exit(void);
int sw_aec_InitVad();

void exit_speex();
//double sw_aec_correlation(_aec_cor *as, short *x, short *y, int len, int flag);
void sw_aec_ALC(short *, int);
//void sw_aec_ALC_mic_ctrl(short *);

void sw_aec_delay_calc(int frame_count);


#ifdef __cplusplus
}
#endif
#endif // _SOFTWARE_AEC_H_
