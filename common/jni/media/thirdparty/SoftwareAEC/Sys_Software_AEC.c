#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "Sys_Software_AEC.h"
#include "webrtc_vad.h"
#include "vad_core.h"

#if defined(__ANDROID__)
    #include <sys/system_properties.h>
    
    //#include "logging.h"
    //#ifdef LOGD
    //    #undef LOGD
    //#endif
    //#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG,"QSE_AEC",__VA_ARGS__)
    //#ifdef printf
    //    #undef printf
    //#endif
    //#define printf    LOGD
    
    #include <android/log.h>
    #ifdef LOGD
        #undef LOGD
    #endif
    #define LOGD(...)   __android_log_print(ANDROID_LOG_DEBUG,"QSE_AEC",__VA_ARGS__)
    #ifdef printf
        #undef printf
    #endif
    #define printf      LOGD
#endif

static int flag_init = 0;
int read_pos = 0;
int write_pos = 0;
int delay = 0;
int tail = 0;
int rate = 0;
int frame_unit = 0;
int FIFO_SIZE = 50;
int g_record_pcm = 0;
double g_mic_gain = 1.0;

SpeexEchoState *echo_state;
SpeexPreprocessState *speex_pre_state;
spx_int16_t *aec_fifo;
spx_int16_t *aec_sine_fifo;
spx_int16_t *oBuf;
spx_int16_t *iBuf;
spx_int16_t *xBuf;
pthread_mutex_t p_aec_mutex;

#if defined(__ANDROID__)
char product_name_string[PROP_VALUE_MAX+1];
#else
char product_name_string[150];
#endif

int sw_aec_write_file(uint8_t *filename, uint8_t *pBuf, int size_byte, int append)
{
    FILE *fpOutput = NULL;

    if(append) fpOutput = fopen(filename, "ab+");
    else fpOutput = fopen(filename, "wb+");

    if(fpOutput != NULL) {
        fseek(fpOutput, 0, SEEK_END);
        fwrite((void*)pBuf, 1, size_byte, fpOutput);
    }
    else
        printf("<error> %s(), open %s failed, size %d\n", __FUNCTION__, filename, size_byte);

    if(fpOutput)
        fclose(fpOutput);
    return 0;
}

#if 0
int get_value_by_file(char *filename)
{
    FILE* file = fopen(filename, "r"); /* should check the result */
    char line[256];

    if(file == NULL) return -1;

    memset(line, 0x0, 256);
    while (fgets(line, sizeof(line), file)) {
        printf("=> %s: %d", filename, atoi(line));
    }
    fclose(file);

    return line[0]!=0?atoi(line):-1;
}
#else
double get_value_by_file(char *filename)
{
    FILE* file = fopen(filename, "r"); /* should check the result */
    char line[256];

    if(file == NULL) return -1;

    memset(line, 0x0, 256);
    while (fgets(line, sizeof(line), file)) {
        printf("=> %s: %d", filename, atoi(line));
    }
    fclose(file);

    return line[0]!=0?(double)atof(line):(double)(-1.0);
}
#endif

void init_global_var(void)
{
    double value;

    read_pos = 0;
    write_pos = 0;
    delay = 0;
    tail = 0;
    rate = 0;
    frame_unit = 0;
    FIFO_SIZE = 50;
    g_mic_gain = 1.0;
    g_record_pcm = 0;

    echo_state = NULL;
    speex_pre_state = NULL;
    aec_fifo = NULL;
    aec_sine_fifo = NULL;
    oBuf = NULL;
    iBuf = NULL;
    xBuf = NULL;
    pthread_mutex_init(&p_aec_mutex, NULL);

    value = get_value_by_file("/sdcard/record_pcm.txt");
    if(value != (double)(-1.0)) g_record_pcm = (int)value;


#if defined(__ANDROID__)
    __system_property_get("ro.product.name", product_name_string);
#else
    strcpy(product_name_string, "notAndroid");
#endif
}

