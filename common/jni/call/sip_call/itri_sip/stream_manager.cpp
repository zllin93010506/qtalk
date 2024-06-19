#include <stdio.h>
#include <string.h>
#ifdef _WIN32

	#include  <windows.h>
	#include "DbgUtility.h"
	#define printf dxprintf
	#define strcasecmp _stricmp
#else

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#define _snprintf snprintf

#endif

#include "itri_sip.h"

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif
#include <sipTx/sipTx.h>
#include "callmanager.h"
#include "itri_define.h"
#include "itri_utility.h"
#include "stream_manager.h"
#include <pthread.h>

#ifdef _ANDROID_APP
#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#define printf LOGD
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include "sip_log_ios.h"
#define printf CLog_sip

#endif

pthread_mutex_t qstream_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qstream_para_mutex=PTHREAD_MUTEX_INITIALIZER;

using namespace call;

extern QuCall qcall[NUMOFCALL];
extern int g_network_inetrface_type;

int QuGetRemoteMediaInfo(pSess remotesess ,char *remote_a_ip,char *remote_v_ip,int *audio_port,int *video_port)
{
	char remoteip[64]="0";
	int iplen=63;
	int media_size;
	int i;
	pSessMedia media=NULL;
	MediaType media_type;

	if (SessGetAddress(remotesess,remoteip,&iplen)==RC_OK){//Get Remote ip address
		printf("SessGetAddress = %s\n",remoteip);
		memset(remote_a_ip,0,sizeof(remote_a_ip));
		memset(remote_v_ip,0,sizeof(remote_v_ip));
		strcpy(remote_a_ip,remoteip);
		strcpy(remote_v_ip,remoteip);
	}
	/*
	char sessbuf[1024];
	int sesslen=1023;
	if (SessToStr(remotesess,sessbuf,&sesslen)==RC_OK) //output string
		printf("sess sdp:\n%s\n",sessbuf);
	*/
	
	bool is_get_audio_port=false;
	bool is_get_video_port=false;
	
	UINT32 tmp_audio_port=0 ;
	UINT32 tmp_video_port=0 ;
	SessGetMediaSize(remotesess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(remotesess,i,&media);
		SessMediaGetType(media,&media_type);
		if (media_type==MEDIA_AUDIO && !is_get_audio_port){
			iplen=63;
			SessMediaGetPort(media ,&tmp_audio_port);
			*audio_port = tmp_audio_port ;
			printf("audio port = %d\n",*audio_port);
			is_get_audio_port=true;
			
			if (SessMediaGetAddress(media,remoteip,&iplen)==RC_OK){
				printf("Audio SessMediaGetAddress = %s\n",remoteip);
				memset(remote_a_ip,0,sizeof(remote_a_ip));
				strcpy(remote_a_ip,remoteip);
			}
			
		}else if(media_type==MEDIA_VIDEO && !is_get_video_port) {	
			iplen=63;
			if (SessMediaGetAddress(media,remoteip,&iplen)==RC_OK){
				printf("Video SessMediaGetAddress = %s\n",remoteip);
				memset(remote_v_ip,0,sizeof(remote_v_ip));
				strcpy(remote_v_ip,remoteip);
			}
			SessMediaGetPort(media ,&tmp_video_port);
			*video_port = tmp_video_port;
			printf("video port = %d\n",*video_port);
			is_get_video_port=true;
		}
	}
	
	return 1;
}

