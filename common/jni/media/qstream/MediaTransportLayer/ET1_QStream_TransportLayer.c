/*
 * FILE:     ET1_QStream_TransportLayer.c
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED:
 *
 *
 *
 */

#define THREADS_CREATE_ERR 1
#define MAX_LOST_SEQ 10 //Corey
#define MAX_PATH     10
#define AudioRecvBufferNumber 15

#define LINUX_FEC             1

#include "../GlobalDefine.h"
#include "ET1_QStream_TransportLayer.h"
#include "ET1_QStream_RFC3984.h"
#include "tv.h"
#include "debug.h"
#include "rtp_callback.h"
#include "rtp.h"


//static struct TL_Session_t *m_tl_session[MAX_TL_SESSION_NUM];
//static int m_tl_session_num = 0;

//extern void QS_videoLatencyAnalyse( int node ); //QStream_Utility.h
/****************************************************************************
* Funciton Name: TransportLayer_SendData
* Parameters:
* Author:        Ian Tuan
* Description:
*****************************************************************************/
int TransportLayer_SendData(struct TL_Session_t *session, TL_Send_Para_t *para)
{
#if 0 // disalbe dtmf outband handle (FIXME, move outband handle to QStream_Intellignet_Audio.DTMF.c)
    DTMF_Payload_t dtmf_payload;
    uint16_t duration;

    int marker;

    if (para->rtp_event_data >= 0)
    {
        if (session->get_dtmf_key)
        {
            TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "TransportLayer Error : Get a new dtmf key %d while sending a previous dtmf key %d\n", para->rtp_event_data, session->dtmf_key);
        }
        else
        {
            /*a new dtmf event coming*/
            session->get_dtmf_key = 1;
            session->dtmf_key = para->rtp_event_data;
            session->dtmf_key_ts = para->timestamp; /*the same dtmf key use the same timestamp*/
        }
    }

    if (session->get_dtmf_key)
    {
        duration = session->payload_type->clock_rate/DTMF_PACKTS_PER_SEC;

        if (session->dtmf_sent_cnt >= DTMF_EVENT_COUNTS)
        {
            dtmf_payload.event = (unsigned char)(session->dtmf_key);
            dtmf_payload.e = 1; /*raise the end bit*/
            dtmf_payload.r = 0;
            dtmf_payload.volume = DTMF_FIXED_VOLUME;
            dtmf_payload.duration = htons(duration * (DTMF_EVENT_COUNTS + DTMF_EVENT_END_COUNTS));
        }
        else
        {
            dtmf_payload.event = (unsigned char)(session->dtmf_key);
            dtmf_payload.e = 0;
            dtmf_payload.r = 0;
            dtmf_payload.volume = DTMF_FIXED_VOLUME;
            dtmf_payload.duration = htons(duration * (session->dtmf_sent_cnt + 1));
        }
    }

    SENDING_BUF_LOCK(session->sending_buffer);

    if (session->get_dtmf_key)
    {
        marker = (session->dtmf_sent_cnt) ? 0 : 1; /*if dtmf_sent_cnt = 0 then set mark bit*/

        SendingBuf_Insert(session->sending_buffer, 
                          &dtmf_payload, 
                          sizeof(DTMF_Payload_t), 
                          session->dtmf_key_ts, 
                          marker, 
                          para->ext_hdr_data,
                          para->ext_len, 
                          1);
        session->dtmf_sent_cnt++;
    }
    else
    {
        /*directly insert the data into sending buffer*/
        SendingBuf_Insert(session->sending_buffer, 
                          para->frame_data, 
                          para->length, 
                          para->timestamp, 
                          0, 
                          para->ext_hdr_data, 
                          para->ext_len, 
                          0);
    }

    SENDING_BUF_UNLOCK(session->sending_buffer);

    if (session->get_dtmf_key)
    {
        if (session->dtmf_sent_cnt >= (DTMF_EVENT_COUNTS + DTMF_EVENT_END_COUNTS))
        {
            session->get_dtmf_key = 0;
            session->dtmf_sent_cnt = 0;
            para->rtp_event_data = QS_DTMF_END_SENDING;
        }
    }
    return 1;
#else

    SENDING_BUF_LOCK(session->sending_buffer);

    SendingBuf_Insert(session->sending_buffer,
                      para->frame_data,
                      para->length,
                      para->timestamp,
                      para->mark,
                      para->ext_hdr_data,
                      para->ext_len,
                      para->rtp_event_data);

    SENDING_BUF_UNLOCK(session->sending_buffer);

    return 1;

#endif

}

/****************************************************************************
* Funciton Name: TransportLayer_RecvData
* Parameters:
* Author:        Ian Tuan
* Description:
*****************************************************************************/
int TransportLayer_RecvData(struct TL_Session_t *session, TL_Recv_Para_t *para)
{
    pdb_e         *cp = NULL;
    pbuf_node     *curr_frame;
    rtp_packet    *packet;

    para->length = 0;
    para->partial_flag = 0; /*audio data doesn't have partial frame issue*/

    //cp = pdb_iter_init(session->participants);
    if (session->has_opposite_ssrc)
        cp = pdb_get(session->participants, session->opposite_ssrc);
    else
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));

    if (!cp)
        return 0;

    RECVING_BUF_LOCK(cp->playout_buffer);

    if (!(cp->playout_buffer->frst))
    {
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return 0;
    }
	
	/* Queue enough packets to perform FEC recovery */
    if(session->ec_on && cp->playout_buffer->size < AudioRecvBufferNumber)
    {
        //TL_DEBUG_MSG("audio playout_buffer is buffering")
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return -1; 
    }

    curr_frame = cp->playout_buffer->frst;

    para->timestamp = curr_frame->rtp_timestamp;

    packet = curr_frame->cdata->data;

    memcpy(para->frame_data, packet->meta.data, packet->meta.data_len);

    para->length = packet->meta.data_len;

    pbuf_remove_first(cp->playout_buffer);

    RECVING_BUF_UNLOCK(cp->playout_buffer);

    return 1;
}