int init_speex(int sample_rate)
{
    int tail_length, ret = 0;
    int agc_ctrl = 0, agc_inc = 15, agc_dec = -5, agc_level = 8000, agc_target = 24000;
    int aec_ctrl = 1, aec_echo_suppress = -75;
    int noise_ctrl = 0, noise_suppress = -15;
    double value = 0.0;

    /* 
        initial parameters 
    */
    if(sample_rate == 16000) frame_unit = 320;
    else frame_unit = 160;

    if(strstr(product_name_string, "fg6") != NULL) {
	    delay = 1; tail = 500; g_mic_gain = 1.0; aec_echo_suppress = -75; noise_ctrl = 1; noise_suppress = -15; agc_ctrl = 0;
    }
    else if(strstr(product_name_string, "qf3") != NULL) {
        delay = 1; tail = 500; g_mic_gain = 1.0; aec_echo_suppress = -75; noise_ctrl = 1; noise_suppress = -15; agc_ctrl = 0;
    }
    /* QF3a */
    else if(strstr(product_name_string, "QT-195W-aa2") != NULL) {
        delay = 4; tail = 500; g_mic_gain = 1.0; aec_echo_suppress = -160; noise_ctrl = 1; noise_suppress = -15; agc_ctrl = 0;
    }
    /* QF5 */
    else if(strstr(product_name_string, "rk3399_mid_qf5") != NULL) {
        delay = 1; tail = 500; g_mic_gain = 1.8; aec_echo_suppress = -160; noise_ctrl = 0; noise_suppress = -15; agc_ctrl = 0;
    }
    /* default */
    else {
	    delay = 1; tail = 500; g_mic_gain = 1.0; aec_echo_suppress = -75; noise_ctrl = 1; noise_suppress = -15; agc_ctrl = 0;
    }

    /* 
        update parameters by config files 
    */
    value = get_value_by_file("/sdcard/mic_gain.txt");
    if(value != (double)(-1.0)) g_mic_gain = value;
    value = get_value_by_file("/sdcard/aec_tail.txt");
    if(value != (double)(-1.0)) tail = (int)value;
    value = get_value_by_file("/sdcard/aec_delay.txt");
    if(value != (double)(-1.0)) delay = (int)value;
    value = get_value_by_file("/sdcard/aec_echo.txt");
    if(value != (double)(-1.0)) aec_echo_suppress = (int)value;
    value = get_value_by_file("/sdcard/denoise_ctrl.txt");
    if(value != (double)(-1.0)) noise_ctrl = (int)value;
    value = get_value_by_file("/sdcard/noise_suppress.txt");
    if(value != (double)(-1.0)) noise_suppress = (int)value;
    value = get_value_by_file("/sdcard/agc_ctrl.txt");
    if(value != (double)(-1.0)) agc_ctrl = (int)value;
    value = get_value_by_file("/sdcard/agc_level.txt");
    if(value != (double)(-1.0)) agc_level = (int)value;
    value = get_value_by_file("/sdcard/agc_inc.txt");
    if(value != (double)(-1.0)) agc_inc = (int)value;
    value = get_value_by_file("/sdcard/agc_dec.txt");
    if(value != (double)(-1.0)) agc_dec = (int)value;
    value = get_value_by_file("/sdcard/agc_target.txt");
    if(value != (double)(-1.0)) agc_target = (int)value;

    /* 
        set Speexdsp preprocess
    */
    tail_length = sample_rate * tail / 1000; // for different sampling rate
	echo_state = speex_echo_state_init(frame_unit, tail_length);
	if(!echo_state)
		return -1;

	speex_pre_state = speex_preprocess_state_init(frame_unit, sample_rate);
	if(!speex_pre_state)
		return -1;

	speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &sample_rate);

    // echo cancellation
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &aec_echo_suppress);
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &aec_echo_suppress);

    // denoise
    speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_DENOISE, &noise_ctrl);
    if(noise_ctrl) {
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);
    }

    // auto gain control (*denoising needs to be enabled before using AGC)
    if((noise_ctrl == 0) && agc_ctrl) {
        agc_ctrl = 0;
        printf("<warning> denoising needs to be enabled before using AGC");
    }
    speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC, &agc_ctrl);
    if(agc_ctrl) {
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &agc_inc);
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &agc_dec);
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_TARGET, &agc_target);
    }

    printf("=> product:%s, rate:%d, delay:%d, tail:%dmsec, echo:%d, mic-gain:%1.2f, size:%d\n",
           product_name_string, sample_rate, delay, tail, aec_echo_suppress, g_mic_gain, frame_unit);
}

