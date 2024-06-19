// QStream_MediaTransportLayer.c : Transport Layer
// -----------------------------------------------------------------------
// Quanta Computer, QRI
// =======================================================================
// 2009/11/20   Frank.Ting Create
// 2009/12/30   Implemnet wrapper for Transport layer
// 2010/01/05   Implement QSCE_Transport_Callback_Dispatch()
//              Implement QSCE_Search_Network_SessionID()
//2010/2/13 Ian Tuan Modified
//              add QSCE_StartWithSSRC function
//
// =======================================================================
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "../GlobalDefine.h"
#include "QStream_MediaTransportLayer.h"
#include "ET1_QStream_TransportLayer.h"
#include "net_udp.h"
/*
PayloadType_t default_video_payload = {
	TYPE(PAYLOAD_VIDEO),
	CLOCK_RATE(90000),
	BITS_PER_SAMPLE(0),
	ZERO_PATTERN(NULL),
	PATTERN_LENGTH(0),
	NORMAL_BITRATE(256000),
	"H264",
	CHANNELS(0),
	PAYLOAD_TYPE_NUM(99),
    PAYLOAD_RTP_EVENT_TYPE_NUM(-1)
};

PayloadType_t default_audio_payload = {
    TYPE(PAYLOAD_AUDIO_PACKETIZED),
    CLOCK_RATE(8000), //this is G711's clock rate not G7221, but for test 
	BITS_PER_SAMPLE(0),
	ZERO_PATTERN(NULL),
	PATTERN_LENGTH(0),
	NORMAL_BITRATE(24000),
	"G.711",
	CHANNELS(1),
	PAYLOAD_TYPE_NUM(0),
    PAYLOAD_RTP_EVENT_TYPE_NUM(101)
};*/

//QSTL_CALLBACK_T g_QSCE_callback;
//pthread_mutex_t g_mtl_session_mutex;
//MTL_SESSION_t g_mtl_session[MTL_MAX_SESSION];
int g_audio_tos = 0;
int g_video_tos = 0;

int QSCE_InitLibrary(void)
{
	//static BOOL is_initialized = FALSE;
 //   int i;
 //   debug_msg("==>%s() %d\n", __FUNCTION__,is_initialized);
	//if (!is_initialized)
	//{
	//	is_initialized = TRUE;
	//	memset((char *)g_mtl_session, 0, sizeof(g_mtl_session));
	//	for(i = 0; i < MTL_MAX_SESSION; i++) {
	//		g_mtl_session[i].payload.mime_type = g_mtl_session[i].mime_type;
	//	}
	//	//pthread_mutex_init(&(g_mtl_session_mutex), NULL);
	//}
 //   debug_msg("<==%s()\n", __FUNCTION__);
    return 0;
}

int QSCE_Create(unsigned long *mtl_id, int media_type)
{   
    int ret;
    MTL_SESSION_t* psession = (MTL_SESSION_t*) malloc(sizeof(MTL_SESSION_t));
    debug_msg("==>%s()\n", __FUNCTION__);
    if (psession!=NULL)
    {
        memset(psession,0,sizeof(MTL_SESSION_t));
        psession->media_type = media_type;
        psession->isBusy = 1;
        if(media_type == QS_MEDIATYPE_AUDIO)
        {           
            psession->payload.type = PAYLOAD_AUDIO_PACKETIZED;     
            //memcpy(&psession->payload, &default_audio_payload, sizeof(PayloadType_t));
            //strcpy(psession->mime_type, default_audio_payload.mime_type);
        }
        else if(media_type == QS_MEDIATYPE_VIDEO) {
            psession->payload.type = PAYLOAD_VIDEO; 
            //memcpy(&psession->payload, &default_video_payload, sizeof(PayloadType_t));
            //strcpy(psession->mime_type, default_video_payload.mime_type);
        }
        *mtl_id = (unsigned long)psession;
        debug_msg("<==%s()\n", __FUNCTION__);
        return 0;
    }else
    {
        debug_msg("<==%s()\n", __FUNCTION__);
        return -1;
    }
    //pthread_mutex_lock(&(g_mtl_session_mutex));
    //ret = -1;
    //for(i = 0; i < MTL_MAX_SESSION; i++) {
    //    if(g_mtl_session[i].isBusy == 0) {  
    //        g_mtl_session[i].media_type = media_type;
    //        g_mtl_session[i].isBusy = 1;
    //        
    //        if(media_type == QS_MEDIATYPE_AUDIO) {                
    //            memcpy(&g_mtl_session[i].payload, &default_audio_payload, sizeof(PayloadType_t));
    //            strcpy(g_mtl_session[i].mime_type, default_audio_payload.mime_type);
    //        }
    //        else if(media_type == QS_MEDIATYPE_VIDEO) {
    //            memcpy(&g_mtl_session[i].payload, &default_video_payload, sizeof(PayloadType_t));
    //            strcpy(g_mtl_session[i].mime_type, default_video_payload.mime_type);
    //        }    
    //        g_mtl_session[i].payload.mime_type = g_mtl_session[i].mime_type;
    //        g_mtl_session[i].reserved = 0;
    //        *mtl_id = (unsigned long)&g_mtl_session[i]; // session_id, address
    //        ret = 0;
    //        break;
    //    }
    //}   
    //pthread_mutex_unlock(&(g_mtl_session_mutex));

    return ret; // (-1) session table is full
}

