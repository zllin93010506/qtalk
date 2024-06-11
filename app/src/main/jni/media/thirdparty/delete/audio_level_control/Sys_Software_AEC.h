
#ifndef _SOFTWARE_AEC_H_
#define _SOFTWARE_AEC_H_

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

int sw_aec_init(int sample_rate);
int sw_aec_push(char *pcm, int len);
int sw_aec_pop(char *pcm, int len);
int sw_aec_exit(void);

#endif // _SOFTWARE_AEC_H_