#if 0
int init_speex(int clock_rate, int tail_ms, int echo_suppress)
{
    int vad = 0;
    int vadProbStart = 80; //35; // silence to voice
    int vadProbContinue = 70; //20; // stay in the voice state

    int denoise = 0;
    int noiseSuppress = -10;

    int aec_spx_agc_enable = 1;
    int agc_increment = 3;
    int agc_decrement = 3;
    
    int tail_length, ret = 0;

    tail_length = clock_rate * tail_ms / 1000; // for different sampling rate
	echo_state = speex_echo_state_init(frame_unit, tail_length);
	if(!echo_state)
		return -1;

	speex_pre_state = speex_preprocess_state_init(frame_unit, clock_rate);
	if(!speex_pre_state)
		return -1;

	speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &clock_rate);

    // Echo cancellation
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &echo_suppress);
	speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &echo_suppress);

#if 0
    // VAD
	ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_VAD, &vad);
    printf("SPEEX_PREPROCESS_SET_VAD: %d (%d)\n", ret, vad);
    if(vad) {
        ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_PROB_START, &vadProbStart);  // Set probability required for the VAD to go from silence to voice
        printf("SPEEX_PREPROCESS_SET_PROB_START: %d\n", ret);
        ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); // Set probability required for the VAD to stay in the voice state (integer percent)
        printf("SPEEX_PREPROCESS_SET_PROB_CONTINUE: %d\n", ret);
    }
#endif


    // AGC && Denoise 
    {
#if 0
        ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
        printf("SPEEX_PREPROCESS_SET_DENOISE: %d (%d)\n", ret, denoise);
        if(denoise) {
            ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);
            printf("SPEEX_PREPROCESS_SET_NOISE_SUPPRESS: %d\n", ret);
        }

	    ret = speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC, &aec_spx_agc_enable); // set AGC
        printf("SPEEX_PREPROCESS_SET_AGC: %d (%d)\n", ret, aec_spx_agc_enable);
        if(aec_spx_agc_enable) {
	        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &agc_increment);
	        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &agc_decrement);
            //speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
        }
#else
        int value = 0;

        value = get_value_by_file("/sdcard/denoise_ctrl.txt");
        if(value < 0) value = 1;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_DENOISE, &value);
        value = get_value_by_file("/sdcard/noise_suppress.txt");
        if(value < 0) value = -25;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &value);

        value = get_value_by_file("/sdcard/agc_ctrl.txt");
        if(value < 0) value = 1;        
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC, &value);
        float agc_level = (float)get_value_by_file("/sdcard/agc_level.txt");
        if(agc_level < 0) agc_level = (float)8000.0;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
        value = get_value_by_file("/sdcard/agc_inc.txt");
        if(value < 0) value = 15;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &value);
        value = get_value_by_file("/sdcard/agc_dec.txt");
        if(value < 0) value = -15;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &value);
        value = get_value_by_file("/sdcard/agc_target.txt");
        if(value < 0) value = 24000;
        speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_TARGET, &value);
        
        
#endif
    }

	return 0;
}
#endif


