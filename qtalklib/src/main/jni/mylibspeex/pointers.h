#include <jni.h>
#include <string.h>
#include <stdio.h>
#include "logging.h"
#include <pthread.h>
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"
#define LOG_TAG "kakasi"
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , "LOG_DEBUG",__VA_ARGS__)
#define frame_size 320
#define filter_length 640
typedef struct _TxMessage
{
	short audio_data[frame_size];
	int data_len;
	
	int timestamp;
    struct _TxMessage *nextTxMessage;
} TxMsg;

typedef struct _TxMsgQue
{
	int msgcnt;
    struct _TxMessage *head;
    struct _TxMessage *tail;
	pthread_mutex_t *tx_mutex;
	pthread_cond_t *tx_cond;
} TxMsgQue;

SpeexEchoState *echo_state;
SpeexPreprocessState *den;
int isStarted;
TxMsgQue *TxMsgQ;
int isAlive;