/****************************************************************************
* Funciton Name: TransportLayer_TransThread
* Parameters:
* Author:        Ian Tuan
* Description:
*****************************************************************************/
static void *TransportLayer_TransThread(void *arg)
{
    PayloadData_t *payload = NULL;
    int r = 0;
    uint32_t ts;
    struct TL_Session_t *session = NULL;
    int end;
    int first_send = TRUE;
    unsigned char data[4] = {0x00, 0x00, 0x00, 0x00};
    int heart_beat_count = 0;
    int buf_size;
    int get_next = TRUE;
    int ret;
    //struct timeval sem_tv;
    //struct timespec sem_ts;
    uint16_t sent_seq;
    uint16_t pt_num = 0;

    session = (struct TL_Session_t*)arg;

//	extern void log_pid(const char* func, pid_t tid);
//	log_pid(__FUNCTION__, gettid());
	
    if (!session)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "Sending Buffer is NULL, Transport Layer init error\n");
        return NULL;
    }

    while (1)
    {
#ifdef WIN32
        //Sleep(1); //sem_trywait use WaitForSingleObject 1 msec
#else
        if(session->payload_type->type == PAYLOAD_VIDEO) usleep(1000); //usleep(session->video_ipi);
        else usleep(session->audio_ipi);
#endif

        if (session->on_stopping!=0) //if (sem_trywait(&(session->trans_terminate_sem)) == 0)
        {
            TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "session %p Trans thread recv terminate signal\n", session);
            //sem_destroy(&(session->trans_terminate_sem));
            break;
        }

        if (first_send)
        {
            if (get_next)
            {
                /*get the first out packet from senidng buffer*/
                payload = SendingBuf_GetNextToSend(session->sending_buffer, &ts, &end);
                if (!payload)
                {
                    heart_beat_count ++;
                    /*the sending buffer is empty, wait next time*/
                    continue;
                }
                else
                {
                    get_next = FALSE;

                    /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
                    if (session->send_rtp_flag)
                    {
                        if (payload->resend)
                            TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK2, "payload->resend=%d\n",payload->resend);
                    }
                }
            }

            if (payload->resend)
            {
                TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK2, "transport layer payload resend payload = %p resend = %d\n", payload, payload->resend);
                /*this is the sent payload, but reciver request us to resend*/

                /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
                if (session->send_rtp_flag)
                {
                    r = rtp_resend_data(session->rtp_session,
                                        payload->seq,
                                        ts,
                                        session->payload_type->pt_num,
                                        payload->marker,
                                        0,
                                        0,
                                        payload->data,
                                        payload->data_len,
                                        payload->ext_hdr_data,
                                        (payload->ext_len)/4,
                                        1);
                }
            }
            else if (payload->redundant_resend)
            {
                /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
                if (session->send_rtp_flag)
                {
                    r = rtp_resend_data(session->rtp_session,
                                        payload->seq,
                                        ts,
                                        session->payload_type->pt_num,
                                        payload->marker,
                                        0,
                                        0,
                                        payload->data,
                                        payload->data_len,
                                        payload->ext_hdr_data,
                                        (payload->ext_len)/4,
                                        1);
                }
            }
            else
            {
                /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
                if (session->send_rtp_flag)
                {
                    if (payload->rtp_event_flag)
                        pt_num = session->payload_type->rtp_event_pt_num;
                    else
                        pt_num = session->payload_type->pt_num;
                }

                /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
                if (session->send_rtp_flag)
                { 
                    r = rtp_send_data(session->rtp_session,
                                      ts,
                                      pt_num,
                                      payload->marker,
                                      0,
                                      0,
                                      payload->data,
                                      payload->data_len,
                                      &sent_seq,
                                      payload->ext_hdr_data,
                                      (payload->ext_len)/4,
                                      1);
                }
            }
        }
        else
        {
            /* if the send_rtp_flag is enable, send the rtp packet; otherwise, don't send the rtp packet */ /* add by willy, 2010-10-01 */
            if (session->send_rtp_flag)
            {
                r = rtp_send_data(session->rtp_session,
                                  1000,
                                  1,
                                  0,
                                  0,
                                  0,
                                  data,
                                  4,
                                  &sent_seq,
                                  0,
                                  0,
                                  0);

                if (r > 0)
                    first_send = TRUE;
            }

            continue;
        }

        if (session->payload_type->type == PAYLOAD_VIDEO)
        {

            if (payload->redundant_resend)
                get_next = TRUE;

            if(r > 0){// make sure the rtp packet is sent out
             ret = SendingBuf_RemoveOutofDate(session->sending_buffer, payload->marker);
            }

            if (ret > 1)
            {
                get_next = TRUE;
            }
            else if (r >0)
            {
                get_next = TRUE;

                payload->seq = sent_seq;
                payload->sent = TRUE;

                //if(end)
                //  SendingBuf_MoveToNextFrame(session->sending_buffer);
            }
        }
        else
        {
            //SendingBuf_MoveToNextFrame(session->sending_buffer);
            SendingBuf_RemoveSent(session->sending_buffer);
            get_next = TRUE;
        }

        if (session->payload_type->type == PAYLOAD_VIDEO)
        {
            heart_beat_count ++;

            if (heart_beat_count > 3000)
            {
                buf_size = GetSendingBufSize(session->sending_buffer);

                TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK5, "video sending buf_size = %d\n", buf_size);

                heart_beat_count = 0;

            }
        }

    }/*end while*/
    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "transport layer trans thread terminated\n");
    pthread_exit(NULL);

    return NULL;
}

/****************************************************************************
* Funciton Name: TransportLayer_PollData                                     
* Parameters: 
* Author:        Paul Weng
* Description:
*****************************************************************************/
int TransportLayer_PollData(struct TL_Session_t *session, TL_Poll_Para_t *para)
{
    pdb_e         *cp = NULL;
    pbuf_node     *curr_frame = NULL;
    rtp_packet    *packet = NULL;

    if (session->has_opposite_ssrc)
    {   
        cp = pdb_get(session->participants, session->opposite_ssrc);
    }
    else
    {
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));
    }

    if (!cp)
    {
        return -1;
    }
     
    RECVING_BUF_LOCK(cp->playout_buffer);     
    if (!(cp->playout_buffer->frst))
    {        
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        para->frame_buffer_size = 0;
        /*the buffer is empty so return -1*/
        return -1;
    }   

    para->frame_buffer_size = cp->playout_buffer->size;    
    curr_frame = cp->playout_buffer->frst;
    if (curr_frame)
    {
        para->timestamp = curr_frame->rtp_timestamp; 
        packet = (rtp_packet*)curr_frame->cdata->data;        
    }
    else
    {
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return -1;
    }

    /*get the exten data*/  
    if (packet && packet->meta.extn && packet->meta.extn_len > 0)
    {       
        /*
            0                     1                 2                   3
            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
     extn-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |      defined by profile       |            length             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |          header extension(AUDIO_RTP_Eextern_Info_t)           |
            |                              ....                             |

            typedef struct {
                uint32_t timestamp;
                uint32_t frame_len;
                uint8_t  buf[MAX_AUDIO_FRAME_SZIE];
            } AUDIO_RTP_Eextern_Info_t;
        */
        uint32_t timestamp;
        uint32_t frame_len;
        uint8_t* buf = packet->meta.extn + 12;
        
        memcpy(&timestamp, packet->meta.extn + 4, 4);
        memcpy(&frame_len, packet->meta.extn + 8, 4);

        para->timestamp = ntohl(timestamp);
        para->ext_len = ntohl(frame_len);    
        memcpy(para->ext_hdr_data, buf, para->ext_len);
    }
    else
    {
        para->ext_len = 0;
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return -1;
    }

    RECVING_BUF_UNLOCK(cp->playout_buffer);
    return 0;    
}

