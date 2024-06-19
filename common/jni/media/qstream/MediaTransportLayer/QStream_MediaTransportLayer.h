// CommunicationEngine.h : Communication Engine
// -----------------------------------------------------------------------
// Quanta Computer, QRI/Users/yang/Desktop/QTa/qt1.5a/source/backend/jni/media/qstream/MediaTransportLayer/QStream_MediaTransportLayer.h
// =======================================================================
// 2009/11/20   Frank.Ting Create
//
// =======================================================================

#ifndef _COMMUNICATION_ENGHINE_H_
#define _COMMUNICATION_ENGHINE_H_

#define QSCE_SOCKET_OPTION_TOS	0
#include "ET1_QStream_TransportLayer.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
        int ctrl;
        int send_pt;
        int recv_pt;
} FEC_t;

//#define MTL_MAX_SESSION         MAX_TL_SESSION_NUM

typedef struct {
    int (*rtcp_rr)(unsigned long id, void *info);
    int (*rtcp_sr)(unsigned long id, void *info);
    int (*tfrc_report)(unsigned long id, void *info);
    
    void (*timestamp_setting)(unsigned long id);
    void (*rtcp_app_set)(unsigned long id);
    void (*rtcp_app_receive)(unsigned long id, void *info);
    void (*dtmf_event)(unsigned long id, int dtmf_key, unsigned long timestamp);   

    int (*rfc4585_handle)(unsigned long id, uint16_t seq_num, unsigned long timestamp);
    void (*rtp_traffic_monitor)(int mediaType, uint16_t seq_num,uint16_t pkt_len);
    int  (*check_resend_pkt)(unsigned long id, uint16_t seq_num, unsigned long timestamp);

    void (*rtcp_psfb_receive)(unsigned long id, void *info);

    void (*rtp_rx)(unsigned long id,uint16_t seq_num, unsigned long timestamp);

} QSTL_CALLBACK_T;


typedef struct _MTL_SESSION_t{
    struct TL_Session_t *session;
    QSTL_CALLBACK_T callback; 
    char host_ipaddr[MAX_PATH];
    char local_ipaddr[MAX_PATH];
    int listen_port;
    int host_port;
    int media_type;
    int tfrc; // 1:enable, 0:disable
    int isBusy;
	int tos;
    unsigned long parent_id;

    TL_FECControl_t fec;
	char mime_type[MAX_PATH];
    PayloadType_t payload;
} MTL_SESSION_t;


int QSCE_InitLibrary(void);
int QSCE_Create(unsigned long *mtl_id, int media_type);
int QSCE_Destory(unsigned long mtl_id);
int QSCE_Terminate(void);
int QSCE_RegisterCallback(unsigned long mtl_id, QSTL_CALLBACK_T *callback, unsigned long parent_id);
int QSCE_SetLocalReceiver(unsigned long mtl_id, unsigned short port,const char *ipaddr);
int QSCE_SetSendDestination(unsigned long mtl_id, unsigned short port,const char *ipaddr);
int QSCE_SetTFRC(unsigned long mtl_id, int flag);

int QSCE_Start(unsigned long mtl_id);
int QSCE_StartWithSSRC(unsigned long mtl_id, unsigned long ssrc);
int QSCE_Stop(unsigned long mtl_id);

unsigned long QSCE_GetSessionSSRC(unsigned long mtl_id);
int QSCE_ResetSendingBuffer(unsigned long mtl_id);
int QSCE_GetRxTimeStamp(unsigned long mtl_id, unsigned long *timestamp);
int QSCE_SetExtraInfo(unsigned long mtl_id, char *ExtraInfo, int infoLen);
int QSCE_SetLossPktReSend(unsigned long mtl_id, int enable);
int QSCE_SendPLIIndication(unsigned long mtl_id); // 請Sender端馬上重送新的GOP
int QSCE_SetMTU(unsigned long mtl_id, int mtu);	// default: 1380 bytes
int QSCE_SetTFRCMinSendingRate(unsigned long mtl_id, unsigned long min_sending_rate);
int QSCE_Set_Network_Payload(unsigned long mtl_id, int clock_rate, int codec_type, char *mime_type, uint16_t rtp_event_pt_num);
int QSCE_Set_TOS(unsigned long mtl_id, int tos);


/* Enable and Disable to Send RTP without terminate the session */ /* add by willy, 2010-10-01 */
int QSCE_Send_RTP_Enable(unsigned long mtl_id);
int QSCE_Send_RTP_Disable(unsigned long mtl_id);


/* Set Socket Option */ /*add by willy, 2010-11-10 */
int QSCE_Set_Socket_Option(int option_name, const void *option_value);
void QSCE_Set_Audio_Tos(int tos);
void QSCE_Set_Video_Tos(int tos); 

/* used to disable/enable rtcp packet */ /* add by willy, 2011-11-07 */
int QSCE_Disable_RTCP(unsigned long mtl_id);
int QSCE_Enable_RTCP(unsigned long mtl_id);

int QSCE_Set_Maximum_Payload_Size(unsigned long mtl_id, int maximum_payload_size);


int QSCE_Set_FEC(unsigned long mtl_id, 
                 int ctrl, 
                 int send_payloadType, int receive_payloadType);

/* QSCE_ */

#ifdef __cplusplus
}
#endif

#endif // _COMMUNICATION_ENGHINE_H_