int QSCE_Destory(unsigned long mtl_id)
{  
    int ret;
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    //unsigned long cur_session;
    
    debug_msg("==>%s()\n", __FUNCTION__);

    if(mtl_id == 0)
        return -1;
    
    ret = 0;
    if(ptr->session != NULL) 
        TransportLayer_TerminateSession(ptr->session);
    free(ptr);
    //pthread_mutex_lock(&(g_mtl_session_mutex));
    //for(i = 0; i < MTL_MAX_SESSION; i++) 
    //{
    //    cur_session = (unsigned long)&g_mtl_session[i];
    //    if(cur_session == mtl_id) 
    //    {
    //        if(ptr->session != NULL) {
    //            TransportLayer_TerminateSession(ptr->session);
    //        }
    //        else
    //            printf("<dbg> %s(), session null\n", __FUNCTION__);

    //        memset((char *)&g_mtl_session[i], 0x0, sizeof(MTL_SESSION_t));
    //        ret = 0;
    //        //break;
    //    }
    //}
    //pthread_mutex_unlock(&(g_mtl_session_mutex));
    
    debug_msg("<==%s()\n", __FUNCTION__);
    return ret; // (-1) can't find out this session id in the g_mtl_session[] table
}

//int QSCE_Search_Network_SessionID(unsigned long *id_out, struct TL_Session_t *id_in)
//{
//    int i, ret;
//    
//    //pthread_mutex_lock(&(g_mtl_session_mutex));
//    ret = -1;
//    for(i = 0; i < MTL_MAX_SESSION; i++) {
//        if(g_mtl_session[i].isBusy) {
//            if(g_mtl_session[i].session == id_in) { 
//                *id_out = (unsigned long)&g_mtl_session[i];
//                ret = 0;
//                break;
//            }
//        }
//    }
//    //pthread_mutex_unlock(&(g_mtl_session_mutex));
//
//    return ret; // (-1) can't find out this id in the g_mtl_session[] table
//}

int QSCE_Terminate(void)
{
    //pthread_mutex_destroy(&(g_mtl_session_mutex));
    return 0;
}

int QSCE_RegisterCallback(unsigned long mtl_id, QSTL_CALLBACK_T *callback, unsigned long parent_id)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    memset((void *)&(ptr->callback), 0x0, sizeof(QSTL_CALLBACK_T));
    memcpy(&(ptr->callback), callback, sizeof(QSTL_CALLBACK_T));
    ptr->parent_id = parent_id;
    return 0;
}

int QSCE_SetLocalReceiver(unsigned long mtl_id, unsigned short port,const char *ipaddr)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    ptr->listen_port = port;
    strcpy(ptr->local_ipaddr,ipaddr);
    return 0;
}

int QSCE_SetSendDestination(unsigned long mtl_id, unsigned short port,const char *ipaddr)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    ptr->host_port = port;
    strcpy(ptr->host_ipaddr, ipaddr);
    return 0;
}

int QSCE_SetTFRC(unsigned long mtl_id, int flag)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    if(flag) ptr->tfrc = TRUE;
    else ptr->tfrc = FALSE;
    return 0;
}

/* ------------------------------------------------
 *  Description:
 *      RTP evnet handle (ex. DTMF)
 * ------------------------------------------------
 */