/****************************************************************************
* Funciton Name: TransportLayer_RecoverData                                     
* Parameters: 
* Author:        Corey Lin
* Description: FEC, traverse all the packets in pbuf to find all lost packet sequence
*****************************************************************************/
int TransportLayer_RecoverData(struct TL_Session_t *session)
{
	pdb_e         *cp = NULL;
    pbuf_node     *curr_frame = NULL;
	coded_data    *curr_cdata, *prev_cdata; 
    rtp_packet    *packet = NULL;
	uint16_t lost_seq[MAX_LOST_SEQ];
	int lost_count = 0;
	int max_size = MAX_LOST_SEQ;
    int init_status;
    uint16_t detect_seq;

	if( !ea_fec_isOn(session->ea) ) return 0;//do nothing if fec is disabled	

    if (session->has_opposite_ssrc){   
        cp = pdb_get(session->participants, session->opposite_ssrc);
    }
    else{
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));
    }

    if (!cp){
        return -1;
    }
     
    RECVING_BUF_LOCK(cp->playout_buffer);     
    if (!(cp->playout_buffer->frst)){        
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        /*the buffer is empty so return -1*/
        return -1;
    }   

    curr_frame = cp->playout_buffer->frst;

    


    if(session->payload_type->type == PAYLOAD_VIDEO){
        /* Video Session */
        /* traverse the first frame */

        if(curr_frame != NULL){

            prev_cdata = NULL;
            curr_cdata = (coded_data*)curr_frame->cdata; 


            /*Because the traverse method as below cannot find out two packet loss cases:*/
            /*1. packet lost at the first packet in frame*/
            /*2. packet lost at the last packet in frame*/
            /*Check if packet loss occurs at the first packet in frame*/
            if(cp->playout_buffer->last_removed_frame_max_seq == -1) 
                init_status = 1;
            else 
                init_status = 0;

            detect_seq = curr_frame->seqno_min - 1u;
            if( init_status == 0 && cp->playout_buffer->last_removed_frame_max_seq  != detect_seq ){
                lost_seq[lost_count] = detect_seq;
                lost_count++;
            }

            detect_seq = curr_frame->seqno_max + 1u;
            if(curr_frame->next && curr_frame->next->seqno_min != detect_seq ){
                lost_seq[lost_count] = detect_seq;
                lost_count++;
            }

            //traverse all packets of the oldest frame in pbuf
            while(curr_cdata != NULL){
                if(prev_cdata != NULL){
                    uint16_t temp = prev_cdata->seqno - 1u;

                    if( curr_cdata->seqno != temp ){
                        uint16_t seq_lost = temp;
                        lost_seq[lost_count] = temp;
                        lost_count++;
                    }
                }

                if(lost_count >= max_size) break;

                prev_cdata = curr_cdata;
                curr_cdata = curr_cdata->next;
            }     

        }
    }else{
       
        /* Audio Session */


        pbuf_node   *prev_frame = NULL;
        int count_path=0;

        //traverse all packets in pbuf
        while(curr_frame != NULL){
            if(prev_frame != NULL){
                uint16_t seq1 = prev_frame->cdata->seqno + 1u;
                uint16_t seq2 = curr_frame->cdata->seqno;
                if( seq1 != seq2){
                    uint16_t seq_lost = seq1;
                    lost_seq[lost_count] = seq_lost;
                    lost_count++;
                }
            
            }
            
            if(count_path >= MAX_PATH) break;
            if(lost_count >= max_size) break;

            prev_frame = curr_frame;
            curr_frame = curr_frame->next;
            count_path++;
        }
    }

    RECVING_BUF_UNLOCK(cp->playout_buffer);

	int i=0;
	for(i=0;i<lost_count;i++){
		//printf("%s() - Try to recover RTP Packet: %d \n",__func__,lost_seq[i]);
		rtp_recover(session->rtp_session, lost_seq[i]);
	}

    return 0; 
	

	

}



/****************************************************************************
 * Funciton Name: TransportLayer_GetTLSession
 * Parameters:
 * Author:        Ian Tuan
 * Description: for caller that has rtp session pointer and want to
 *              find the corresponding tl session
 * History
 *              1. frank(c.y.), modify to supprot debug message
 *              2. frank(c.y.), modify to support ring table
 *****************************************************************************/
struct TL_Session_t  *TransportLayer_GetTLSession(struct rtp *session)
{
    struct TL_Session_t *tl_session = NULL;
    if (session!=NULL)
        tl_session = rtp_get_tl_session(session);
    /*int i = 0;
    struct TL_Session_t *tl_session = NULL;
         
    //for(i=0; i < m_tl_session_num;i++)
    for(i = 0; i < MAX_TL_SESSION_NUM; i++)  // frank modify to support ring table 
    {
        tl_session = m_tl_session[i];
        if(tl_session == NULL)
            continue;
     
        if(tl_session->rtp_session == session) {
            if(!(tl_session->rtcp_callback)) {
                printf("<dbg> tl_session->rtcp_callback is invaild\n"); 
            }
            return tl_session;
        }
    }      

    printf("<dbg> tl_session is invaild, target %p, %s()\n", session, __FUNCTION__);
    for(i = 0; i < MAX_TL_SESSION_NUM; i++) {
        if(m_tl_session[i] != NULL) printf("[%d]%p->%p, ", i, m_tl_session[i], m_tl_session[i]->rtp_session);   
    }
    printf("\n\n");   
   */
    return tl_session; 
 }

/****************************************************************************
 * Funciton Name: TransportLayer_FreeMTLSession
 * Parameters:
 * Author:      Frank (C.Y.)
 * Description: for caller that has rtp session pointer and want to free
 *              m_tl_session[] table
 *****************************************************************************/
static int TransportLayer_FreeMTLSession(struct TL_Session_t *session)
{
    /*int i = 0;
    struct TL_Session_t *tl_session = NULL;
         
    for(i = 0; i < MAX_TL_SESSION_NUM; i++)
    {        
        tl_session = m_tl_session[i];
        if(tl_session == NULL)
            continue;
  
        if(tl_session == session) {
            printf("%s(), Free MTLSession[%d], session %p\n", __FUNCTION__, i, session); 
            m_tl_session[i] = NULL;
            return i;
        }
    }  
    
    printf("<warning> can't find out corresponding entry, target %p, %s()\n", session, __FUNCTION__);
    for(i = 0; i < MAX_TL_SESSION_NUM; i++) {
        if(m_tl_session[i] != NULL) printf("[%d]%p->%p, ", i, m_tl_session[i], m_tl_session[i]->rtp_session);   
    }
    printf("\n\n");*/

    return -1;
}

/****************************************************************************
 * Funciton Name: TransportLayer_GetEmptyMTLSessionIndex
 * Parameters:
 * Author:      Frank (C.Y.)
 * Description: get an empty table entry to register new rtp session
 *****************************************************************************/