/* enter to USB-MIC source */
int sw_aec_init(int sample_rate)
{
    //int echo_suppress = 0;

    init_global_var();    

#if 0
    rate = sample_rate;
    if(rate == 16000) frame_unit = 320;
	else frame_unit = 160;

    g_mic_gain = (double)get_value_by_file("/sdcard/mic_gain.txt");
    if(g_mic_gain < 0.0) g_mic_gain = 1.0;

    tail = get_value_by_file("/sdcard/aec_tail.txt");
    if(tail < 0) {
        tail = 300; /* msec */
    }

    delay = get_value_by_file("/sdcard/aec_delay.txt");
    if(delay < 0) {
	    if(strstr(product_name_string, "fg6") != NULL) {
		    delay = 5;
	    }
        else if(strstr(product_name_string, "qf3") != NULL) {
            if(rate == 8000) delay = 6;
            else delay = 13;//12;
	    }
        /* QF3a */
        else if(strstr(product_name_string, "QT-195W-aa2") != NULL) {
            if(rate == 8000) delay = 3;
            else delay = 6;
        }
        /* QF5 */
        else if(strstr(product_name_string, "rk3399_mid_qf5") != NULL) {
            if(rate == 8000) delay = 3;
            else delay = 6;
        }
        /* default */   
	    else {
		    delay = 8/(16000/rate);
	    }
    }

    echo_suppress = get_value_by_file("/sdcard/aec_echo.txt");
    if(echo_suppress < 0) {
        if(strstr(product_name_string, "qf3") != NULL) {
            echo_suppress = -80;
        }
        /* QF3a */
        else if(strstr(product_name_string, "QT-195W-aa2") != NULL) {
            echo_suppress = -30;
        }
        /* QF5 */
        else if(strstr(product_name_string, "rk3399_mid_qf5") != NULL) {
            echo_suppress = -75;
        }
        /* default */
        else {
	        echo_suppress = -75;
        }
    }

	if(init_speex(tail, rate, echo_suppress) < 0) {
		printf("<error> Speex init failed\n");
		return -1;
	}
    printf("=> product:%s, rate:%d, delay:%d, tail:%dmsec, echo:%d, size:%d\n", product_name_string, sample_rate, delay, tail, echo_suppress, frame_unit);
#else
    if(init_speex(sample_rate) < 0) {
		printf("<error> Speex initial failed\n");
		return -1;
	}
#endif

	aec_fifo = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t)*FIFO_SIZE);
	if(!aec_fifo) {
		printf("<error> AEC FIFO allocte fail!!!!!!!\n");
		return -1;
	}

	oBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	iBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	xBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));

	if(!oBuf) {
		printf("<error> Buf allocte failed\n");
		return -1;
	}

	if(!iBuf) {
		free(oBuf);
		printf("<error> Buf allocte failed\n");
		return -1;
	}

	if(!xBuf) {
		free(oBuf);
		free(iBuf);
		printf("<error> Buf allocte failed\n");
		return -1;
	}

	memset(oBuf,0x0,frame_unit*sizeof(spx_int16_t));
	memset(iBuf,0x0,frame_unit*sizeof(spx_int16_t));
	memset(xBuf,0x0,frame_unit*sizeof(spx_int16_t));

	memset(aec_fifo,0x0,frame_unit*sizeof(spx_int16_t)*FIFO_SIZE);

	printf("=> %s(), init OK\n", __FUNCTION__);

	flag_init = 1;

	return 0; // 0:sucess, -1:fail
}


/* hook at playout side */
int sw_aec_push(char *pcm, int len)
{
	int i;
    short *pPCM = NULL;

	if(flag_init != 1) {
	    return 0;
	}

    if(len > frame_unit*2) {
        printf("%s(), invalid length %d\n", __FUNCTION__, len);
        return 0;
    }

	pthread_mutex_lock(&p_aec_mutex);

    if (aec_fifo) {
		memcpy(aec_fifo + write_pos*frame_unit, pcm, len);
	} else {
		return -1;
	}

    //printf("w:%d\n", write_pos);
	write_pos++;
	write_pos %= FIFO_SIZE;

    if(g_record_pcm == 1)
        sw_aec_write_file("/sdcard/playout.pcm", (char *)oBuf, len, 1);

	pthread_mutex_unlock(&p_aec_mutex);

	return 0; // 0:sucess, -1:fail
}


