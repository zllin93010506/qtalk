
/*=====================================================
 *   API(s) for Sotfware AEC    
 *
 *   by Tony
 *=====================================================
*/

#include "Sys_Software_AEC.h"
#include "webrtc_vad.h"
#include "vad_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

SpeexEchoState *echo_state;
SpeexPreprocessState *speex_pre_state;
spx_int16_t *oBuf;
spx_int16_t *iBuf;
spx_int16_t *xBuf;


#if AEC_SERVER_TX
extern int *aec_fifo;
#else
spx_int16_t *aec_fifo;
#endif

spx_int16_t *aec_sine_fifo;

VadInst *vadinst;

int flag;
int cmp;
int rate;
int echo_suppress;
int noise_suppress;
int enable;
int tail_length;

FILE *fd_out=NULL, *fd_time_push=NULL, *fd_time_pop=NULL, *fd_rec=NULL, *fd_play=NULL, *fd_delay=NULL, *fd_debug= NULL;
struct timeval pop_before, pop_after, push_before, push_after;

#if AEC_SERVER_TX
extern int read_pos;
extern int write_pos;
extern int frame_unit;
#else
int read_pos;
int write_pos;
int frame_unit;
#endif


int play_read;
int play_write;
int delay;
int tail;

int aec_dump_file;
int aec_sine_output;
int aec_cor_on_off;
int aec_alc_enable;
int aec_spx_agc_enable;

int FIFO_SIZE;


#define WIDE_BAND 16000
#define NARROW_BAND 8000
#define SPEEX_ENABLE 1
#define AEC_DUMP 1
#define AEC_DUMP_TIME 0
#define DELAY_FILE 1
#define TIME_INT 300

#define COR_LEN_SIZE 1280

#define MAX_VALUE   0x7FFF
#define ALC_HIGH_LVL -12
#define ALC_LOW_LVL	-35
#define ALC_NOISE_LVL -45
#define ALC_ATTACK_HIGH_CNT 4
#define ALC_ATTACK_LOW_CNT 7
#define ALC_RELEASE_HIGH_CNT 500
#define ALC_RELEASE_LOW_CNT 50
#define AVG_RMS_LEN 2
#define ALC_MIC_GAIN_LOCK_CNT 50

#define SHRT_MAX_VALUE 1


int cor_y_ptr=0, cor_e_ptr=0;
short *cor_e_buf, *cor_y_buf;
long long sum_cor_den = 0;
long long sum_cor_num = 0;

unsigned short analysis[TIME_INT+1] = {0};
unsigned short analysis_push[TIME_INT+1] = {0};

int ptr_rms = 0;
int alc_high_cnt = 0;
int alc_low_cnt = 0;
double sum_rms = 0;
int avg_rms_len = 5;
double rms[AVG_RMS_LEN];
int time_cnt = 0;
int b_alc_gain_lock = 0;
int b_alc_mic_gain_change_lock = 0;
int volume_cnt = 0;
int alc_lock_cnt = 0;


spx_int16_t sine_1k_array[] ={ 
0,3149,5820,7604,8231,7604,5820,3150,
-1,-3149,-5821,-7604,-8231,-7604,-5819,-3149
};

int init_speex(void)
{
	echo_suppress = -60;
	noise_suppress = -45;
	enable = 1;


	FIFO_SIZE = delay + 2;
	tail_length = frame_unit * tail;

	read_pos = 0;
	write_pos = read_pos + delay;


	echo_state = speex_echo_state_init(frame_unit,tail_length);
	if(!echo_state)
		return -1;

	speex_pre_state = speex_preprocess_state_init(frame_unit,rate);
	if(!speex_pre_state)
		return -1;

	speex_echo_ctl(echo_state,SPEEX_ECHO_SET_SAMPLING_RATE,&rate);

	speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_ECHO_STATE,echo_state);

	speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_ECHO_SUPPRESS,&echo_suppress);

	speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE,&echo_suppress);

	speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,&noise_suppress);

	speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_VAD,&enable);

	if (aec_spx_agc_enable){	
		speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC, &aec_spx_agc_enable); // set AGC
		//speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &agc_increment); // 
		//speex_preprocess_ctl(speex_pre_state, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &agc_decrement); // 
	}
	//speex_preprocess_ctl(speex_pre_state,SPEEX_PREPROCESS_SET_DEREVERB,&enable);

	return 0;
}




