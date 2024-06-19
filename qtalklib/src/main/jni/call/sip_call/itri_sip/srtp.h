#ifndef SRTP_H
#define SRTP_H

//#include <stdbool.h>
//#include <callcontrol/callctrl.h>
//#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
//#include <sdp/sdp.h>
#include "media_info.h"

#define CALLER_AUDIO_SRTP_KEY "A1EEC3717DA76195BB878578790AF71C4EE9F859"
#define CALLEE_AUDIO_SRTP_KEY "B1EEC3717DA76195BB878578790AF71C4EE9F859"
#define CALLER_VIDEO_SRTP_KEY "C1EEC3717DA76195BB878578790AF71C4EE9F859"
#define CALLEE_VIDEO_SRTP_KEY "D1EEC3717DA76195BB878578790AF71C4EE9F859"

typedef enum {
	AES_CM_128_HMAC_SHA1_80 = 1,
	AES_CM_128_HMAC_SHA1_32 ,
	F8_128_HMAC_SHA1_80

}SRTPCryptoSuit;

int QuSessMediaGetCrypto(pSessMedia media,const char* attrib,const char* attrvalue);
int QuSessMediaGetCryptoSize(pSessMedia media);
void QuGetLocalMediaSRTPInfo(pSess sess,MediaCodecInfo *media_info);
void QuGetRemoteMediaSRTPInfo(pSess sess,MediaCodecInfo *media_info);

#endif