void rtp_evnet_handle(MTL_SESSION_t *ptr, struct TL_Rtcp_Event_t *e)
{
    DTMF_Payload_t *dtmf_payload;
    rtp_packet *pckt_rtp;
    unsigned long timestamp;
    int dtmf_key;
    
    pckt_rtp = (rtp_packet *)e->data;

    if(pckt_rtp->fields.pt == ptr->payload.rtp_event_pt_num) {

        dtmf_payload = (DTMF_Payload_t *)pckt_rtp->meta.data;
        dtmf_key = dtmf_payload->event;
        timestamp = pckt_rtp->fields.ts;

        if(ptr->callback.dtmf_event) {
            ptr->callback.dtmf_event(ptr->parent_id, dtmf_key, timestamp);
        }
        //printf("%s(), dtmf key %d, addr %p\n", __FUNCTION__, dtmf_key, ptr->callback.dtmf_event);
    }    
    else 
        ;//printf("%s(), pt %d != rx pt %d\n", __FUNCTION__, pckt_rtp->fields.pt, ptr->payload.rtp_event_pt_num);
}


/* ------------------------------------------------
 *  Description:
 *      RTP packet app process (ex. RFC4585)
 * ------------------------------------------------
 */
void rtp_packet_app_process(MTL_SESSION_t *ptr, struct TL_Rtcp_Event_t *e)
{
    rtp_packet *pckt_rtp;
    unsigned long timestamp;
    uint16_t seq_num, pkt_len;

    pckt_rtp = (rtp_packet *)e->data;

    timestamp = pckt_rtp->fields.ts;
    seq_num = pckt_rtp->fields.seq;
    pkt_len = pckt_rtp->meta.data_len;

    if(ptr->media_type == QS_MEDIATYPE_VIDEO) {
        if(pckt_rtp->fields.pt == ptr->payload.pt_num) {

            if(ptr->callback.rfc4585_handle) 
                ptr->callback.rfc4585_handle(ptr->parent_id, seq_num, timestamp); /* ptr->session->rtp_session ... */
    
            if(ptr->callback.rtp_rx)
                ptr->callback.rtp_rx(ptr->parent_id, seq_num, timestamp);
        }
    }
    else if(ptr->media_type == QS_MEDIATYPE_AUDIO) {
        if(pckt_rtp->fields.pt == ptr->payload.pt_num) {
            
            if(ptr->callback.rtp_rx)
                ptr->callback.rtp_rx(ptr->parent_id, seq_num, timestamp);
        }
    }
    else ;

    if(ptr->callback.rtp_traffic_monitor) {
        ptr->callback.rtp_traffic_monitor(ptr->media_type, seq_num, pkt_len);
    } 
}

int function_handle_from_transport_layer(MTL_SESSION_t *ptr, struct TL_Rtcp_Event_t *e)
{
    rtp_packet *pckt_rtp;
    unsigned long timestamp;
    int ret = -1, seq_num;

    pckt_rtp = (rtp_packet *)e->data;

    if(ptr->media_type == QS_MEDIATYPE_VIDEO) {
        if(pckt_rtp->fields.pt == ptr->payload.pt_num) {

            timestamp = pckt_rtp->fields.ts;
            seq_num = pckt_rtp->fields.seq;

            if(e->type == TL_CHECK_RESEND_PKT) {
                if(ptr->callback.check_resend_pkt) {
                    ret = ptr->callback.check_resend_pkt(ptr->parent_id, seq_num, timestamp);
                }
            }
        }            
    }
    else if(ptr->media_type == QS_MEDIATYPE_AUDIO) {
        ;
    }

    return ret;
}


/* ------------------------------------------------
 *  Description:
 *      Transport layer callback event dispatch
 *
 *  By Frank (C.Y)
 * ------------------------------------------------
 */