/* enter to USB-MIC source */
int sw_aec_init(int sample_rate)
{
	int i = 0;
	rate = sample_rate;

	flag = 0;
	cmp = 1;

	printf("%s(), sample_rate:%d\n", __FUNCTION__, sample_rate);

	delay = 9;
	tail = 11;
	aec_dump_file = 0;
	aec_sine_output = 0;
	aec_cor_on_off = 1;
	aec_alc_enable = 0;
	aec_spx_agc_enable = 0;

#if DELAY_FILE
	fd_delay = fopen("/tmp/delay", "r");
	if(!fd_delay)
		printf("Read AEC delay file failed!\n");
	else{
		fscanf(fd_delay,"%d\n%d\n%d\n%d\n%d",&delay, &tail, &aec_dump_file, &aec_alc_enable, &aec_spx_agc_enable);
	}
#endif
	printf("%s(), SPEEX FIFO delay:%d tail:%d aec_dump_file enable :%d frame unit:%d\n", __FUNCTION__, delay, tail, aec_dump_file, frame_unit);

#if AEC_DUMP
	if (aec_dump_file)
	{
		fd_out = fopen("/tmp/out.pcm", "w+");
		if(!fd_out)
			printf("Dump file open fail\n");

		fd_rec = fopen("/tmp/rec.pcm", "w+");
		if(!fd_rec)
			printf("Dump file open fail\n");

		fd_play = fopen("/tmp/play.pcm", "w+");
		if(!fd_play)
			printf("Dump file open fail\n");

		fd_debug = fopen("/tmp/debug", "w+");
		if(!fd_debug)
			printf("Dump file open fail\n");
	}
#endif

#if AEC_DUMP_TIME
	fd_time_pop = fopen("/tmp/time_pop", "w+");
	if(!fd_time_pop)
		printf("Dump time failed\n");

	fd_time_push = fopen("/tmp/time_push", "w+");
	if(!fd_time_push)
		printf("Dump time failed\n");
#endif

	if(sample_rate == WIDE_BAND)
		frame_unit = 320;
	else
		frame_unit = 160;

	if(init_speex()<0){
		printf("Speex init failed\n");
		return -1;
	}
	else{
		printf("Speex init\n");
	}

	aec_fifo = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t)*FIFO_SIZE);
	if(!aec_fifo){
		printf("AEC FIFO allocte fail!!!!!!!\n");
		return -1;
	}

	oBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	iBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	xBuf = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	
	if(aec_sine_output){
		aec_sine_fifo = (spx_int16_t *)malloc(frame_unit*sizeof(spx_int16_t));
	}

	if(!oBuf){
		printf("Buf allocte failed\n");
		return -1;
	}
	if(!iBuf)
	{	
		free(oBuf);
		printf("Buf allocte failed\n");
		return -1;
	}

	if(!xBuf)
	{	
		free(oBuf);
		free(iBuf);
		printf("Buf allocte failed\n");
		return -1;
	}

	if(!aec_sine_fifo && aec_sine_output)
	{
		free(oBuf);
		free(iBuf);
		free(xBuf);
		printf("sine test Buf allocte failed\n");	
		return -1;
	}

	memset(oBuf,0x0,frame_unit*sizeof(spx_int16_t));
	memset(iBuf,0x0,frame_unit*sizeof(spx_int16_t));
	memset(xBuf,0x0,frame_unit*sizeof(spx_int16_t));

	memset(aec_fifo,0x0,frame_unit*sizeof(spx_int16_t)*FIFO_SIZE);

	if(aec_sine_output){
		memset(aec_sine_fifo,0x0,frame_unit*sizeof(spx_int16_t));
	}

	memset(&pop_before, 0x0, sizeof(pop_before));
	memset(&pop_after, 0x0, sizeof(pop_before));
	memset(&push_before, 0x0, sizeof(pop_before));
	memset(&push_after, 0x0, sizeof(pop_before));

	if(aec_sine_output && aec_sine_fifo){
		for(i=0; i<20; i++){
			memcpy(aec_sine_fifo, sine_1k_array, 16);
			aec_sine_fifo += 16;
		}	
		aec_sine_fifo -= 320;
	}
	
	if(aec_cor_on_off){	
			cor_y_buf = (short*)malloc(sizeof(short)*COR_LEN_SIZE);
    		cor_e_buf = (short*)malloc(sizeof(short)*COR_LEN_SIZE);
    		memset(cor_y_buf, 0x00, sizeof(short)*COR_LEN_SIZE);
    		memset(cor_e_buf, 0x00, sizeof(short)*COR_LEN_SIZE);
	}

	if(aec_alc_enable){
		if(sw_aec_InitVad() == -1)
			printf("Init VAD fail\n");
	} 

	printf("%s(), Init OK\n", __FUNCTION__);

	return 0; // 0:sucess, -1:fail
}