int QuGetMediaCodecInfo(pSess sess,MediaCodecInfo *media_info)
{
	int iplen=31;
	int media_size;
	UINT32 rtpmap_size;
	int i,j;
	pSessMedia media=NULL;
	pMediaPayload payload=NULL;
	pMediaPayload fmtp_payload=NULL;
	MediaType media_type;
	char payloadbuf[256]="0";
	int payloadlen;
	char payloadtype[32]="0";
	int payloadtypelen;
	char *pch;
	
	int payload_type;
	bool is_audio_match=false;
	bool is_video_match=false;
	
	
	printf("------QuGetMediaCodecInfo---------\n");
	
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			for(j=0;j<rtpmap_size;j++){
				//printf("rtpmap_size=%d\n",rtpmap_size);
				payloadtypelen=31;
				payloadlen=255;
				SessMediaGetRTPParamAt(media,RTPMAP,j,&payload);
				MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
				
				
				payload_type = atoi(payloadtype);
				
				printf("payload_type :%d\n",payload_type);
				
				if (0==payload_type && (!is_audio_match)){
					strcpy(media_info->audio_payload,"PCMU");
					media_info->audio_payload_type = payload_type; 
					printf("0==audio_type :%s\n",media_info->audio_payload);
					media_info->audio_sample_rate=8000;
				}else if (8==payload_type && (!is_audio_match)){
					strcpy(media_info->audio_payload,"PCMA");
					media_info->audio_payload_type = payload_type; 
					printf("8==audio_type :%s\n",media_info->audio_payload);
					media_info->audio_sample_rate=8000;
				}else if (9==payload_type && (!is_audio_match)){
					strcpy(media_info->audio_payload,"G722");
					media_info->audio_payload_type = payload_type; 
					printf("9==audio_type :%s\n",media_info->audio_payload);
					
				}
				
				if (RC_OK==MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)){
					printf("payloadbuf :%s\n",payloadbuf);
					pch = strtok (payloadbuf,"/");
					//printf("pch :%s\n",pch);

					if (pch != NULL && !strcasecmp(pch,"telephone-event")){
						media_info->dtmf_type = payload_type;
						printf("telephone-event :%d\n",media_info->dtmf_type);
					}else {
						if (!is_audio_match){
							strcpy(media_info->audio_payload,pch);
							media_info->audio_payload_type = payload_type;
							printf("not telephone-event :%s\n",media_info->audio_payload);
							
							/*2010-0601 set g7221 codec information*/
							pch = strtok(NULL,"");
							if (NULL!=pch){
								media_info->audio_sample_rate=atoi(pch);
								if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
									payloadlen=255;
									MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
								}
								if (!strcasecmp("G7221",media_info->audio_payload)){
									/*check fmtp paload length*/
									if(strlen(payloadbuf)>0){
										char fmtp_tmp[40]="0";
										strcpy (fmtp_tmp,payloadbuf);
										char *fmtp_pch = strtok (fmtp_tmp,"=");
										fmtp_pch = strtok(NULL,"");
										media_info->audio_bit_rate=atoi(fmtp_pch);
									}
								}else if (!strcasecmp(OPUS_NAME,media_info->audio_payload)){
									if(strlen(payloadbuf)>0){
										char fmtp_tmp[255]="0";
										strcpy (fmtp_tmp,payloadbuf);
										printf("fmtp_tmp=%s\n",fmtp_tmp);
										/*Get "maxplaybackrate" value*/
										char *fmtp_pch = (char *)strstr (fmtp_tmp,"maxaveragebitrate");
										fmtp_pch = strtok(fmtp_pch,"=");
										fmtp_pch = strtok(NULL,"");
										media_info->audio_bit_rate=atoi(fmtp_pch);
										printf("bit rate=%d\n",media_info->audio_bit_rate);

										/*Get "sprop-maxcapturerate" value*/
										strcpy (fmtp_tmp,payloadbuf);
										fmtp_pch = (char *)strstr (fmtp_tmp,"sprop-maxcapturerate");
										fmtp_pch = strtok(fmtp_pch,"=");
										fmtp_pch = strtok(NULL,"");
										media_info->audio_sample_rate=atoi(fmtp_pch);
										printf("sample rate=%d\n",media_info->audio_sample_rate);
									}
								}
							}
							is_audio_match=true;
						}
#if AUDIO_FEC
                        if (pch != NULL && !strcasecmp(pch,"red")){
                            payloadtypelen = 31;
                            if (MediaPayloadGetType(payload,payloadtype,&payloadtypelen)==RC_OK){
                                //atoi(AUDIO_FEC_PAYLOAD);
                                media_info->audio_fec_ctrl.receive_payloadType = atoi(payloadtype);
                                media_info->audio_fec_ctrl.send_payloadType = atoi(AUDIO_FEC_PAYLOAD);
                                printf("red audio_payload_type :%d\n",media_info->audio_fec_ctrl.receive_payloadType);
                                
                            }
                            media_info->audio_fec_ctrl.ctrl = 1;
                        }
#endif
					}
				}
				
			}
		}else if(media_type==MEDIA_VIDEO) {
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					
					payloadlen=255;
					if (MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)==RC_OK){
						pch = strtok (payloadbuf,"/");
						if (!strcasecmp(pch,"H264") && (!is_video_match)){
							strcpy(media_info->video_payload,pch);
							printf("video_payload :%s\n",media_info->video_payload);
							
							payloadtypelen = 31;
							if (MediaPayloadGetType(payload,payloadtype,&payloadtypelen)==RC_OK){
								media_info->video_payload_type = atoi(payloadtype);
								printf("video_payload_type :%d\n",media_info->video_payload_type);
							}
							is_video_match=true;
							
						}
					}
				}
	
			}

			media_info->rtcp_fb.nack.pli = QuSessMediaGetAttribute(media,"rtcp-fb","pli");
			media_info->rtcp_fb.ccm.fir = QuSessMediaGetAttribute(media,"rtcp-fb","fir");
		}
	}

	return 1;
}