/*static int TransportLayer_GetEmptyMTLSessionIndex(void)
{
    int i = 0;
    struct TL_Session_t *tl_session = NULL;

    for (i=0; i < MAX_TL_SESSION_NUM; i++)
    {
        tl_session = m_tl_session[i];
        if (tl_session == NULL)
            return i;
    }
    return -1;
}*/


/****************************************************************************
 * Funciton Name: TransportLayer_SetAppInfo
 * Parameters:
 * Author:      Ian
 * Description: When upper layer get rtcp app callback, the can use this function
 *              to write the info they want rtcp to send
 *****************************************************************************/
int TransportLayer_SetAppInfo(struct TL_Session_t *session, uint8_t *appdata, uint16_t length)
{
    rtcp_app *app_info;

    if (length > (MAX_RTCP_APP_LEN - 12))
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "TransportLayer_SetAppInfo : app length over size\n");
        return -1;
    }

    if (length % 4)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "TransportLayer_SetAppInfo : app length is not mutilple of 4");
        return -1;
    }

    app_info = (rtcp_app*)(session->app_buf);

    memcpy(app_info->data, appdata, length);
    session->app_len = length;

    return 1;
}

/****************************************************************************
* Funciton Name: TransportLayer_RTCP_APP_Callback
* Parameters:  session : transport layer session pointer
                      rtp_ts : to avoid set twice app info in same rtcp packet
                      max_size: to avoid the app info over limit size

* Author:        Ian Tuan
* Description: this function will be called by send_rtcp() in rtp.c, the purpose is to
  notify the upper layer to set app info in the rtcp packet. However this function is
  just a event trigger, the upper layer can use TransportLayer_SetAppInfo to write the data,
  and it will packed those data.
*****************************************************************************/
rtcp_app* TransportLayer_RTCP_APP_Callback(struct rtp *session, uint32_t rtp_ts, int max_size)
{
    struct TL_Session_t *tl_session;
    struct TL_Rtcp_Event_t *event=NULL;
    rtcp_app *app_info = NULL;

    tl_session = TransportLayer_GetTLSession(session);

    if (!tl_session)
        return NULL;

    if (!(tl_session->rtcp_callback))
        return NULL;
     if(tl_session->on_stopping)
     {
        return NULL;
     }

    memset(tl_session->app_buf,0 , MAX_RTCP_APP_LEN);
    tl_session->app_len = 0;

    if (tl_session->last_app_rtpts == rtp_ts)
        return NULL;

    tl_session->last_app_rtpts = rtp_ts;

    event = (struct TL_Rtcp_Event_t*)malloc(sizeof(struct TL_Rtcp_Event_t));
    if (NULL == event)
    {
        return NULL;
    }
    event->tl_session = tl_session;
    //event->ssrc = e->ssrc;
    event->type = TL_APP_SET_RTCP_APP;
    event->data = NULL;

    tl_session->rtcp_callback(tl_session, event);

    /* the upper layer may not request to send application info every time*/
    if (!(tl_session->app_len))
    {
        free(event);
        return NULL;
    }

    app_info = (rtcp_app*)(tl_session->app_buf);
    app_info->length = (tl_session->app_len)/4 + 2; /*from octects to word length*/
    app_info->p = 0;
    app_info->subtype = 0;
    strcpy(app_info->name,"qua");

    free(event);

    return app_info;
}

/****************************************************************************
 * Funciton Name: TransportLayer_SendGenericNACK
 * Parameters:
 * Author:        Frank Ting
 * Description:   Send transport layer FB messages (generic nack)
 *****************************************************************************/
static int TransportLayer_SendGenericNACK(struct TL_Session_t *session, int pid, int bitmask)
{
    send_rtpfb_generic_nack(session->rtp_session, pid, bitmask);
    return 0;
}


/****************************************************************************
 * Funciton Name: TransportLayer_Send_SLI
 * Parameters:
 * Author:        Frank Ting
 * Description:   Send transport layer FB messages (slice loss indication)
 *****************************************************************************/
static void TransportLayer_Send_SLI(struct TL_Session_t *session, uint16_t first, uint16_t number, uint8_t picture_id)
{
    send_psfb_slice_loss_indication(session->rtp_session, first, number, picture_id);
}


/****************************************************************************
 * Funciton Name: TransportLayer_Send_PLI
 * Parameters:
 * Author:        Frank Ting
 * Description:   Send transport layer FB messages (picture loss indication)
 *****************************************************************************/
static void TransportLayer_Send_PLI(struct TL_Session_t *session)
{
    send_psfb_picture_loss_indication(session->rtp_session);
}


/****************************************************************************
 * Funciton Name: TransportLayer_Send_FIR
 * Parameters:
 * Author:        Frank Ting
 * Description:   Send transport layer FB messages (full intra request)
 *****************************************************************************/
static void TransportLayer_Send_FIR(struct TL_Session_t *session)
{
    send_psfb_full_intra_request(session->rtp_session);
}



/****************************************************************************
 * Funciton Name: TransportLayer_DestroyProcess
 * Parameters:
 * Author:        Ian
 * Description:
 *****************************************************************************/
static void free_sdes_string(pdb_e*	sdes_item)
{
	if (sdes_item->sdes_cname != NULL) 		
	{
		free(sdes_item->sdes_cname);
	}	
	
	if (sdes_item->sdes_name != NULL) 
	{
		free(sdes_item->sdes_name);
	}	
	
	if (sdes_item->sdes_email != NULL) 
	{
		free(sdes_item->sdes_email);
	}	
	
	if (sdes_item->sdes_phone != NULL)
	{
		free(sdes_item->sdes_phone);
	}	
	if (sdes_item->sdes_loc != NULL) 
	{
		free(sdes_item->sdes_loc);
	}	
	
	if (sdes_item->sdes_tool != NULL)
	{
		free(sdes_item->sdes_tool);
	}	
	
	if (sdes_item->sdes_note != NULL) 
	{
		free(sdes_item->sdes_note);
	}

	return ;
}
 
 static void TransportLayer_DestroyProcess( struct TL_Session_t *session)
{
    pdb_e*	remote_sdes_item = NULL;
    pdb_e*  local_sdes_item = NULL;
	
    /*free sending buffer*/
    SendingBuf_Destroy(session->sending_buffer);

    /*free playout buffer*/
    if (session->has_opposite_ssrc)
    {
    	remote_sdes_item  = pdb_get(session->participants, session->opposite_ssrc);
    	if (remote_sdes_item)
    	{
			pbuf_done(remote_sdes_item->playout_buffer);
			pdb_remove(session->participants, session->opposite_ssrc, &remote_sdes_item);
			free_sdes_string(remote_sdes_item);
			free(remote_sdes_item);
    	}
    }
	
    local_sdes_item  = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));
	if (local_sdes_item)
	{
		pbuf_done(local_sdes_item->playout_buffer);
		pdb_remove(session->participants, rtp_my_ssrc(session->rtp_session), &local_sdes_item);
		free_sdes_string(local_sdes_item);
		free(local_sdes_item);
	}
    
    rtp_done(session->rtp_session);

    pdb_destroy(&(session->participants));
	
	ec_agent_destroy(session->ea);

    free(session->payload_type);

    free(session);

}