/* hook at play side */
int sw_aec_push(char *pcm, int len)
{
	int ret;
	unsigned int t;

#if AEC_DUMP_TIME
	gettimeofday(&push_after, NULL);
#endif

#if SPEEX_ENABLE
	if((write_pos+1)%FIFO_SIZE == read_pos){
		read_pos++;
		read_pos %= FIFO_SIZE;
		//		fprintf(fd_debug, "%s(%d)\n", __FUNCTION__, read_pos);
	}

	memcpy(aec_fifo + write_pos*frame_unit, pcm, len);


	write_pos++;
	write_pos %= FIFO_SIZE;
#endif

#if AEC_DUMP_TIME

	if(push_after.tv_usec < push_before.tv_usec)
		t = 1000000 - push_before.tv_usec + push_after.tv_usec;
	else
		t = push_after.tv_usec - push_before.tv_usec;

	analysis_push[t/1000]++;

	push_before.tv_usec = push_after.tv_usec;
#endif

	//	playout_fifo(pcm, len);

	return 0; // 0:sucess, -1:fail
}

/* hook at recoder side */
int sw_aec_pop(char *pcm, int len)
{
	int ret;
	unsigned int t;

	//	memset(oBuf, 0x0, len);
	//	memset(iBuf, 0x0, len);
	//	memcpy(iBuf, pcm, len);

#if AEC_DUMP_TIME
	gettimeofday(&pop_after, NULL);
#endif

#if SPEEX_ENABLE

	if((write_pos - read_pos + FIFO_SIZE) % FIFO_SIZE < (delay - 4)){
		read_pos = (read_pos -1 + FIFO_SIZE) % FIFO_SIZE;
		//		fprintf(fd_debug, "%s(read:%d wirte:%d)\n",__FUNCTION__,read_pos,write_pos);
	}

#if AEC_DUMP
	if(aec_dump_file){
		if(fd_rec && fd_play){
			ret = fwrite(pcm, len, 1, fd_rec);
			ret = fwrite(aec_fifo + read_pos*frame_unit, len, 1, fd_play);
			if(ret != 1)
				printf("Write data lost! only write %d\n",ret);
		}
	}
#endif

#if AEC_SERVER_TX

#elif AEC_SERVER_RX	
	if(echo_state){		

		speex_echo_cancellation(echo_state, pcm, pcm + frame_unit, oBuf);
		//memcpy(pcm, oBuf, len);
		if(aec_cor_on_off)
			sw_aec_correlation(pcm, oBuf, frame_unit);
		

		if(!flag){
			if(speex_pre_state){
				speex_preprocess_run(speex_pre_state, oBuf);
				}
			else 
				return -1;

	
		if(aec_alc_enable){
				sw_aec_ALC(oBuf, frame_unit);
		 }

			if(cmp){
				if(!memcmp(iBuf,oBuf,len)){
					exit_speex();
					init_speex();
					//			flag = 1;
				}
				else{
					printf("Preprocessor run!\n");
					cmp = 0;
				}
			}


			if(aec_dump_file){
				if(fd_out){
					ret = fwrite(oBuf, len, 1, fd_out);
					if(ret != 1)
						printf("Write data lost! only write %d\n",ret);
				}
			}

			if(aec_sine_output)
				memcpy(pcm, aec_sine_fifo, len);
			else
				memcpy(pcm, oBuf,len);
		}
	}
	else 
		return -1;
#else
	if(echo_state){		

		speex_echo_cancellation(echo_state, pcm, aec_fifo + read_pos*frame_unit, oBuf);
		//memcpy(pcm, oBuf, len);
		if(aec_cor_on_off)
			sw_aec_correlation(pcm, oBuf, frame_unit);
		

		if(!flag){
			if(speex_pre_state){
				speex_preprocess_run(speex_pre_state, oBuf);
				}
			else 
				return -1;

	
		if(aec_alc_enable){
				sw_aec_ALC(oBuf, frame_unit);
		 }

			if(cmp){
				if(!memcmp(iBuf,oBuf,len)){
					exit_speex();
					init_speex();
					//			flag = 1;
				}
				else{
					printf("Preprocessor run!\n");
					cmp = 0;
				}
			}


			if(aec_dump_file){
				if(fd_out){
					ret = fwrite(oBuf, len, 1, fd_out);
					if(ret != 1)
						printf("Write data lost! only write %d\n",ret);
				}
			}

			if(aec_sine_output)
				memcpy(pcm, aec_sine_fifo, len);
			else
				memcpy(pcm, oBuf,len);
		}
	}
	else 
		return -1;
#endif

	read_pos++;
	read_pos %= FIFO_SIZE;
#endif

#if AEC_DUMP_TIME
	//	gettimeofday(&tv_after, NULL);

	if(pop_after.tv_usec < pop_before.tv_usec)
		t = 1000000 - pop_before.tv_usec + pop_after.tv_usec;
	else
		t = pop_after.tv_usec - pop_before.tv_usec;

	analysis[t/1000]++;

	pop_before.tv_usec = pop_after.tv_usec;
#endif



	return 0; // 0:sucess, -1:fail
}