int QuGetRemoteH264CodecInfo(pSess sess,MediaCodecInfo *media_info)
{	
	printf("------QuGetH264_MBPS_MFS---------\n");
	
	int media_size;
	UINT32 rtpmap_size;
	int i,j;
	pSessMedia media=NULL;
	pMediaPayload payload=NULL;
	pMediaPayload fmtp_payload=NULL;
	MediaType media_type;
	char payloadbuf[256]="0";
	int payloadlen;
	char payloadtype[32]="0";
	int payloadtypelen;
	char *pch;
	
	UINT32 rsess_bandwidth=-1;
	bool is_set_h264_codec_info = false;
	
	if (RC_OK==SessGetBandwidth(sess,"TIAS",&rsess_bandwidth)){
		rsess_bandwidth = rsess_bandwidth/1000;
		printf("Sess TIAS = %d\n",rsess_bandwidth);							
	}else if (RC_OK==SessGetBandwidth(sess,"AS",&rsess_bandwidth)){
		printf("Sess AS = %d\n",rsess_bandwidth);
	}else if (RC_OK==SessGetBandwidth(sess,"CT",&rsess_bandwidth)){
		printf("Sess CT = %d\n",rsess_bandwidth);
	}
	
	if (rsess_bandwidth != -1) {
		if (rsess_bandwidth > 128){
			media_info->media_bandwidth=rsess_bandwidth - 128;// audio bandwidth is 128kbps
		}else{
			media_info->media_bandwidth = rsess_bandwidth;
		}
	}else{
		media_info->media_bandwidth = -1;
	}
	
	UINT32 rr_sess_bandwidth=-1;
	if (RC_OK==SessGetBandwidth(sess,"RR",&rr_sess_bandwidth)){
		printf("Sess RR = %d\n",rr_sess_bandwidth);
		media_info->RR_bps=rr_sess_bandwidth;
	}
	UINT32 rs_sess_bandwidth=-1;
	if (RC_OK==SessGetBandwidth(sess,"RS",&rs_sess_bandwidth)){
		printf("Sess RS = %d\n",rs_sess_bandwidth);
		media_info->RS_bps=rs_sess_bandwidth;
	}
							
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		if(media_type==MEDIA_VIDEO && !is_set_h264_codec_info) {
			
			printf("rtpmap_size=%d\n",rtpmap_size);
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					
					payloadlen=255;
					if (MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)==RC_OK){
						pch = strtok (payloadbuf,"/");
						
						if (!strcasecmp(pch,"H264")){
							
							UINT32 rmedia_bandwidth=-1;
							
							if (RC_OK==SessMediaGetBandwidth(media,"TIAS",&rmedia_bandwidth)){
								rmedia_bandwidth = rmedia_bandwidth/1000;
								printf("media TIAS = %d\n",rmedia_bandwidth);
								media_info->media_bandwidth=rmedia_bandwidth;
							}else if (RC_OK==SessMediaGetBandwidth(media,"AS",&rmedia_bandwidth)){
								printf("media AS = %d\n",rmedia_bandwidth);
								media_info->media_bandwidth=rmedia_bandwidth;
							}else if (RC_OK==SessMediaGetBandwidth(media,"CT",&rmedia_bandwidth)){
								printf("media CT = %d\n",rmedia_bandwidth);
								media_info->media_bandwidth=rmedia_bandwidth;
							}
							
							UINT32 rr_bandwidth=-1;
							if (RC_OK==SessMediaGetBandwidth(media,"RR",&rr_bandwidth)){
								printf("media RR = %d\n",rr_bandwidth);
								media_info->RR_bps=rr_bandwidth;
							}
							UINT32 rs_bandwidth=-1;
							if (RC_OK==SessMediaGetBandwidth(media,"RS",&rs_bandwidth)){
								printf("media RS = %d\n",rs_bandwidth);
								media_info->RS_bps=rs_bandwidth;
							}
							
							
							payloadtypelen = 31;
							MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
							/*2011-1123 parser H264 paremeter*/
							if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
								payloadlen=255;
								if(1==QuParseH264Parameter(fmtp_payload,&media_info->h264_parameter)) {
									//QuPrintProfileLevelID(h264_level_id);
								}
							}
							
							is_set_h264_codec_info = true;
						}else if (!strcasecmp(pch,"red")){
							//if (!strcmp(sip_config.generic_rtp.video_fec_enable,"1")){
                            if (1){
								payloadtypelen = 31;
								if (MediaPayloadGetType(payload,payloadtype,&payloadtypelen)==RC_OK){
									media_info->video_fec_ctrl.receive_payloadType = atoi(payloadtype);
									media_info->video_fec_ctrl.send_payloadType = atoi(VIDEO_FEC_PAYLOAD);
									printf("red video_payload_type :%d\n",media_info->video_fec_ctrl.receive_payloadType);
								}
								
								media_info->video_fec_ctrl.ctrl = 1;
							}else{
								media_info->video_fec_ctrl.ctrl = 0;
							}
							
						}
					}
				}
			}
		}
	}
	
	
	return 1;
}