/****************************************************************************
* Funciton Name: TransportLayer_RecvThread
* Parameters:
* Author:        Ian Tuan
* Description:
*****************************************************************************/
static void *TransportLayer_RecvThread(void *arg)
{
    struct timeval	 curr_time;
    struct timeval	 timeout;
    uint32_t ts;
    struct TL_Session_t *session = NULL;
    int heart_beat_count = 0;
	int isRecvData = FALSE;

	//extern void log_pid(const char* func, pid_t tid);
	//log_pid(__FUNCTION__, gettid());
	
    session = (struct TL_Session_t*)arg;

    /* Receive packets from the network... The timeout is adjusted */
    /* to match the video capture rate, so the transmitter works.  */
    timeout.tv_sec  = 0;
    timeout.tv_usec = 100; /* huh? */

#if 0
    /*setup start time, add by Willy, 2010-01-11*/
    //session->start_time.tv_sec = 0;
    //session->start_time.tv_usec = 0;
#else
    /* modify by Frank.T ++ */
    gettimeofday(&(session->start_time), NULL);
    /* modify by Frank.T -- */
#endif


    while (1)
    {
        if (session->on_stopping!=0) //if (sem_trywait(&(session->recv_terminate_sem)) == 0)
        {
            TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "session %p recv thread recv terminate signal\n", session);
            //sem_destroy(&(session->recv_terminate_sem));
            //TransportLayer_DestroyProcess(session);
            break;
        }

        gettimeofday(&curr_time, NULL);

#if 0
        if (session->payload_type->pt_num == 99)
            ts = tv_diff(curr_time, session->start_time) * 90000;
        else
            ts = tv_diff(curr_time, session->start_time) * 8000;
#else
        /* Frank.T modify ++ */
        if (session->payload_type->type == TYPE(PAYLOAD_VIDEO))
            ts = tv_diff(curr_time, session->start_time) * 90000;
        else
            ts = tv_diff(curr_time, session->start_time) * 8000;

        /* if ts overflow to reset start_time to current time, Frank.T ++ */
        if (ts == (uint32_t)0xffffffff)
        {
            gettimeofday(&session->start_time, NULL);
            ts = 0;
            printf("<info> ts overflow, reset start_time\n");
        }
        /* Frank.T modify -- */
#endif

        rtp_update(session->rtp_session, curr_time);
        rtp_send_ctrl(session->rtp_session, ts,  TransportLayer_RTCP_APP_Callback, curr_time);

        /*
         * Receive packets from the network...
         */
        isRecvData = rtp_recv(session->rtp_session, &timeout, ts);

   
		/* Corey:
		 * Try to recover packets
		 * */
        if(isRecvData)
            TransportLayer_RecoverData(session);

        heart_beat_count ++;

        if (heart_beat_count > 10000)
        {
            //TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK5, "TransportLayer_RecvThread heart beat\n");
            heart_beat_count = 0;
        }

#ifdef WIN32
        //Sleep(1); //sem_trywait use WaitForSingleObject 1 msec
#else
        usleep(1000);
#endif
    }

    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "transport layer recv thread terminated\n");

    pthread_exit(NULL);

    return NULL;
}



static void TransportLayer_Register_Method(struct TL_Session_t *session)
{
    switch (session->payload_type->type)
    {
        case PAYLOAD_VIDEO:
            session->send_data = RFC3984_Packetize;
            session->recv_data = RFC3984_Unpacketize;
            session->poll_data = RFC3984_FrameComplete;
            session->send_generic_nack = TransportLayer_SendGenericNACK;
            session->send_rfc4585_pli = TransportLayer_Send_PLI;
            session->send_rfc4585_sli = TransportLayer_Send_SLI;
            session->send_rfc5104_fir = TransportLayer_Send_FIR;
            break;
        case PAYLOAD_AUDIO_PACKETIZED:
            session->send_data = TransportLayer_SendData;
            session->recv_data = TransportLayer_RecvData;
            session->poll_data = TransportLayer_PollData;
            session->send_generic_nack = NULL;
            session->send_rfc4585_pli = NULL;
            session->send_rfc4585_sli = NULL;
            session->send_rfc5104_fir = NULL;
            break;
        default:
            break;
    }
}


/****************************************************************************
 * Funciton Name: TransportLayer_InitwithSSRC
 * Parameters:
 * Author:        Ian Tuan
 * Description:  The rtp profile specific the ssrc is radomly choosed, but we allow
                        upper layer has the right to set the ssrc here
 *****************************************************************************/