int QSCE_Transport_Callback_Dispatch(struct TL_Session_t *session, struct TL_Rtcp_Event_t *e)
{
    unsigned long net_id;
    MTL_SESSION_t *ptr;
    int ret = -1;
    if (session==NULL)
        return ret;
    if (session->mtl_id == 0)
        return ret;
    if (session->on_stopping)
        return ret;
    /*if(QSCE_Search_Network_SessionID(&net_id, session) < 0) {
        debug_msg("<error> QSCE_Search_Network_SessionID() failed\n");
        return ret;
    }*/
    ptr = (MTL_SESSION_t *)session->mtl_id;

    //printf("==> event:%d, id:%08x\n", e->type, ptr->parent_id);

    ret = 0;
    switch(e->type)
    {
        case TL_RX_SR:
            if(ptr->callback.rtcp_sr)
                ptr->callback.rtcp_sr(ptr->parent_id, (void *)(e->data));            
            break;
        case TL_RX_RR:
            if(ptr->callback.rtcp_rr)
                ptr->callback.rtcp_rr(ptr->parent_id, (void *)(e->data));            
            break;
         case TL_RX_TFRC_RX:
            if(ptr->callback.tfrc_report)
                ptr->callback.tfrc_report(ptr->parent_id, (void *)(e->data));            
            break;
        case  TL_APP_SET_TIME_INFO:  /*for lip sync set time information*/
            if(ptr->callback.timestamp_setting)
                ptr->callback.timestamp_setting(ptr->parent_id);
            break;
        case  TL_APP_SET_RTCP_APP:  /* setting for report to remote side */
            if(ptr->callback.rtcp_app_set)
                ptr->callback.rtcp_app_set(ptr->parent_id);                
            break;
        case RX_APP:
            if(ptr->callback.rtcp_app_receive)
                ptr->callback.rtcp_app_receive(ptr->parent_id, (void *)(e->data));
            break;
        case TL_RX_RTP_EVENT: /* for rtp event handle, frank */
            rtp_evnet_handle(ptr, e);
            break;
        case TL_RX_RTP:
            rtp_packet_app_process(ptr, e);
            break;        
        case TL_CHECK_RESEND_PKT:
            ret = function_handle_from_transport_layer(ptr, e);
            break;
        case TL_RX_RTCP_PSFB:            
            if(ptr->callback.rtcp_psfb_receive)
                ptr->callback.rtcp_psfb_receive(ptr->parent_id, (void *)(e->data));
            break;
        default:
            debug_msg("<error> unknow RTCP type (%d)\n", e->type);
            break;
    } 
    return ret;
}

int QSCE_StartWithSSRC(unsigned long mtl_id, unsigned long ssrc)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;

    ptr->session = TransportLayer_InitwithSSRC(ptr->host_ipaddr, 
                                       ptr->listen_port, 
                                       ptr->host_port, 
                                       ptr->tfrc,
									   ptr->fec,									   
									   ptr->tos,
                                       &ptr->payload, 
                                       QSCE_Transport_Callback_Dispatch, ssrc,
                                       ptr->local_ipaddr);
    if(ptr->session == NULL)
        return -1;
    ptr->session->mtl_id = mtl_id;
    return TransportLayer_Start(ptr->session);
   
    //return 0;

}

int QSCE_Start(unsigned long mtl_id)
{ 
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    PayloadType_t *payload_ptr;

    payload_ptr = (PayloadType_t *)&ptr->payload;
    
    //if(LINUX_DEBUGMSG) printf("%s(0x%08lx), enter\n", __FUNCTION__, mtl_id);  
  
    ptr->session = TransportLayer_Init(ptr->host_ipaddr, 
                                       ptr->listen_port, 
                                       ptr->host_port, 
                                       ptr->tfrc,
									   ptr->fec,
									   ptr->tos,
                                       payload_ptr, 
                                       QSCE_Transport_Callback_Dispatch,
                                       ptr->local_ipaddr);
    
    if(ptr->session == NULL)
        return -1;

    ptr->session->mtl_id = mtl_id;
    return TransportLayer_Start(ptr->session);

    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__); 

    //return 0;
}

int QSCE_Stop(unsigned long mtl_id)
{
    //MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;

    //TransportLayer_TerminateSession(ptr->session);

    return 0;
}

unsigned long QSCE_GetSessionSSRC(unsigned long mtl_id)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;

    return TransportLayer_GetSessionSSRC(ptr->session);
}

int QSCE_ResetSendingBuffer(unsigned long mtl_id)
{
    return 0;
}

int QSCE_SendPLIIndication(unsigned long mtl_id) 	// 請Sender端馬上重送新的GOP
{
    return 0;
}

int QSCE_Set_FEC(unsigned long mtl_id, 
                 int ctrl, 
                 int send_payloadType, int receive_payloadType)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;	

 //   if(LINUX_DEBUGMSG) printf("%s(%d,%d,%d), enter\n", __FUNCTION__, ctrl, send_payloadType, receive_payloadType);
    if(mtl_id == 0) return -1;

    ptr->fec.ctrl = ctrl;
    ptr->fec.send_payloadType = send_payloadType;
    ptr->fec.receive_payloadType = receive_payloadType;


 //   if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);
    return 0;
}