void QuAudioCodecStrtoInt(char *audio_codec_name , int *audio_codec){
	
	if (!strcasecmp(PCMU_NAME,audio_codec_name)||!strcasecmp("PCMU",audio_codec_name)){
		*audio_codec = PCMU;
	}else if (!strcasecmp(PCMA_NAME,audio_codec_name)||!strcasecmp("PCMA",audio_codec_name)){
		*audio_codec = PCMA;
	}else if (!strcasecmp(G7221_NAME,audio_codec_name)){
		*audio_codec = G7221;
	}else if (!strcasecmp(G722_NAME,audio_codec_name)){
		*audio_codec = G722;
	}else if (!strcasecmp(AAC_LC_NAME,audio_codec_name)){
		*audio_codec = AAC_LC;
	}else if (!strcasecmp(OPUS_NAME,audio_codec_name)){
		*audio_codec = OPUS;
	}else{
		*audio_codec = -1;
	}
	
}

void QuAudioIntToCodec(int audio_codec,char *audio_codec_name){
	
	if (audio_codec_name==NULL)
		return;
	
	if (audio_codec==PCMU){
		strcpy(audio_codec_name,PCMU_NAME);
	}else if (audio_codec == PCMA){
		strcpy(audio_codec_name,PCMA_NAME);
	}else if (audio_codec == G7221){
		strcpy(audio_codec_name,G7221_NAME);
	}else if (audio_codec == G722){
		strcpy(audio_codec_name,G722_NAME);
	}else if (audio_codec == AAC_LC){
		strcpy(audio_codec_name,AAC_LC_NAME);
	}else if (audio_codec == OPUS){
		strcpy(audio_codec_name,OPUS_NAME);
	}else{
		strcpy(audio_codec_name,"");
		printf("!!No Audio int to Codec\n");
	}
	
}

void QuVideoCodecStrtoInt(char *video_codec_name , int *video_codec){
	
	if (!strcasecmp("H264",video_codec_name)){
		*video_codec = H264;
	}else{
		*video_codec = -1;
	}
	
}