struct TL_Session_t *TransportLayer_InitwithSSRC(const char *addr, 
                                                 uint16_t rx_port, 
                                                 uint16_t tx_port, 
                                                 int tfrc_on, 
												 TL_FECControl_t fec,
                                                 int tos,
                                                 PayloadType_t *payload_type,  
                                                 tl_rtcp_callback callback, 
                                                 uint32_t ssrc,
                                                 const char *local_addr)
{
    double rtcp_bw = 5 * 1024 * 1024;
    struct TL_Session_t *new_session;
    PayloadType_t *my_pl_type;
    int resend_enable = 0, ring_table_index;
	RTP_ECControl_t ec_arg;
    int tl_fec_on = 0, tl_ec_on = 0;


    /* ring table, frank(c.y.) modify */
    /*ring_table_index = TransportLayer_GetEmptyMTLSessionIndex();
    if (ring_table_index < 0 || ring_table_index >= MAX_TL_SESSION_NUM)
    {
        printf("<warning> can't get empty entry in the m_tl_session[], %s(%d)\n", __FUNCTION__, __LINE__);
        return NULL;
    }*/

    new_session = (struct TL_Session_t*)malloc(sizeof( struct TL_Session_t));
    if (NULL == new_session)
    {
        return NULL;
    }

    new_session->participants = pdb_init(payload_type->pt_num);

    if (new_session->participants != NULL)
    {
        /*initial the av profile part*/
        my_pl_type = (PayloadType_t*)malloc(sizeof(PayloadType_t));
        if (NULL == my_pl_type)
        {
            return NULL;
        }
        memcpy(my_pl_type, payload_type, sizeof(PayloadType_t));

        new_session->payload_type = my_pl_type;

        TransportLayer_Register_Method(new_session);

        if (new_session->payload_type->type == PAYLOAD_VIDEO)
            resend_enable = 1;//tfrc_on

        new_session->dtmf_key = 0;
        new_session->get_dtmf_key = 0;
        new_session->dtmf_sent_cnt = 0;

        if (callback)
            new_session->rtcp_callback = callback;
        else
            new_session->rtcp_callback = NULL;


        if( LINUX_FEC && fec.ctrl ) tl_fec_on = 1;
        else tl_fec_on = 0;

        //2012-09-06: FEC only
        //Corey - Error Correction agent, (packet retransmission is only for Video), 2012-08-01
        //we support two mechanism of error correction (RFC4585,4588 & FEC).
        //Note that we will turn off ec agent only when both the mechanisms are off.
        if(( tl_fec_on )){
            
            new_session->ea = ec_agent_create(tl_fec_on);
            tl_ec_on = 1;
        }else{
            new_session->ea = NULL;
            tl_ec_on = 0;
        }

        // ec setting
        new_session->ec_on         = tl_ec_on;
        ec_arg.ctrl                = tl_ec_on;
        ec_arg.ea_id               = tl_ec_on  ? ((unsigned long)new_session->ea):0;
        ec_arg.ecMod_id            = tl_ec_on  ? ea_get_ecMod_id( new_session->ea ):0;
        ec_arg.fec_on              = tl_ec_on  ? tl_fec_on:0;
        ec_arg.send_payloadType    = tl_fec_on ? fec.send_payloadType:-1;
        ec_arg.receive_payloadType = tl_fec_on ? fec.receive_payloadType:-1;
 
		
        new_session->rtp_session = rtp_init(addr, rx_port, tx_port, RTP_TTL,
                                            rtcp_bw, tfrc_on, ec_arg ,resend_enable,
                                            tos, rtp_recv_callback,
                                            (void *)(new_session->participants),
                                            local_addr);

        rtp_set_my_ssrc(new_session->rtp_session, ssrc);

        new_session->tfrc_on = tfrc_on;

        /*set default maximum_payload_size to 1380, add by willy, 2010-08-19*/
        new_session->maximum_payload_size = 1380; // set default to 1380

        /* set default value to 0, if this value is set to 1, then all the rtp packet send in the TransportLayer_TransThread will not be sent */
        /*add by willy , 2010-09-21*/
        new_session->send_rtp_flag = 1;

        /* set default tos to 0 */ /* add by willy, 2010-11-10 */
        new_session->tos = 0;

        /* set default ipg to 1000 */
        new_session->video_ipi = 4000;
        new_session->audio_ipi = 1000;

        if (new_session->rtp_session != NULL)
        {
            rtp_set_tl_session(new_session->rtp_session,new_session);
            pdb_add(new_session->participants, rtp_my_ssrc(new_session->rtp_session));
            new_session->has_opposite_ssrc = FALSE;
            new_session->opposite_ssrc = 0; /* if has_opposite_ssrc == FALSE, it means nothing*/

            /* add by willy, 2010-10-11 */
            new_session->change_opposite_ssrc = 0;

            rtp_set_option(new_session->rtp_session, RTP_OPT_WEAK_VALIDATION, TRUE);
            rtp_set_sdes(new_session->rtp_session,
                         rtp_my_ssrc(new_session->rtp_session),
                         RTCP_SDES_TOOL, "QSTREAM", strlen("QSTREAM"));

            new_session->sending_buffer =  SendingBuf_Init();

            //m_tl_session[m_tl_session_num] = new_session;
            //m_tl_session[ring_table_index] = new_session; /* ring table, frank (c.y.) modify */
            //printf("<dbg> insert session, m_tl_session[%d] = %p->%p\n", m_tl_session_num, new_session, new_session->rtp_session); // frank debug

            return new_session;
        }
    }

    return NULL;
}

/****************************************************************************
 * Funciton Name: TransportLayer_Init
 * Parameters:
 * Author:        Ian Tuan
 * Description: initial all the things in transport layer session
 *              including participants database
 * Histroy:
 *              1. frank(c.y.), modify to support m_tl_session[] ring table
 *              2. frank(c.y.), alining code
 *
 *****************************************************************************/
struct TL_Session_t *TransportLayer_Init(const char *addr, 
                                         uint16_t rx_port, 
                                         uint16_t tx_port,
                                         int tfrc_on, 
										 TL_FECControl_t fec,
                                         int tos, 
                                         PayloadType_t *payload_type, 
                                         tl_rtcp_callback callback,
                                         const char *local_addr)
{
    double rtcp_bw = 5 * 1024 * 1024;
    struct TL_Session_t *new_session;
    PayloadType_t *my_pl_type;
    int resend_enable = 0, ring_table_index;
	RTP_ECControl_t ec_arg;
    int tl_fec_on = 0, tl_ec_on=0;

	
	TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "FECINFO: ctrl:%d, SPType:%d, RPType:%d\n", fec.ctrl, fec.send_payloadType, fec.receive_payloadType);

    /* ring table, frank(c.y.) modify */
    /*ring_table_index = TransportLayer_GetEmptyMTLSessionIndex();
    if (ring_table_index < 0 || ring_table_index >= MAX_TL_SESSION_NUM)
    {
        printf("<warning> can't get empty entry in the m_tl_session[], %s(%d)\n", __FUNCTION__, __LINE__);
        return NULL;
    }*/

    new_session = (struct TL_Session_t*)malloc(sizeof( struct TL_Session_t));
    if (NULL == new_session)
    {
        return NULL;
    }


    new_session->participants = pdb_init(payload_type->pt_num);

    if (new_session->participants != NULL)
    {
        /*initial the av profile part*/
        my_pl_type = (PayloadType_t*)malloc(sizeof(PayloadType_t));
        if (NULL == my_pl_type)
        {
            return NULL;
        }
        memcpy(my_pl_type, payload_type, sizeof(PayloadType_t));

        new_session->payload_type = my_pl_type;

        TransportLayer_Register_Method(new_session);

        if (new_session->payload_type->type == PAYLOAD_VIDEO)
            resend_enable = 1;//tfrc_on

        new_session->dtmf_key = 0;
        new_session->get_dtmf_key = 0;
        new_session->dtmf_sent_cnt = 0;

        if (callback)
            new_session->rtcp_callback = callback;
        else
            new_session->rtcp_callback = NULL;


        if( LINUX_FEC && fec.ctrl) tl_fec_on = 1;
        else tl_fec_on = 0;

         //2012-09-06: FEC only
        //Corey - Error Correction agent, (packet retransmission is only for Video), 2012-08-01
        //we support two mechanism of error correction (RFC4585,4588 & FEC).
        //Note that we will turn off ec agent only when both the mechanisms are off.
        if(( tl_fec_on ) ){
            
            new_session->ea = ec_agent_create(tl_fec_on);
            tl_ec_on = 1;
        }else{
            new_session->ea = NULL;
            tl_ec_on = 0;
        }

        // ec setting
        new_session->ec_on         = tl_ec_on;
        ec_arg.ctrl                = tl_ec_on;
        ec_arg.ea_id               = tl_ec_on  ? ((unsigned long)new_session->ea):0;
        ec_arg.ecMod_id            = tl_ec_on  ? ea_get_ecMod_id( new_session->ea ):0;
        ec_arg.fec_on              = tl_ec_on  ? tl_fec_on:0;
        ec_arg.send_payloadType    = tl_fec_on ? fec.send_payloadType:-1;
        ec_arg.receive_payloadType = tl_fec_on ? fec.receive_payloadType:-1;


        new_session->rtp_session = rtp_init(addr, rx_port, tx_port, RTP_TTL,
                                            rtcp_bw, tfrc_on, ec_arg, resend_enable,
                                            tos, rtp_recv_callback,
                                            (void *)(new_session->participants),
                                            local_addr);

        new_session->tfrc_on = tfrc_on;

        /*set default maximum_payload_size to 1380, add by willy, 2010-08-19*/
        new_session->maximum_payload_size = 1380; // set default to 1380

        /* set default value to 0, if this value is set to 1, then all the rtp packet send in the TransportLayer_TransThread will not be sent */
        /*add by willy , 2010-09-21*/
        new_session->send_rtp_flag = 1;

        /* set default tos to 0 */ /* add by willy, 2010-11-10 */
        new_session->tos = 0;

        /* set default ipg to 1000 */
        new_session->video_ipi = 4000;
        new_session->audio_ipi = 1000;

        if (new_session->rtp_session != NULL)
        {
            rtp_set_tl_session(new_session->rtp_session,new_session);
            pdb_add(new_session->participants, rtp_my_ssrc(new_session->rtp_session));
            new_session->has_opposite_ssrc = FALSE;
            new_session->opposite_ssrc = 0; /* if has_opposite_ssrc == FALSE, it means nothing*/

            /* add by willy, 2010-10-11 */
            new_session->change_opposite_ssrc = 0;

            rtp_set_option(new_session->rtp_session, RTP_OPT_WEAK_VALIDATION, TRUE);
            rtp_set_sdes(new_session->rtp_session,
                         rtp_my_ssrc(new_session->rtp_session),
                         RTCP_SDES_TOOL, "QSTREAM", strlen("QSTREAM"));

            new_session->sending_buffer =  SendingBuf_Init();

            //m_tl_session[m_tl_session_num] = new_session;
            //m_tl_session[ring_table_index] = new_session; /* ring table, frank (c.y.) modify */
            //printf("<dbg> insert session, m_tl_session[%d] = %p->%p\n", m_tl_session_num, new_session, new_session->rtp_session); // frank debug

            return new_session;
        }
    }
    return NULL;
}