void exit_speex(void){
	speex_echo_state_destroy(echo_state);
	speex_preprocess_state_destroy(speex_pre_state);
	echo_state = NULL;
	speex_pre_state = NULL;
}

/* exit from USB-MIC source */
int sw_aec_exit(void)
{	
	int i,total = 0;
	int percentage = 0;
	
    printf("%s(), enter\n", __FUNCTION__);

#if AEC_DUMP_TIME	
	for(i=1;i<TIME_INT+1;i++){
		total += analysis[i];
		//fprintf(fd_time, "%d ms: %u\n", i, analysis[i]);
	}
	for(i=1;i<TIME_INT+1;i++){
		fprintf(fd_time_pop, "%d ms: %u\t%d\n", i, analysis[i], total);
	}
	total = 0;
	for(i=1;i<TIME_INT+1;i++){
		total += analysis_push[i];
		//fprintf(fd_time, "%d ms: %u\n", i, analysis[i]);
	}
	for(i=1;i<TIME_INT+1;i++){
		fprintf(fd_time_push, "%d ms: %u\t%d\n", i, analysis_push[i], total);
	}

#endif

	free(oBuf);
	free(iBuf);
	free(xBuf);
	free(aec_fifo);	
	if(aec_cor_on_off){
		free(cor_y_buf);
		free(cor_e_buf);
	}

#if AEC_DUMP
	if(aec_dump_file){
		if(fd_out)
			fclose(fd_out);
		if(fd_time_pop)
			fclose(fd_time_pop);
		if(fd_time_push)
			fclose(fd_time_push);
		if(fd_rec)
			fclose(fd_rec);
		if(fd_play)
			fclose(fd_play);
	}
#endif

	if(aec_sine_output){
		free(aec_sine_fifo);
	}
		

#if DELAY_FILE
	if(fd_delay)
		fclose(fd_delay);
#endif

	if(aec_alc_enable){
		WebRtcVad_Free(vadinst);	
	}


	oBuf = iBuf = xBuf = aec_fifo  = NULL;
	exit_speex();

    printf("%s(), exit\n", __FUNCTION__);

    return 0; // 0:sucess, -1:fail
}


 void sw_aec_add_dB(int dB_value, int len, short *ptr_data, int limit)
 {
         int i=0;
         long tmp=0, tmp1=0, tmp2 = 0;
 
         switch (dB_value)
         {
         case 3:
                 for(i=0; i<len; i++)
                 {
                         tmp1    =       tmp     =       (long)(*ptr_data);
 
                         //if(tmp <= limit){
                                 tmp1    =       tmp1>>1;
                                 tmp     +=      tmp1;           // add 3dB
 
                         if(tmp > 0x7FFF)
                                 tmp     = 0x7FFF;
                         else if(tmp < -0x7FFF)
                                 tmp     = -0x7FFF;
                         //}
 
                         *ptr_data       =       (short)tmp;
                         ptr_data++;
                 }
         break;
 
         case 6:
                 for(i=0; i<len; i++)
                 {
                         tmp     =       (long)(*ptr_data); 
                       
                                 tmp     =      tmp<<1;          // add 6dB

                                if(tmp >= 32767)
                                        tmp     = 32767;
                                else if(tmp <= -32768)
                                        tmp     = -32768;
                        //}

                        *ptr_data       =       (short)tmp;
                        ptr_data++;
                }

        break;

         case -6:
                 for(i=0; i<len; i++)
                 {
                         tmp     =       (long)(*ptr_data);
                        
                                 tmp     =      tmp>>1;          // minus 6dB

                                if(tmp >= 32767)
                                        tmp     = 32767;
                                else if(tmp <= -32768);
                                        tmp     = -32768;          

                        *ptr_data       =       (short)tmp;
                        ptr_data++;
                }

        break;

        case -3:
                for(i=0; i<len;i++)
                {
                        tmp     =       (long)(*ptr_data);
                        tmp1    =       tmp>>1;

                        tmp     -=      tmp1;           // minus 3dB

                        if(tmp > 32767)
                                tmp     =       32767;

                        *ptr_data       =       (short)tmp;
                        ptr_data++;
                }
        break;

        default:
        break;
      }
}