/* hook at record side */
int sw_aec_pop(char *pcm, int len)
{
	int ret = 0, i, diff;
	unsigned int t;
    static int pre_w = 0;

	if(flag_init != 1){
	    return ret;
	}

	pthread_mutex_lock(&p_aec_mutex);

    diff = (write_pos>=read_pos)?write_pos-read_pos:FIFO_SIZE+write_pos-read_pos;
    if((pre_w != write_pos) && ((diff > delay+4)||(diff < delay))) {
        printf("=> [cur] r:%d, w:%d, diff:%d\n", read_pos, write_pos, diff);
        read_pos = (write_pos>=delay)?write_pos-delay:FIFO_SIZE-delay+write_pos; //write_pos - delay;
        read_pos %= FIFO_SIZE;

        diff = (write_pos>=read_pos)?write_pos-read_pos:FIFO_SIZE+write_pos-read_pos;
        printf("=> [new] r:%d, w:%d, diff:%d\n", read_pos, write_pos, diff);

        pre_w = write_pos;
    }

    //printf("r:%d, w:%d (diff:%d)\n", read_pos, write_pos, (write_pos>read_pos)?write_pos-read_pos:FIFO_SIZE+write_pos-read_pos);
	if(echo_state && aec_fifo) {
		speex_echo_cancellation(echo_state, (spx_int16_t *)pcm, aec_fifo + read_pos*frame_unit, oBuf);
#if 0
        if(speex_pre_state) {

            if(speex_preprocess_run(speex_pre_state, oBuf) == 0) {
                memset(oBuf, 0x0, len);
                printf("VAD event\n");
            }

        }
#else
        speex_preprocess_run(speex_pre_state, oBuf);
#endif
        // Frank debug ++
        if(g_record_pcm == 1)
            sw_aec_write_file("/sdcard/mic1.pcm", (char *)oBuf, len, 1);
        // Frank debug --

        for(i=0;i<frame_unit;i++) {
            double sample = (double)oBuf[i] * g_mic_gain;
            if (sample > 32767.0) {sample = 32767.0;}
            if (sample < -32768.0) {sample = -32768.0;}
            oBuf[i] = (spx_int16_t)sample;
        }
        memcpy(pcm, oBuf,len);

        // Frank debug ++
        if(g_record_pcm == 1)
            sw_aec_write_file("/sdcard/mic2.pcm", (char *)oBuf, len, 1);
        // Frank debug --
	}
	else {
	    ret = -1;
	}

    read_pos++;
    read_pos %= FIFO_SIZE;

    pthread_mutex_unlock(&p_aec_mutex);
    


	return ret; // 0:sucess, -1:fail
}

void exit_speex(void) {
	speex_echo_state_destroy(echo_state);
	speex_preprocess_state_destroy(speex_pre_state);
	echo_state = NULL;
	speex_pre_state = NULL;	
}

/* exit from USB-MIC source */
int sw_aec_exit(void)
{
	int i,total = 0;

	flag_init = 0;

    printf("%s(), enter\n", __FUNCTION__);

	free(oBuf);
	oBuf = NULL;
	free(iBuf);
	iBuf = NULL;
	free(xBuf);
	xBuf = NULL;
	free(aec_fifo);
	aec_fifo = NULL;

	pthread_mutex_destroy(&p_aec_mutex);

	oBuf = iBuf = xBuf = aec_fifo  = NULL;
	exit_speex();

    printf("%s(), exit\n", __FUNCTION__);

    return 0; // 0:sucess, -1:fail
}