/****************************************************************************
 * Funciton Name: TransportLayer_Start
 * Parameters:
 * Author:        Ian Tuan
 * Description:
 *****************************************************************************/
int TransportLayer_Start(struct TL_Session_t *session)
{
    //int res;

    session->on_stopping = 0;
    //if (sem_init(&(session->trans_terminate_sem),0,0) != 0)
    //{
    //    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "Creating the network device semaphore error\n");
    //    return THREADS_CREATE_ERR;
    //}

    //if (sem_init(&(session->recv_terminate_sem),0,0) != 0)
    //{
    //    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "Creating the network device mutex error\n");
    //    return THREADS_CREATE_ERR;
    //}

    //if ( pthread_mutex_init(&(session-> m_network_device_lock), NULL) != 0 )
    //{
    //    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "Creating the network device mutex error\n");
    //    return THREADS_CREATE_ERR;
    //}

    //gettimeofday(&(session->start_time), NULL); /* initiate start_time, Frank.T */
    if ( pthread_create(&(session->m_trans_thread_id), NULL, TransportLayer_TransThread, session) != 0)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "creating transmission thread error\n");
        return THREADS_CREATE_ERR;
    }
    if (pthread_create(&(session->m_recv_thread_id), NULL, TransportLayer_RecvThread, session) !=0)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "creating receiving thread error\n");
        return THREADS_CREATE_ERR;

    }

    return (0);
}


/*
    this function is for upper layer to set the ntp time corresponding rtp timestamp
    because the timestamp is provided by upper layer, they know the time corresponding time info
*/
int TransportLayer_SetRTCPTimeIfno(struct TL_Session_t *session, uint32_t rtp_ts,
                                   uint32_t ntp_sec, uint32_t ntp_frac)
{
    ntp_rtpts_pair info;

    info.ntp_sec = ntp_sec;
    info.ntp_frac = ntp_frac;
    info.rtp_ts = rtp_ts;

    rtp_set_rtcp_time_info(session->rtp_session, &info);

    return 1;
}


int TransportLayer_SetTFRCMinSendingRate(struct TL_Session_t *session, uint32_t min_sending_rate)
{
    rtp_set_tfrc_min_sending_rate(session->rtp_session, min_sending_rate);

    return 1;
}


/****************************************************************************
 * Funciton Name: TransportLayer_RTCPCallback
 * Parameters:
 * Author:        Ian Tuan
 * Description:   handle the rtcp call back message from rtp.c
 *                and
 *****************************************************************************/