int QuGetG7221CodecInfo(pSess sess, char* sample_rate, char* bit_rate){
	int media_size;
	UINT32 rtpmap_size;
	pSessMedia media=NULL;
	MediaType media_type;
	pMediaPayload payload=NULL;
	pMediaPayload fmtp_payload=NULL;
	int i,j;
	char payloadbuf[256]="0";
	int payloadlen;
	char payloadtype[32]="0";
	int payloadtypelen;
	char *pch;
	
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			for(j=0;j<rtpmap_size;j++){
				//printf("rtpmap_size=%d\n",rtpmap_size);
				payloadtypelen=31;
				payloadlen=255;
				SessMediaGetRTPParamAt(media,RTPMAP,j,&payload);
				MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
				MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
				if (0==j){
					pch = strtok (payloadbuf,"/");
					pch = strtok(NULL,"");
					
					strcpy(sample_rate,pch); 
					
					if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
						payloadlen=255;
						MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);

						char fmtp_tmp[40];
						strcpy (fmtp_tmp,payloadbuf);
						char *fmtp_pch = strtok (fmtp_tmp,"=");
						//printf("%s\n",fmtp_pch);
						fmtp_pch = strtok(NULL,"");
						strcpy(bit_rate,fmtp_tmp);
					}
				}
			}
		}
	}
	
	return 1;
}

void QuQstreamPrintParameter(QStreamPara *media){
	
	if (media!=NULL){
		printf("=================================start=====================================\n");
		printf("session=%d , mode=%d, isquanta=%d\n",media->session,media->mode,media->isquanta);
		printf("audio_source=%d,sip_idr_support=%d\n",media->audio_source,media->sip_idr_info_support);
		printf("remote_a_ip=%s ,remote_v_ip=%s\n",media->remote_a_ip,media->remote_v_ip);
		printf("laudio_port=%d ,lvideo_port=%d\n",media->laudio_port,media->lvideo_port);
		printf("raudio_port=%d ,rvideo_port=%d\n",media->raudio_port,media->rvideo_port);
		printf("audio_codec=%d ,audio_payload=%s ,audio_sample_rate=%d ,audio_bit_rate=%d\n",media->audio_codec,media->audio_payload,media->audio_sample_rate,media->audio_bit_rate);
		printf("video_codec=%d ,video_payload=%s\n",media->video_codec,media->video_payload);
		printf("dtmf_pt=%d ,dtmf_transmit_mode=%d\n",media->dtmf_pt,media->dtmf_transmit_mode);
		printf("carrier_network=%s\n",media->carrier_network);
		printf("media bandwidth=%d\n",media->media_bandwidth);
		printf("level_idc=%d,mbps=%d,mfs=%d,pk_mode=%d\n",media->h264_parameter.profile_level_id.level_idc,media->h264_parameter.max_mbps,media->h264_parameter.max_fs,media->h264_parameter.packetization_mode);
		printf("RS_bps =%d\n",media->RS_bps);
		printf("RR_bps=%d\n",media->RR_bps);
		printf("fir=%d,pli=%d\n",media->rtcp_fb.ccm.fir,media->rtcp_fb.nack.pli);
        printf("audio_fec=%d,self=%d,opps=%d\n",media->audio_fec_ctrl.ctrl,media->audio_fec_ctrl.send_payloadType,media->audio_fec_ctrl.receive_payloadType);
        printf("video_fec=%d,self=%d,opps=%d\n",media->video_fec_ctrl.ctrl,media->video_fec_ctrl.send_payloadType,media->video_fec_ctrl.receive_payloadType);
		printf("audio srtp enable=%d,self key=%s,opp_key=%s\n",media->audio_srtp.enable_srtp, media->audio_srtp.self_srtp_key.key_salt, media->audio_srtp.opposite_srtp_key.key_salt);
		printf("video srtp enable=%d,self key=%s,opp_key=%s\n",media->video_srtp.enable_srtp, media->video_srtp.self_srtp_key.key_salt, media->video_srtp.opposite_srtp_key.key_salt);
        printf("==================================end====================================\n");
	}

}

