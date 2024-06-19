/*
Quanta QRI Copyright (C) 2009 

This is VC Application Software Header file.
*/

#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H
#include <stdio.h>
//#include <stdbool.h>

#include <SessAPI/sessapi.h>
#include "srtp.h"

typedef enum {
  LEVEL_1_0=10,
  LEVEL_1_1,
  LEVEL_1_2,
  LEVEL_1_3,
  LEVEL_2_0=20,
  LEVEL_2_1,
  LEVEL_2_2,
  LEVEL_3_0=30,
  LEVEL_3_1,
  LEVEL_3_2,
  LEVEL_4_0=40,
  LEVEL_4_1,
  LEVEL_4_2,
  LEVEL_5_0=50,
  LEVEL_5_1,
}QuantaLevelIdType;
//RFC3984

typedef struct _QStreamPara
{
	int session;
	int isquanta;
	char remote_a_ip[32];
	char remote_v_ip[32];
	int laudio_port; 
	int lvideo_port; 
	int raudio_port; 
	int rvideo_port; 
	int audio_codec;
	char audio_payload[16];
	int audio_sample_rate;
	int audio_bit_rate;
	int video_codec;
	char video_payload[16];
	
	int mode;
	
	int dtmf_pt;

	int dtmf_transmit_mode;
	char carrier_network[32];
	int multiple_way_video;
	
	/*0:dect 1:smartphone*/
	int audio_source;
	char phone[64];
	
	/*2011/10/24 media bandwith*/
	int media_bandwidth;
	QuH264Parameter h264_parameter;
	
	int RS_bps;     // (unit: bps) RTCP bandwidth allocated to active data senders
	int RR_bps;     // (unit: bps) RTCP bandwidth allocated to active data receivers
	RTCP_FB rtcp_fb;
	
	int sip_idr_info_support;
    
    call::FEC_Control_t audio_fec_ctrl;
    call::FEC_Control_t video_fec_ctrl;

    call::MEDIA_SRTP_KEY_PARAMETER audio_srtp;
    call::MEDIA_SRTP_KEY_PARAMETER video_srtp;

} QStreamPara;


typedef struct _QCommandQueue
{
	int command_type;
	QStreamPara media_para;
	struct _QCommandQueue *next;
}QCommandQueue;

void *QuStreamInitEXE(void *arg);
//int QuStreamTerminate(int session);
void *QuStreamTerminateEXE(void *arg);
//int QuStreamHold(int session);
void *QuStreamHoldEXE(void *arg);
//int QuStreamResume(int session, char *remoteip, int laudio_port, int lvideo_port, int raudio_port, int rvideo_port, int audio_codec,char* audio_payload, int video_codec,char* video_payload);
void *QuStreamResumeEXE(void *arg);
void *QuStreamResumeAndConfOffEXE(void *arg);
//int QuStreamSwitchCallMode(int session, int mode, char *remoteip, int laudio_port, int lvideo_port, int audio_port, int video_port, int audio_codec,char* audio_payload, int video_codec,char* video_payload);
void *QuStreamSwitchCallModeEXE(void *arg);
void *QuStreamSwitchResolutionEXE(void *arg);
void *QuStreamSwitchResolutionAndConfOffEXE(void *arg);

void *QuStreamVideoConferenceHoldEXE(void *arg);
void *QuStreamConferenceInitEXE(void *arg);
void *QuStreamVideoConferenceModeStartEXE(void *arg);
void *QuStreamVideoConferenceModeStopEXE(void *arg);
	
int QuGetRemoteMediaInfo(pSess remotesess ,char *remote_a_ip,char *remote_v_ip,int *audio_port,int *video_port);
int QuGetMediaCodecInfo(pSess sess,MediaCodecInfo *media_info);
//int QuGetMediaCodecInfo(pSess sess ,int *audio_type ,char*audio_payload,int *audio_sample_rate,int *audio_bit_rate, int *video_type,char *video_payload,int *dtmf_type,QuH264Parameter *h264_parameter);

void *QuStreamPreInviteEXE(void *arg);
void *QuStreamAbortPreInviteEXE(void *arg);
void *QuStreamRejectPreInviteEXE(void *arg);

/*
void set_media_para(QStreamPara *media ,int session,int isquanta,char *remote_a_ip,char *remote_v_ip,int laudio_port,int lvideo_port,int raudio_port,int rvideo_port, 
					int audio_codec,char* audio_payload,int audio_sample_rate,int audio_bit_rate,int video_codec,char* video_payload,int mode,int dtmf_pt,QuH264Parameter *h264_parameter,int multiple_way_video);
*/
void set_media_para(QStreamPara *media ,int session,int isquanta,char *remote_a_ip,char *remote_v_ip,int laudio_port,int lvideo_port,int raudio_port,int rvideo_port, 
					MediaCodecInfo *media_info,int mode,int multiple_way_video);
					
void QuAudioCodecStrtoInt(char *audio_codec_name , int *audio_codec);
void QuVideoCodecStrtoInt(char *video_codec_name , int *video_codec);
void QuAudioIntToCodec(int audio_codec,char *audio_codec_name);

void *QuStreamInitEarlyMediaEXE(void *arg);
void *QuStreamTerminateEarlyMediaEXE(void *arg);

void QuQstreamCommand(int command_type,QStreamPara *media_para);
void *QuCommandEXE(void *arg);
void QuQstreamCommandClean();
void QuQstreamPrintParameter(QStreamPara *media);

int QuGetRemoteH264CodecInfo(pSess sess,MediaCodecInfo *media_info);

void setQTMediaStreamPara(QStreamPara media ,itricall::ItriCall* qt_call, int media_type);

#endif
