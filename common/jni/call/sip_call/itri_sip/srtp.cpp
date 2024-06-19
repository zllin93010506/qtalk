#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#ifdef _ANDROID_APP
#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
//#define printf LOGW
#endif

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif

#include "callmanager.h"
#include "srtp.h"

int QuSessMediaGetCrypto(pSessMedia media,const char* attrib,const char* attrvalue){

	UINT32 len;
	int size;
	int pos;
	char *retcrypto=NULL;

	size=dxLstGetSize(media->cryptolist);

	printf("size=%d\n",size);
	for(pos=0;pos<size;pos++){

		retcrypto=(char *)dxLstPeek(media->cryptolist,pos);
		printf("name=%s\n",retcrypto);
	}
	return -1;
}

int QuSessMediaGetCryptoSize(pSessMedia media){

	int size;
	size=dxLstGetSize(media->cryptolist);

	return size;
}

static void QuGetInlineKey(call::SRTP_KEY_PARAMETER *srtp_key ,char* srtpbuf){
	char *str_buf = strdup(srtpbuf);

	strcpy(srtp_key->key_salt,(str_buf+7));
	printf("key_salt=%s\n",srtp_key->key_salt);
}

static void QuGetInlineLifetime(call::SRTP_KEY_PARAMETER *srtp_key ,char* srtpbuf){
	char * pch;
	int base_value = 0;
	int exponent_value = 0;

	char *str_buf = strdup(srtpbuf);

	pch = strtok (str_buf,"^");
	base_value = atoi(pch);
	pch = strtok (NULL,"^");
	exponent_value = atoi(pch);

	srtp_key->lifetime = 1048576;//pow (base_value,exponent_value);
	printf("lifetime=%lu\n",srtp_key->lifetime);
}

static void QuGetInlineMKI(call::SRTP_KEY_PARAMETER *srtp_key ,char* srtpbuf){
	char * pch;
	int mki_value = 0;
	int mki_length = 0;
	char *str_buf = strdup(srtpbuf);

	pch = strtok (str_buf,":");
	mki_value = atoi(pch);
	pch = strtok (NULL,":");
	mki_length = atoi(pch);

	srtp_key->mki.mki_value = mki_value;
	srtp_key->mki.mki_length = mki_length;

	printf("mki_value=%d,mki_length=%d\n",srtp_key->mki.mki_value,srtp_key->mki.mki_length);
}


void QuGetSRTPKey(call::SRTP_KEY_PARAMETER *srtp_key , char* srtpbuf){
	char * pch;
	char *key_str = NULL;
	char *lifetime_str = NULL;
	char *mki_str = NULL;

	int token_counter = 0;

	pch = strtok (srtpbuf," ");


	if (!strcmp(pch,"AES_CM_128_HMAC_SHA1_80")){
		srtp_key->crypto_suit = AES_CM_128_HMAC_SHA1_80;
	}else if (!strcmp(pch,"AES_CM_128_HMAC_SHA1_32")){
		srtp_key->crypto_suit = AES_CM_128_HMAC_SHA1_32;
	}else if (!strcmp(pch,"F8_128_HMAC_SHA1_80")){
		srtp_key->crypto_suit = F8_128_HMAC_SHA1_80;
	}else{
		srtp_key->crypto_suit = -1 ;
	}

	while (pch != NULL)
	{
		pch = strtok (NULL," |");

		if (pch !=NULL){
			//dispatcher srtp key parameter rfc4568
			switch (token_counter){
				case 0:
					key_str = strdup(pch);
					break;
				case 1:
					lifetime_str = strdup(pch);
					break;
				case 2:
					mki_str = strdup(pch);
					break;
				default:
				    break;
			}
		}

		token_counter++;

	}

	if (key_str)
		QuGetInlineKey(srtp_key,key_str);

	if (lifetime_str)
		QuGetInlineLifetime(srtp_key,lifetime_str);

	if (mki_str)
		QuGetInlineMKI(srtp_key,mki_str);
}

void QuGetLocalMediaSRTPInfo(pSess sess,MediaCodecInfo *media_info){
	int media_size;
	int i;
	MediaType media_type;
	pSessMedia media=NULL;
	char cryptobuf[256];
	UINT32 cryptolen = 255;

	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);

		if (media_type==MEDIA_AUDIO){

			if (RC_OK == SessMediaGetCrypto(media , 0 , cryptobuf , &cryptolen)){
				QuGetSRTPKey(&(media_info->audio_srtp.self_srtp_key) , cryptobuf);
			}
		}else if(media_type==MEDIA_VIDEO) {
			if (RC_OK == SessMediaGetCrypto(media , 0 , cryptobuf , &cryptolen)){
				QuGetSRTPKey(&(media_info->video_srtp.self_srtp_key) , cryptobuf);
			}
		}
	}
}

void QuGetRemoteMediaSRTPInfo(pSess sess,MediaCodecInfo *media_info){
	int media_size;
	int i;
	MediaType media_type;
	pSessMedia media=NULL;
	char cryptobuf[256];
	UINT32 cryptolen = 255;

	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);

		if (media_type==MEDIA_AUDIO){
            
			if (RC_OK == SessMediaGetCrypto(media , 0 , cryptobuf , &cryptolen)){
				QuGetSRTPKey(&(media_info->audio_srtp.opposite_srtp_key) , cryptobuf);
			}
		}else if(media_type==MEDIA_VIDEO) {
			if (RC_OK == SessMediaGetCrypto(media , 0 , cryptobuf , &cryptolen)){
				QuGetSRTPKey(&(media_info->video_srtp.opposite_srtp_key) , cryptobuf);
			}
		}
	}

}