void set_media_para(QStreamPara *media ,int session,int isquanta,char *remote_a_ip,char *remote_v_ip,int laudio_port,int lvideo_port,int raudio_port,int rvideo_port, 
					MediaCodecInfo *media_info,int mode,int multiple_way_video)
{
	
	//printf("init set_media_para\n");
	pthread_mutex_lock ( &qstream_para_mutex );
	//printf("start set_media_para\n");
	memset(media,0, sizeof(QStreamPara));
	
	media->session=session;
	//printf("session=%d\n",media->session);
	media->isquanta=isquanta;
	if (remote_a_ip!=NULL)
		strcpy(media->remote_a_ip, remote_a_ip);
	
	if (remote_v_ip!=NULL)
		strcpy(media->remote_v_ip, remote_v_ip);

	media->laudio_port=laudio_port; 
	media->lvideo_port=lvideo_port; 
	media->raudio_port=raudio_port; 
	media->rvideo_port=rvideo_port;
	
	if (media_info!=NULL)
		media->audio_codec=media_info->audio_payload_type;
	
	if (media_info!=NULL && media_info->audio_payload!=NULL)
		strcpy(media->audio_payload,media_info->audio_payload); 

	if (media_info!=NULL)
		media->audio_sample_rate=media_info->audio_sample_rate;

	if (media_info!=NULL)
		media->audio_bit_rate=media_info->audio_bit_rate;
	
	if (media_info!=NULL)
		media->video_codec=media_info->video_payload_type;
	
	if (media_info!=NULL && media_info->video_payload!=NULL)
		strcpy(media->video_payload, media_info->video_payload); 
	
	media->mode=mode;
	
	if (media_info!=NULL)
		media->dtmf_pt=media_info->dtmf_type;

	if (media->dtmf_pt==-1){
		media->dtmf_transmit_mode=1;//set dtmf mode
	}else{
		//media->dtmf_transmit_mode=atoi(sip_config.generic_lines.dtmf_transmit_method);
	}
	
	//strcpy(media->carrier_network, sip_config.device_provisioning.carrier_network); 
	
	if (media_info!=NULL)
		media->media_bandwidth=media_info->media_bandwidth;
	
	if(media_info!=NULL && &media_info->h264_parameter!=NULL){
		memcpy(&media->h264_parameter,&media_info->h264_parameter,sizeof(QuH264Parameter));
	}
	
	media->multiple_way_video = multiple_way_video;
	
	if (media_info!=NULL)
		media->RS_bps=media_info->RS_bps;
	
	if (media_info!=NULL)
		media->RR_bps=media_info->RR_bps;
	
	if (media_info!=NULL && &media->rtcp_fb!=NULL)
		memcpy(&media->rtcp_fb,&media_info->rtcp_fb,sizeof(RTCP_FB));
	
    if (media_info!=NULL && &media->audio_fec_ctrl!=NULL)
		memcpy(&media->audio_fec_ctrl,&media_info->audio_fec_ctrl,sizeof(FEC_Control_t));
    
    if (media_info!=NULL && &media->video_fec_ctrl!=NULL)
		memcpy(&media->video_fec_ctrl,&media_info->video_fec_ctrl,sizeof(FEC_Control_t));

    
    //set audio/video SRTP key parameter
	if (media_info !=NULL && (&media->audio_srtp !=NULL)){
		if ((media_info->audio_srtp.self_srtp_key.crypto_suit) && (media_info->audio_srtp.opposite_srtp_key.crypto_suit)){
			media_info->audio_srtp.enable_srtp = true;
		}
		memcpy(&media->audio_srtp , &media_info->audio_srtp , sizeof(MEDIA_SRTP_KEY_PARAMETER));
	}
	
	if (media_info !=NULL && (&media->video_srtp) !=NULL){
		if ((media_info->video_srtp.self_srtp_key.crypto_suit) && (media_info->video_srtp.opposite_srtp_key.crypto_suit)){
			media_info->video_srtp.enable_srtp = true;
		}

		memcpy(&media->video_srtp , &media_info->video_srtp , sizeof(MEDIA_SRTP_KEY_PARAMETER));
	}

	//printf("end set_media_para\n");
	pthread_mutex_unlock ( &qstream_para_mutex );
	
	//printf("%s\n",media->video_payload);
	//printf("%d\n",media->h264_level_id.level_idc);
}