/*implemented by willy, 2010-08-19*/
int QSCE_Set_Maximum_Payload_Size(unsigned long mtl_id, int maximum_payload_size)	// default: 1380 bytes
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    
    //if(LINUX_DEBUGMSG) printf("%s(%d), enter\n", __FUNCTION__, maximum_payload_size);

    if(mtl_id == 0) return -1;

    if((maximum_payload_size < 100)||(maximum_payload_size > 1500))
        return -1;

    ptr->session->maximum_payload_size = maximum_payload_size;
   
    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

    return 0;
}


int QSCE_Set_Network_Payload(unsigned long mtl_id, int clock_rate, int codec_type, char *mime_type, uint16_t rtp_event_pt_num)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;

    //if(LINUX_DEBUGMSG) printf("%s(%d,%d,%d,%s), enter\n", __FUNCTION__, clock_rate, codec_type, rtp_event_pt_num, mime_type);

    if(mtl_id == 0) return -1;

    ptr->payload.pt_num = codec_type;
    ptr->payload.clock_rate = clock_rate;
    ptr->payload.rtp_event_pt_num = rtp_event_pt_num;
    strcpy(ptr->payload.mime_type, mime_type); 

    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

    return 0;
}


int QSCE_SetTFRCMinSendingRate(unsigned long mtl_id, unsigned long min_sending_rate)
{
	MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	int ret;

    //if(LINUX_DEBUGMSG) printf("%s(%lu), enter\n", __FUNCTION__, min_sending_rate);

	if(ptr->session == NULL) 
        return -1;

	ret = TransportLayer_SetTFRCMinSendingRate(ptr->session, min_sending_rate);		/* uint of min_sending_rate: kbps */

    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

    return ret;
}


/* Enable to send RTP */ /* add by willy, 2010-10-01 */
int QSCE_Send_RTP_Enable(unsigned long mtl_id)
{
	MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	
    //if(LINUX_DEBUGMSG) printf("%s(), enter\n", __FUNCTION__);

	if(ptr->session == NULL) 
	    return -1;
	else
	    ptr->session->send_rtp_flag = 1;
	
    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

	return 0;
}


/* Disable to send RTP */ /* add by willy, 2010-10-01 */
int QSCE_Send_RTP_Disable(unsigned long mtl_id)
{
	MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	
    //if(LINUX_DEBUGMSG) printf("%s(), enter\n", __FUNCTION__);

	if(ptr->session == NULL) 
	    return -1;
	else
	    ptr->session->send_rtp_flag = 0;

    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

	return 0;

}


int QSCE_Set_Socket_Option(int option_name, const void *option_value)
{
	if (option_value == NULL)
	    return -1;
	
	switch (option_name) {
	    case QSCE_SOCKET_OPTION_TOS:
		udp_set_socket_option(UDP_SOCKET_OPTION_TOS, option_value);
		break;
	    
	    default:
		return -1;
	}

	return 0;
}


void QSCE_Set_Audio_Tos(int tos)
{
	g_audio_tos = tos;
}


void QSCE_Set_Video_Tos(int tos)
{
	g_video_tos = tos;
}

int QSCE_Set_TOS(unsigned long mtl_id, int tos)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
    
    //if(LINUX_DEBUGMSG) printf("%s(%d), enter\n", __FUNCTION__, tos);
    
    if(mtl_id == 0) return -1;
    
    ptr->tos = tos;
    
    //if(LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);
    
    return 0;
}


int QSCE_Disable_RTCP(unsigned long mtl_id)
{

    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	
	if(ptr->session == NULL) 
	    return -1;

    return TransportLayer_DisableRTCP(ptr->session);
}


int QSCE_Enable_RTCP(unsigned long mtl_id)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	
	if(ptr->session == NULL) 
	    return -1;

    return TransportLayer_EnableRTCP(ptr->session);
}


int QSCE_Set_Video_IPI(unsigned long mtl_id, int ipi)
{
    MTL_SESSION_t *ptr = (MTL_SESSION_t *)mtl_id;
	
	if(ptr->session == NULL) 
	    return -1;

    return TransportLayer_Set_Video_InterPacketInterval(ptr->session, ipi);
}