void sw_aec_correlation(short *y, short *e, int len)
{
        int i=0;
        long long abs_sum_cor_num=0;

        double Rho = 0.0;

        for(i=0; i<len; i++){                        
                sum_cor_den                     -=      abs(((*(cor_e_buf+cor_e_ptr))*(*(cor_y_buf+cor_y_ptr))));
                sum_cor_num                     -=      ((*(cor_e_buf+cor_e_ptr))*(*(cor_y_buf+cor_y_ptr)));
                
                *(cor_e_buf + cor_e_ptr)        =       *e;
                *(cor_y_buf + cor_y_ptr)        =       *y;
                
                sum_cor_den                     +=      abs(((*(cor_e_buf+cor_e_ptr))*(*(cor_y_buf+cor_y_ptr))));
                sum_cor_num                     +=      ((*(cor_e_buf+cor_e_ptr))*(*(cor_y_buf+cor_y_ptr)));
                
                abs_sum_cor_num         =       sum_cor_num;
                if(abs_sum_cor_num < 0)
                        abs_sum_cor_num = -abs_sum_cor_num;
                
                Rho             =       ((double)abs_sum_cor_num/(double)sum_cor_den);
                
                cor_e_ptr++;    
                cor_e_ptr %= COR_LEN_SIZE;
                
                cor_y_ptr++; 
                cor_y_ptr %= COR_LEN_SIZE;
                
                *e              =       (short)((double)Rho*(double)(*e));
                
                e++;
                y++;
                
        }       
        return; 
}

double sw_aec_RMS(short *x, int len)
{
    int i =0;
    long long x_sum =0;
    double x_rms = 0;

    for(i=0; i<len; i++){
        x_sum += (*x)*(*x);
        x++;
    }

    x_rms = (double)x_sum/(double)len;

    x_rms = sqrt(x_rms);

	if(x_rms != 0)
		x_rms = 20*log10(x_rms/MAX_VALUE);
	else
		x_rms = -60;	// should consider more in detail, how much is noise level ???
    
	return x_rms;
}