void setQTMediaStreamPara(QStreamPara media ,itricall::ItriCall* qt_call, int media_type){
	
	printf("!!!!!setQTMediaStreamPara!!!!!\n");
	//remote session information
	strcpy (qt_call->mCallData.remote_audio_ip , media.remote_a_ip);
	qt_call->mCallData.remote_audio_port = media.raudio_port;

	strcpy(qt_call->mCallData.remote_video_ip , media.remote_v_ip);
	qt_call->mCallData.remote_video_port = media.rvideo_port;
	
	//local session information
	strcpy(qt_call->mCallData.local_audio_ip , "127.0.0.1");
	qt_call->mCallData.local_audio_port = media.laudio_port;

	strcpy(qt_call->mCallData.local_video_ip , "127.0.0.1");
	qt_call->mCallData.local_video_port = media.lvideo_port;
	
	//audio codec information
	
	qt_call->mCallData.audio_mdp.clock_rate = media.audio_sample_rate;
	qt_call->mCallData.audio_mdp.bitrate = media.audio_bit_rate;

	qt_call->mCallData.audio_mdp.payload_id = media.audio_codec;

	strcpy (qt_call->mCallData.audio_mdp.mime_type , media.audio_payload);
    
    //int call_id = QuGetCallFromICall(qt_call);
    //if (qcall[call_id].network_inetrface_type == NETWORK_3G){
    qt_call->mCallData.network_interface_type = NETWORK_WIFI;

    switch (g_network_inetrface_type){
    	case NETWORK_3G:
    		qt_call->mCallData.network_interface_type = NETWORK_3G;
    		break;
    	case NETWORK_WIFI:
    		qt_call->mCallData.network_interface_type = NETWORK_WIFI;
        	break;
    	case NETWORK_ETH:
    		qt_call->mCallData.network_interface_type = NETWORK_ETH;
        	break;
        default:
        	qt_call->mCallData.network_interface_type = NETWORK_WIFI;
        	break;

    }
	
    qt_call->mCallData.audio_mdp.fec_ctrl.ctrl = media.audio_fec_ctrl.ctrl;
    qt_call->mCallData.audio_mdp.fec_ctrl.receive_payloadType = media.audio_fec_ctrl.receive_payloadType;
    qt_call->mCallData.audio_mdp.fec_ctrl.send_payloadType = media.audio_fec_ctrl.send_payloadType;
    
    /*srtp parameter*/
    memcpy(&(qt_call->mCallData.audio_mdp.srtp), &media.audio_srtp, sizeof(MEDIA_SRTP_KEY_PARAMETER));

    //video codec information
	if (media_type==VIDEO){
		printf("Set Video parameter\n");
		qt_call->mCallData.video_mdp.clock_rate = 90000;
        
        if (media.rtcp_fb.ccm.fir==1){
            qt_call->mCallData.video_mdp.enable_rtcp_fb_fir = true;
        }else {
            qt_call->mCallData.video_mdp.enable_rtcp_fb_fir = false;
        }
        
        if (media.rtcp_fb.nack.pli==1){
            qt_call->mCallData.video_mdp.enable_rtcp_fb_pli = true;
        }else {
            qt_call->mCallData.video_mdp.enable_rtcp_fb_pli = false;
        }
		
		
		qt_call->mCallData.video_mdp.level_idc = media.h264_parameter.profile_level_id.level_idc;
		qt_call->mCallData.video_mdp.max_fs = media.h264_parameter.max_fs;
		qt_call->mCallData.video_mdp.max_mbps = media.h264_parameter.max_mbps;
		strcpy (qt_call->mCallData.video_mdp.mime_type , media.video_payload);
		qt_call->mCallData.video_mdp.packetization_mode = media.h264_parameter.packetization_mode;
		qt_call->mCallData.video_mdp.payload_id = media.video_codec;
		qt_call->mCallData.video_mdp.profile = media.h264_parameter.profile_level_id.profile_idc;
		qt_call->mCallData.video_mdp.tias_kbps = media.media_bandwidth;
        
        qt_call->mCallData.video_mdp.fec_ctrl.ctrl = media.video_fec_ctrl.ctrl;
        qt_call->mCallData.video_mdp.fec_ctrl.receive_payloadType = media.video_fec_ctrl.receive_payloadType;
        qt_call->mCallData.video_mdp.fec_ctrl.send_payloadType = media.video_fec_ctrl.send_payloadType;

        memcpy(&(qt_call->mCallData.video_mdp.srtp), &media.video_srtp, sizeof(MEDIA_SRTP_KEY_PARAMETER));
	}
}
