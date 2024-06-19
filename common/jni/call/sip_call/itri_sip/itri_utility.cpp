#include <stdio.h>

#ifndef _WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <resolv.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
#endif

#include "itri_call.h"
#include "itri_sip.h"

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif

#include "itri_define.h"
#include "callmanager.h"

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

char *str_replace (char *source, char *find,  char *rep){  

	int find_L=strlen(find);  
	int rep_L=strlen(rep);  
	int length=strlen(source)+1;  
	int gap=0;  
	
	char *result = (char*)malloc(sizeof(char) * length);  
	strcpy(result, source);      

	char *former=source;  
	const char *location= strstr(former, find);  
	
	while(location!=NULL){  
		gap+=(location - former);  
		result[gap]='\0';  
		
		length+=(rep_L-find_L);  
		result = (char*)realloc(result, length * sizeof(char));  
		strcat(result, rep);  
		gap+=rep_L;  
		
		former=(char*)location+find_L;  
		strcat(result, former);  

		location= strstr(former, find);  
	}  
	
	return result;
}  

/*Print Media Codec Information*/
void MediaCodecInfoPrint(MediaCodecInfo *media_info){

	if (media_info!=NULL){
		printf("---------media codec info---------\n");
		printf("audio_payload_type=%d,audio_payload=%s\n",media_info->audio_payload_type,media_info->audio_payload);
		printf("audio sample rate=%d ,audio bit rate=%d\n",media_info->audio_sample_rate,media_info->audio_bit_rate);
		printf("video_payload_type=%d,video_payload=%s\n",media_info->video_payload_type,media_info->video_payload);
		printf("dtmf_type=%d\n",media_info->dtmf_type);
		printf("media_bandwidth=%d\n",media_info->media_bandwidth);
		printf("max_mbps=%d\n",media_info->h264_parameter.max_mbps);
		printf("max_fs=%d\n",media_info->h264_parameter.max_fs);
		printf("pli=%d\n",media_info->rtcp_fb.nack.pli);
		printf("fir=%d\n",media_info->rtcp_fb.ccm.fir);
		printf("rs=%d\n",media_info->RS_bps);
		printf("rr=%d\n",media_info->RR_bps);
		printf("audio fec=>ctrl=%d,send_payloadType=%d,receive_payloadType=%d\n",media_info->audio_fec_ctrl.ctrl
				,media_info->audio_fec_ctrl.send_payloadType,media_info->audio_fec_ctrl.receive_payloadType);
		printf("video fec=>ctrl=%d,send_payloadType=%d,receive_payloadType=%d\n",media_info->video_fec_ctrl.ctrl
				,media_info->video_fec_ctrl.send_payloadType,media_info->video_fec_ctrl.receive_payloadType);
		//printf("rfc4588=>ctrl=%d,send_payloadType=%d,receive_payloadType=%d,my_rtxTime=%d,opposite_rtxTime=%d\n",
		//	   media_info->rfc4588_ctrl.ctrl,media_info->rfc4588_ctrl.send_payloadType,media_info->rfc4588_ctrl.receive_payloadType
		//	   ,media_info->rfc4588_ctrl.my_rtxTime,media_info->rfc4588_ctrl.opposite_rtxTime);

		printf("---------END media codec info---------\n");
	}

}

int QuSessMediaGetAttribute(pSessMedia media,const char* attrib,const char* attrvalue){
	
	UINT32 len;
	int size;
	int pos;
	char *value=NULL;
	pMediaAtt temp;
	
	size=dxLstGetSize(media->attribelist);
	
	for(pos=0;pos<size;pos++){
		temp=(pMediaAtt)dxLstPeek(media->attribelist,pos);
		
		if(temp&&temp->name&&(strICmp(temp->name,"rtcp-fb")==0)){
			value=strdup(temp->value);
			//printf("value=%s,attrvalue=%s\n",value,attrvalue);
			
			char *attrvalue_tmp=NULL;
			
			attrvalue_tmp=strtok(value," ");
			
			while (attrvalue_tmp!=NULL){
				
				//printf("attrvalue_tmp=%s\n",attrvalue_tmp);
				if (!strcmp(attrvalue_tmp,attrvalue)){
					return 1;
				}
				
				attrvalue_tmp=strtok(NULL," ");
			}
			
			//break;
		}
	}
	return -1;
}