void sw_aec_ALC(short *x, int len)
{

    double x_rms;

#if 0
	sum_rms -= rms[ptr_rms];
    rms[ptr_rms]= sw_aec_RMS(x, len);	
	sum_rms += rms[ptr_rms];
	x_rms	= sum_rms/AVG_RMS_LEN;
	ptr_rms++;
	ptr_rms %= AVG_RMS_LEN;
#endif

	x_rms = sw_aec_RMS(x, len);

	time_cnt++;
	if(time_cnt == 75){
		printf("rms=%fdB, highcnt=%d, lowcnt=%d, lock_flag=%d, lock_cnt=%d, volume_cnt=%d\n", x_rms, alc_high_cnt, alc_low_cnt, b_alc_gain_lock, alc_lock_cnt, volume_cnt);
		time_cnt = 0;
	}
	
	if(b_alc_gain_lock){
		if(alc_lock_cnt)
			alc_lock_cnt--;
		if(alc_lock_cnt == 0){
			b_alc_gain_lock = 0;
			printf("ALC unlock!!!!\n");	
		}
		//return;
	}
	else if(b_alc_mic_gain_change_lock){	// do not change mic gain too fast, must have to lock for a specific time period.
		if(alc_lock_cnt)
			alc_lock_cnt--;
		if(alc_lock_cnt == 0){
			b_alc_mic_gain_change_lock = 0;			
		}
		return;
	}

	if(!WebRtcVad_Process(vadinst, rate, x, len)){
		return;
	}


   if(x_rms >= ALC_HIGH_LVL)
        alc_high_cnt++;
   else if (x_rms <= ALC_LOW_LVL && x_rms >= ALC_NOISE_LVL){
        alc_low_cnt++;
   }
   else{
       alc_high_cnt = 0;
       alc_low_cnt = 0;
   }
   
	if(alc_high_cnt > ALC_ATTACK_HIGH_CNT){
		alc_high_cnt = 0;
		if(volume_cnt < 3)
			volume_cnt++;

		if(!b_alc_gain_lock){
			b_alc_mic_gain_change_lock = 1;
			alc_lock_cnt = ALC_MIC_GAIN_LOCK_CNT;
			sw_aec_ALC_mic_ctrl(x_rms);
		}
	}
	else if(alc_low_cnt > ALC_ATTACK_LOW_CNT){
		alc_low_cnt = 0;
		if(volume_cnt > -2)
			volume_cnt--;
		
		if(!b_alc_gain_lock){
			b_alc_mic_gain_change_lock = 1;
			alc_lock_cnt = ALC_MIC_GAIN_LOCK_CNT;
			sw_aec_ALC_mic_ctrl(x_rms);
		}
	}

	return;
}

void sw_aec_ALC_mic_ctrl(double x_rms)
{

	switch(volume_cnt){
	case 3:				
			system("amixer -c 3 cset numid=2 15");
			b_alc_gain_lock = 1;
			alc_lock_cnt = ALC_RELEASE_HIGH_CNT;
			printf("\n\nALC lock for 10 secs, gain set to 15dB\n\n");
	break;

	case 2:		
			system("amixer -c 3 cset numid=2 22");
			printf("microphone gain decrease to 22dB!!, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);	
	break;

	case 1:
			system("amixer -c 3 cset numid=2 26");
			printf("microphone gain decrease to 26dB!!, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);
	break;

	case 0:
			system("amixer -c 3 cset numid=2 30");
			printf("microphone gain set to normal 30dB, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);
	break;

	case -1:
			system("amixer -c 3 cset numid=2 33");
			printf("microphone gain increase to 33dB!!, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);
	
	break;

	case -2:
			system("amixer -c 3 cset numid=2 36");
			printf("microphone gain increase to 36dB!!, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);

	break;

	case -3:
			system("amixer -c 3 cset numid=2 39");
			printf("microphone gain increase to 39dB!!, rms=%f, volume_cnt=%d\n", x_rms, volume_cnt);
	break;

	default:
	break;
	}

   return ;
}  
/*
bool NoiseDetect()
{
	
}
*/

// VAD source code , from google code website.
int sw_aec_InitVad()
{
	
	if(WebRtcVad_Create(&vadinst))
	{
		printf("vad create error\n");
		return -1;
	}
	if(WebRtcVad_Init(vadinst))
	{
		printf("vad init error\n");
		return -1;
	}
	if(WebRtcVad_set_mode(vadinst, 3)) // mode:3, very aggressive mode
	{
		printf("set vad mode error\n");
		return -1;
	}
	else
		printf("VAD in aggressive mode\n");

	return 0;
}