int TransportLayer_RTCPCallback(struct rtp *session,  rtp_event *e)
{
    struct TL_Session_t *tl_session;
    struct TL_Rtcp_Event_t *event;
    int ret = -1;

    tl_session = TransportLayer_GetTLSession(session);

    if (!tl_session)
        return ret;

    if (!(tl_session->rtcp_callback))
        return ret;
    if(tl_session->on_stopping)
        return ret;
        
    event = (struct TL_Rtcp_Event_t*)malloc(sizeof(struct TL_Rtcp_Event_t));
    if (NULL == event)
    {
        return -1;
    }
    event->tl_session = tl_session;
    event->ssrc = e->ssrc;

    ret = 0;
    switch (e->type)
    {
    case RX_TFRC_RX: /* compute TCP friendly data rate */
        event->type = TL_RX_TFRC_RX;
        event->data = e->data;
        if (tl_session->rtcp_callback)
        {
            ret = tl_session->rtcp_callback(tl_session, event);
        }
        break;

    case RX_RTCP_START:
        break;

    case RX_RTCP_FINISH:
        break;

    case RX_SR:
        event->type = TL_RX_SR;
        event->data = e->data;
        if (tl_session->rtcp_callback)
        {
            ret = tl_session->rtcp_callback(tl_session, event);
        }
        break;

    case RX_RR:
        event->type = TL_RX_RR;
        event->data = e->data;
        if (tl_session->rtcp_callback)
        {
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        break;

    case RX_RR_EMPTY:
        break;

    case RX_SDES:
        break;

    case RX_APP:
        event->type = TL_RX_APP;
#ifdef WIN32 //gcc sizeof(void) == 1 == sizeof(char)
        event->data = (char*)e->data + 4*3; /*shift the ssrc and name*/
#else
        event->data = e->data + 4*3; /*shift the ssrc and name*/
#endif
        if (tl_session->rtcp_callback)
        {
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        break;

    case RX_BYE:
        break;

    case SOURCE_DELETED:
        break;

    case SOURCE_CREATED:
        if (!(tl_session->has_opposite_ssrc))
        {
            tl_session->has_opposite_ssrc = TRUE;
            tl_session->opposite_ssrc = e->ssrc;
            set_rtp_opposite_ssrc(tl_session->rtp_session, tl_session->opposite_ssrc);
        }
        break;

    case RR_TIMEOUT:
        break;

    case APP_SET_TIME_INFO:  /*for lip sync set time information*/
        event->type = TL_APP_SET_TIME_INFO;
        event->data = e->data;
        if (tl_session->rtcp_callback)
        {
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        break;

    case CHECK_SSRC: /* if there are CHANGE_OPPOSITE_SSRC_THRESHOLD continues rtp which ssrc is not equal than current opposite_ssrc, change it */ /* add by willy, 2010-10-11 */
        if (e->ssrc != tl_session->opposite_ssrc)
        {
            tl_session->change_opposite_ssrc ++;
            if (tl_session->change_opposite_ssrc >= CHANGE_OPPOSITE_SSRC_THRESHOLD)
            {
                tl_session->change_opposite_ssrc = 0;
                tl_session->opposite_ssrc = e->ssrc;
                set_rtp_opposite_ssrc(tl_session->rtp_session, tl_session->opposite_ssrc);
            }
        }
        else
        {
            tl_session->change_opposite_ssrc = 0;
        }

        if (tl_session->rtcp_callback)
        {
            event->type = TL_RX_RTP;
            event->data = e->data;
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        break;

    case RX_RTP: /* rtp event handle, frank */
        if (tl_session->rtcp_callback)
        {
            event->type = TL_RX_RTP_EVENT;
            event->data = e->data;
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        break;

    case CHECK_RESEND_PKT: /* check it's resend packet or not */
        if (tl_session->rtcp_callback)
        {
            event->type = TL_CHECK_RESEND_PKT;
            event->data = e->data;
            ret = tl_session->rtcp_callback(tl_session,event);
        }
        //printf("%s(), ret: %d\n", __FUNCTION__, ret);
        break;

    case RX_RTCP_PSFB:
        if (tl_session->rtcp_callback)
        {
            rtcp_common *data = (rtcp_common *)e->data;
            TL_PSFB_Para_t psfb_event;
            
            psfb_event.fmt = data->count;
            event->type = TL_RX_RTCP_PSFB;
            event->data = (void *)&psfb_event;
            ret = tl_session->rtcp_callback(tl_session, event);
        }
        break;
    default:
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "Unknown RTP event (type=%d)\n", e->type);
        break;
    }
    free(event);

    return ret;
}

int TransportLayer_GetSendingBufStatus(struct TL_Session_t *session, uint32_t *buf_frame_count, uint32_t *buf_drop_frame_count)
{
    int result = 0;

    result = SendingBuf_GetStatus(session->sending_buffer, buf_frame_count, buf_drop_frame_count);

    return result;
}

int TransportLayer_TerminateSession(struct TL_Session_t *session)
{
    int ret;

    //if (LINUX_DEBUGMSG) printf("%s(), enter\n", __FUNCTION__);

    TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1, "post session terminate signal\n");

    /*post trans thread terminate signal*/
    //if (LINUX_DEBUGMSG) printf("%s(), trans thread\n", __FUNCTION__);
    //sem_post(&(session->trans_terminate_sem));
	session->on_stopping = 1;
    ret = pthread_join(session->m_trans_thread_id, NULL);
    if (ret)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1,"Terminate TL Trans thread error code=%d\n",ret);
    }

    /*post recv thread terminate signal*/
    //if (LINUX_DEBUGMSG) printf("%s(), recv thread\n", __FUNCTION__);
    //sem_post(&(session->recv_terminate_sem));
    ret = pthread_join(session->m_recv_thread_id, NULL);
    if (ret)
    {
        TL_DEBUG_MSG(TL_TRANSPORT, DBG_MASK1,"Terminate TL Recv thread error code=%d\n",ret);
    }

    //if (LINUX_DEBUGMSG) printf("%s(), destroy process\n", __FUNCTION__);
    TransportLayer_DestroyProcess(session);

    /* free m_tl_session[] table, frank (c.y) */
    //if (LINUX_DEBUGMSG) printf("%s(), free mtl session\n", __FUNCTION__);
    TransportLayer_FreeMTLSession(session);

   // if (LINUX_DEBUGMSG) printf("%s(), exit\n", __FUNCTION__);

    return TL_SUCCESS;
}

uint32_t TransportLayer_GetSessionSSRC(struct TL_Session_t *session)
{
    return rtp_my_ssrc(session->rtp_session);
}

int TransportLayer_Set_Video_InterPacketInterval(struct TL_Session_t *session, int ipi)
{
    if(session == NULL) return TL_FAIL;
    if(ipi <= 0) return TL_FAIL;

    session->video_ipi = ipi;
    return TL_SUCCESS;
}


int TransportLayer_DisableRTCP(struct TL_Session_t *session)
{
    if(session == NULL) return TL_FAIL;

	disable_rtcp(session->rtp_session);

	return TL_SUCCESS;
}


int TransportLayer_EnableRTCP(struct TL_Session_t *session)
{
    if(session == NULL) return TL_FAIL;

	enable_rtcp(session->rtp_session);

	return TL_SUCCESS;
}

int TransportLayer_GetSessionInfo(struct rtp *session, int info_type, int* value)
{
    struct TL_Session_t *tl_session;

    if(session == NULL) return TL_FAIL;

    tl_session = TransportLayer_GetTLSession(session);

    if (!tl_session)
        return TL_FAIL;


    switch(info_type)
    {
        case MEDIA_TYPE:
            *value = tl_session->payload_type->type;
            break;
        default:            
            break;
    }

    return TL_SUCCESS;
}

int TransportLayer_GetSeqNum(struct TL_Session_t *session, uint16_t* seq_num)  
{
    pdb_e         *cp = NULL;
    pbuf_node     *curr_frame = NULL;
    rtp_packet    *packet = NULL;

    //cp = pdb_iter_init(session->participants);
    if (session->has_opposite_ssrc)
        cp = pdb_get(session->participants, session->opposite_ssrc);
    else
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));

    if (!cp)
        return TL_FAIL;

    RECVING_BUF_LOCK(cp->playout_buffer);

    if (!(cp->playout_buffer->frst))
    {
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return TL_FAIL;
    }

    curr_frame = cp->playout_buffer->frst;    
    if (curr_frame)
    {
        packet = (rtp_packet*)curr_frame->cdata->data;
    }
    else
    {
        RECVING_BUF_UNLOCK(cp->playout_buffer);        
        return TL_FAIL;
    }
    
    if (packet)
    {
        *seq_num = packet->fields.seq;
    }
    else
    {
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return TL_FAIL;
    }
    
    RECVING_BUF_UNLOCK(cp->playout_buffer);

    return TL_SUCCESS;
}


