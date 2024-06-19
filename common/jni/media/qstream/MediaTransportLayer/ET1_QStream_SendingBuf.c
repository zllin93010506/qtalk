/*
 * FILE:     ET1_QStream_SendingBuf.c
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */
#ifdef WIN32
#include "config_w32.h"
#endif 
#include "ET1_QStream_SendingBuf.h"
#include "debug.h"

#define SEND_REDUNDANT_PACKET_ENABLE 0

#define MAX_SENDING_BUF_FRAME_SIZE 50 /*this maxima sending buffer size defie is only for H264 video frame */

/*this holding time means if a frame over this time we won't sent it out but won't kill it right now*/
#define MAX_FRAME_HOLDING_TIME    45000 /*this is 500ms and unit is 1/90000 sec*/
#define MAX_FRAME_RESERVED_TIME   180000

#define I_FRAME_WITH_PARAMETER   24

#define DIFF_TS(X, Y) (Y - X) /*older frame time stamp put in X, last frame time stamp put in Y*/

int GetSendingBufSize(SendingBuf_t *send_buf)
{
     return send_buf->size;
}

 /****************************************************************************
 * Funciton Name: SendingBuf_GetNextToSend                                     
 * Parameters:    
 *                 send_buf:  the sending buffer pointer
 *                 timestamp: indicate the rtp timestamp of this packet 
 *                 end_flag:  indicate if the curr packet is the end of a frame 
 *
 * Author:        Ian Tuan
 *
 * Description:   This funciton will give the caller the next packet need to send
 *                and also with rtp timestamp and a frame end flag
 *****************************************************************************/
 PayloadData_t *SendingBuf_GetNextToSend(SendingBuf_t *send_buf, uint32_t *timestamp, int *end_flag)
 {
     SendingBuf_Node_t *node;
     PayloadData_t *p;
     //int count = 0;
     //int i = 0;
     //static int j = 0;
     
     *end_flag = 0;
     
     SENDING_BUF_LOCK(send_buf);
     
     if(send_buf->resend_list)
     {
         p = send_buf->resend_list;
         
         send_buf->resend_list = send_buf->resend_list->resend_next;
         
         *timestamp = p->timestamp;

         send_buf->last_get_payload = p; /*record this payload in last_get_payload*/

         SENDING_BUF_UNLOCK(send_buf);
         TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"sending buffer resend p = %p seq = %d resend = %d\n", p, p->seq, p->resend);
         return (p);
     }
     
     /*check if there is a first out frame*/
     if(send_buf->first_out)
     {

         node = send_buf->first_out;

//        for(p = node->payload_data; p != NULL; p = p->next)
//             i++;
//        TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "%d payload in buffer\n",i);

         for(p = node->payload_data; p != NULL; p = p->next)
         {
              if(!(p->sent))
                  break;
         }
         
         *timestamp = node->rtp_ts;
         
         p->sent = 1;
         
         TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "node->rtp_ts=%d p->next=%p\n",node->rtp_ts,p->next);
         
         node->payload_sent ++;
         
         if((p->next == NULL)) 
         {
               //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "before move first_out, first = %x first_out = %x \n", send_buf->first, send_buf->first_out);
               //send_buf->first_out = send_buf->first_out->next;
               //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "after move first_out, first = %x first_out = %x \n", send_buf->first, send_buf->first_out);
               *end_flag = 1;      
               
               send_buf->first_out = send_buf->first_out->next;
               send_buf->non_sent_size --;

         }
         
         send_buf->last_get_payload = p;

         SENDING_BUF_UNLOCK(send_buf);
        
         return (p); 
     }
 
#if SEND_REDUNDANT_PACKET_ENABLE
        /* this is for TFRC has data to try the network bandwidth when we don't have the latest data needed to send.*/ 
        if(send_buf->last)
        {
             node = send_buf->last;

             if(send_buf->last_redundant_ts == send_buf->last->rtp_ts)
            {
                 
                 for(p = node->payload_data; p != NULL; p = p->next)
                {
                        if(p->redundant_resend == 0)
                            break;
                        if(p->next == NULL)
                            break;
                 }
                 
                 //if(!p)
                 //    p = node->payload_data;
            }
            else
           {
                 p = node->payload_data;
                 send_buf->last_redundant_ts = node->rtp_ts;
           }

            p->redundant_resend = 1;
            
            SENDING_BUF_UNLOCK(send_buf);
        
            return (p); 
         }     

#endif

     SENDING_BUF_UNLOCK(send_buf);
     
     return NULL;
     
 }

 void SendingBuf_MoveToNextFrame(SendingBuf_t *send_buf)
{
    SENDING_BUF_LOCK(send_buf);

    if(send_buf->first_out)
        send_buf->first_out = send_buf->first_out->next;

    SENDING_BUF_UNLOCK(send_buf);

}

 void SendingBuf_FreeBufNode(SendingBuf_Node_t *buf_node)
 {
     PayloadData_t *temp;
     
     while(buf_node->payload_data)
     {
          temp = buf_node->payload_data->next;
          
          /*if has exten header data, then release it*/
          if(buf_node->payload_data->ext_hdr_data)
              free(buf_node->payload_data->ext_hdr_data);

          free(buf_node->payload_data->data);
          free(buf_node->payload_data);
          
          buf_node->payload_data = temp;       
     }
     
     free(buf_node);
 }


int SendingBuf_GetStatus(SendingBuf_t *send_buf, uint32_t *frame_count, uint32_t *drop_frame_count)
{
     //SendingBuf_Node_t *temp;
     //uint32_t has_sent_frames = 0;

    SENDING_BUF_LOCK(send_buf);

    *frame_count = send_buf->non_sent_size;

    *drop_frame_count = send_buf->drop_frame_count;
     /*reset drop frame count*/
     send_buf->drop_frame_count = 0;

    SENDING_BUF_UNLOCK(send_buf);
    
    return 1;
}

static inline uint8_t GetNALType(const uint8_t *h){
	return (*h) & ((1<<5)-1);
}

 /****************************************************************************
 * Funciton Name: SendingBuf_RemoveOutofDate                                   
 * Parameters: 
 * Author:        Ian Tuan
 * Description: this function prevent the video frame data stay in buffer out of date
 *                      return value ret = 1---> 
 *                                           ret = 2
 *****************************************************************************/
int SendingBuf_RemoveOutofDate(SendingBuf_t *send_buf, int isEndOfFrame)
{
      SendingBuf_Node_t *temp;
      PayloadData_t *payload, *tp;
      uint32_t diff;
      uint8_t type;
      int first_out_removed = FALSE;
      int ret = 0;

      SENDING_BUF_LOCK(send_buf);
     
       ret = 1;

       while(send_buf->first)
      {
           diff = DIFF_TS(send_buf->first->rtp_ts, send_buf->last->rtp_ts);
           //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "1 diff = %u last->rtp_ts = %u, first->rtp_ts=%u\n",diff, send_buf->last->rtp_ts, send_buf->first->rtp_ts);

           if(diff > MAX_FRAME_RESERVED_TIME)
          {
              if(send_buf->first->rtp_ts == send_buf->last_get_payload->timestamp)
              {
                  ret = 2;
              }

              if(send_buf->resend_list)
              {
                  for(tp = send_buf->resend_list; tp != NULL; tp = tp->resend_next)
                 {
                      if(send_buf->first->rtp_ts == tp->timestamp)
                      { 
                          send_buf->resend_list = send_buf->resend_list->resend_next;
                      }
                      else
                          break;
                  }
               }

               if(send_buf->first == send_buf->first_out)
              {
                   TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"@@@@sending buffer: strange condition first_out->rtp_ts = %u, last->rtp_ts= %u, buf size=%u, non_sent_size=%u\n",
                                            send_buf->first_out->rtp_ts, send_buf->last->rtp_ts,send_buf->size, send_buf->non_sent_size);
                   send_buf->first_out = send_buf->first_out->next;
                   first_out_removed = TRUE;
                   
              }

               temp = send_buf->first->next;
               //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "free node\n");
               SendingBuf_FreeBufNode(send_buf->first);
               send_buf->first = temp;
               send_buf->size --;

               if(first_out_removed)
              {
                   /*we have droped the non sent size , we should count the non_sent_size*/
                   send_buf->non_sent_size --;
                   send_buf->drop_frame_count++;
              }

          }/*end if (diff >...*/
          else
              break;
      }/*end while*/

       while(send_buf->first_out && isEndOfFrame == 1)
       {

            diff = DIFF_TS(send_buf->first_out->rtp_ts, send_buf->last->rtp_ts);
            //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "2 diff = %u last->rtp_ts = %u, first_out->rtp_ts=%u\n",diff, send_buf->last->rtp_ts, send_buf->first_out->rtp_ts);
            /*check if there are frames are too old, if they are then kill them*/
            if(diff > MAX_FRAME_HOLDING_TIME)
            {
                //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "sending buffer: 1 drop frame not sent complete first_out->rtp_ts = %u last->rtp_ts= %u\n", send_buf->first_out->rtp_ts, send_buf->last->rtp_ts);
				printf("sending buffer: drop frame {out of date}, first_out->rtp_ts = %u last->rtp_ts= %u\n", send_buf->first_out->rtp_ts, send_buf->last->rtp_ts);
				
                 first_out_removed = TRUE;
                 ret = 2;
                    
                  send_buf->first_out = send_buf->first_out->next;
                  send_buf->non_sent_size --;
                  send_buf->drop_frame_count ++;

            }
//            else if (first_out_removed) 
//            {
//                 payload = send_buf->first_out->payload_data;
//                 type = GetNALType(payload->data);

//                 if(type == I_FRAME_WITH_PARAMETER)
//                 { 
//                     diff = DIFF_TS(send_buf->first_out->rtp_ts, send_buf->last->rtp_ts);
//                     //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "3 diff = %u last->rtp_ts = %u, first_out->rtp_ts=%u\n",diff, send_buf->last->rtp_ts, send_buf->first_out->rtp_ts);
//                     if(diff > MAX_FRAME_HOLDING_TIME)
//                         continue;
//                     else
//                         break; /*we have killed all we need to kill, this frame is still young and is a Gop's begineing*/
//                  }

//                   /* 
//                        but if their boss (i frame) or brohters (other p frames) is killed, 
//                        then they should also go to die
//                         no matter if they are i or p
//                   */
//                    TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "sending buffer: 2 drop frame not sent complete\n");
//         			  printf("sending buffer: 2 drop frame not sent complete\n");
//                    send_buf->first_out = send_buf->first_out->next;
//                    send_buf->non_sent_size --;     
//                    send_buf->drop_frame_count ++;         
//             }
             else
             {
                  /*we have killed all too old data and has been sent
                    the other frames are still young and have not sent yet
                  */
                  break;
              }
        
       }/*end while*/
       
       if(first_out_removed)
            TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "sending buffer removed frame that never be sent send_buf->size=%d non_sent_size = %d\n", send_buf->size, send_buf->non_sent_size);

      if(!(send_buf->first))
     {
          send_buf->last = NULL;
          send_buf->first_out = NULL;
          if(send_buf->size)
          {
                  TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "sending buffer size count error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                 send_buf->size = 0;
           }
     } 

      SENDING_BUF_UNLOCK(send_buf);

      return ret;
}

 int SendingBuf_RemoveSent(SendingBuf_t *send_buf)
 {
      SendingBuf_Node_t *temp;
      
      SENDING_BUF_LOCK(send_buf);
      
      while(send_buf->first)
      {
          if(send_buf->first == send_buf->first_out)
               break;
          
          temp = send_buf->first->next;
          //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "free node\n");
          SendingBuf_FreeBufNode(send_buf->first);
          
          send_buf->first = temp;

          send_buf->size --;

          //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "buf size = %d\n", send_buf->size);
      }  

      if(!(send_buf->first))
     {
          send_buf->last = NULL;
          send_buf->first_out = NULL;
          if(send_buf->size)
          {
                 TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1, "sending buffer size count error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                 send_buf->size = 0;
           }
     } 

      SENDING_BUF_UNLOCK(send_buf);

	  return 0;
 }


/*remove all the frame of this sending buffer*/
int SendingBuf_FlushBuffer(SendingBuf_t *send_buf)
{
     SendingBuf_Node_t *temp;

     
      
      while(send_buf->first)
      {
          
          temp = send_buf->first->next;
          //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "free node\n");
          SendingBuf_FreeBufNode(send_buf->first);
          
          send_buf->first = temp;

          send_buf->size --;

          //TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK2, "buf size = %d\n", send_buf->size);
      }  
    
     
  
     return 1;
}

int SendingBuf_Destroy(SendingBuf_t *send_buf)
{
    SENDING_BUF_LOCK(send_buf);

    SendingBuf_FlushBuffer(send_buf);

    SENDING_BUF_UNLOCK(send_buf);

    SENDING_BUF_LOCK_DESTROY(send_buf);

    free(send_buf);

    return 1;
}

  /****************************************************************************
 * Funciton Name: SendingBuf_Init                                     
 * Parameters: 
 * Author:        Ian Tuan
 * Description:
 *****************************************************************************/
 SendingBuf_t* SendingBuf_Init(void)
 {
      SendingBuf_t    *send_buf = NULL;
	
     send_buf = (SendingBuf_t*)malloc(sizeof(SendingBuf_t));
     
     if (send_buf != NULL) 
     {
          send_buf->first = NULL;
          send_buf->first_out = NULL;
          send_buf->last = NULL;
          send_buf->size = 0;
          send_buf->non_sent_size = 0;
          send_buf->drop_frame_count = 0;
          send_buf->resend_list = NULL;
          send_buf->last_get_payload = NULL;
          send_buf->last_redundant_ts = 0;
          /*init mutex lock*/
          pthread_mutex_init(&(send_buf->lock), NULL);
          
         return send_buf;
     }
     return NULL;
 }
 
 /****************************************************************************
 * Funciton Name: SendingBuf_Insert                                     
 * Parameters: 
 *                send_buf: the frame buffer start pointer
 *                payload:   the frame buffer end pointer
 *
 * Author:        Ian Tuan
 * Description: the call function should add the mutext lock  
 *              outside this insert funciton
 *
 *****************************************************************************/
 int SendingBuf_Insert(SendingBuf_t * send_buf, uint8_t *payload, 
                      uint16_t payload_len, uint32_t timestamp, int marker,
                      uint8_t *ext_hdr_data, uint16_t ext_len, int rtp_event_flag)
 {   
     SendingBuf_Node_t *new_buf_node;
     PayloadData_t *payload_data;
     PayloadData_t *p;
     uint8_t *data;
     uint8_t *info;        

     data = (uint8_t*)malloc(payload_len);
	 if (NULL == data)
	 {
		return -1;
	 }
     memcpy(data,payload,payload_len);

     /*initialize a new payload data structure*/
     payload_data = (PayloadData_t*)malloc(sizeof(PayloadData_t));
	 if (NULL == payload_data)
	 {
		free(data);
		return -1;
	 }

     payload_data->timestamp = timestamp;
     payload_data->prev = NULL;
     payload_data->next = NULL;
     payload_data->resend_next = NULL;
     payload_data->data = data;
     payload_data->data_len = payload_len;
     payload_data->marker = marker;
     payload_data->sent = 0;
     payload_data->resend = 0;
     payload_data->redundant_resend = 0;
     payload_data->rtp_event_flag = rtp_event_flag;

     if(ext_hdr_data)
     {
         info = (uint8_t*)malloc(ext_len);
		 if (NULL == info)
		 {
			free(data);
			free(payload_data);
			return -1;
		 }
         memcpy(info, ext_hdr_data, ext_len);
         payload_data->ext_hdr_data = info;
         payload_data->ext_len = ext_len;    
     }
     else
     {
         payload_data->ext_hdr_data = NULL;
         payload_data->ext_len = 0;
     }
     
     if(send_buf->last)
    {
         /*check if this payload is the first of a frame data*/
         if(send_buf->last->rtp_ts != timestamp)
         {
             new_buf_node = (SendingBuf_Node_t*)malloc(sizeof(SendingBuf_Node_t));
			 if (NULL == new_buf_node)
			 {
				free(data);
				free(payload_data);
				return -1;
			 }
             new_buf_node->prev = new_buf_node->next = NULL;
             new_buf_node->payload_data = payload_data;
             new_buf_node->rtp_ts = timestamp;
             new_buf_node->payload_sent = 0;
         
             /*if the first out is null then this frame is the first out*/
             if(!send_buf->first_out)
            {
                 send_buf->first_out = new_buf_node;
            }

             /*add buf node into the buffer tail*/
             send_buf->last->next = new_buf_node;    
             new_buf_node->prev = send_buf->last;

             /* this defnitely is the the last frame in buffer*/
             send_buf->last = new_buf_node;
             send_buf->size ++;
             send_buf->non_sent_size ++;
         }
         else
        {
             /*search the buffer node tail*/
             for(p=send_buf->last->payload_data; p->next != NULL; p = p->next)
             ;
         
             p->next = payload_data;   
         
             payload_data->prev = p;
         }
     }/*end if(send_buf->last)*/
     else
    {            
        new_buf_node = (SendingBuf_Node_t*)malloc(sizeof(SendingBuf_Node_t));
		if (NULL == new_buf_node)
		{
			free(data);
			free(payload_data);
			return -1;
		}
        new_buf_node->prev = new_buf_node->next = NULL;
        new_buf_node->payload_data = payload_data;
        new_buf_node->rtp_ts = timestamp;
        new_buf_node->payload_sent = 0;
        /*
         first is null means the buffer is null
         so this node is the only node
         */
        send_buf->first = new_buf_node;
        send_buf->first_out= new_buf_node;
        send_buf->last = new_buf_node; /* this defnitely is the the last frame in buffer*/
        send_buf->size ++;
        send_buf->non_sent_size ++;
     }
     
     return 1;
 }
 
  /****************************************************************************
 * Funciton Name: SendingBuf_SearchSentPayload                                     
 * Parameters: 
 *                send_buf: the frame buffer start pointer
 *                seq : resend packet's rtp sequence number
 *
 * Author:        Ian Tuan
 * Description: This function helps to search the payload data 
 *               has been sent when given a sequence number
 *
 *****************************************************************************/
 PayloadData_t *SendingBuf_SearchSentPayload(SendingBuf_t *send_buf, uint16_t seq)
 {
      PayloadData_t *payload;
      SendingBuf_Node_t *frame;
      
      if(!(send_buf->first))
      {
          return NULL;   
      }
      
      frame = send_buf->first;
      TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"search resend payload first frame start seq = %u last frame start_seq = %u\n", frame->payload_data->seq, send_buf->last->payload_data->seq);
          
      //while(frame != send_buf->first_out)
	  while(frame != NULL)
      {
          //if(frame->next && (frame->next != send_buf->first_out))
		  if(frame->next  )
          {
              if(frame->next->payload_data->seq > frame->payload_data->seq)
              {
                  if((seq >= frame->payload_data->seq) && (seq < frame->next->payload_data->seq))
                  {
                  
                      for(payload = frame->payload_data; payload != NULL; payload = payload->next)
                      {
                          if(payload->seq == seq)
                         {
                              TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"1 return resend payload = %p\n", payload);
                              return payload;
                          }
                      }
                      /*if we get here, this is a strange error. It means we get a frame with non continued seq no payload data*/
					  if(frame->next != NULL){
                        frame = frame->next;
                        continue;
                       }
					   
                      return NULL;
                  }
                  else
                  {
                      frame = frame->next;
                      continue;
                  }          
              }
              else /*if(frame->next->payload_data->seq > frame->payload_data->seq)*/
              {
                  /*wrap around condition*/
                  for(payload = frame->payload_data; payload != NULL; payload = payload->next)
                  {
                      if(payload->seq == seq)
                     {
                          TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"2 return resend payload = %p\n", payload);
                          return payload;
                      }
                  }
              }       

          }
          else /*if(frame->next && (frame->next != send_buf->first_out))*/
          {
              /*the last frame we can search*/
              for(payload = frame->payload_data; payload != NULL; payload = payload->next)
              {
				  //printf("search last frame,, payload->seq = %d, seq = %d!!!!!\n",payload->seq, seq);
                  if(payload->seq == seq)
                  {
                       TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"3 return resend payload = %p\n", payload);
                       return payload;
                  }
              }

              return NULL;
          }

          frame = frame->next;
     }/*end while*/

     /*also strange if we get here*/
     return NULL;
 }
 
 /****************************************************************************
 * Funciton Name: SendingBuf_Resend                                     
 * Parameters: 
 *                send_buf: the frame buffer start pointer
 *                seq : resend packet's rtp sequence number
 *
 * Author:        Ian Tuan
 * Description: this function supports RFC4585 module to mark the packets need  
 *              to be resend
 *
 *****************************************************************************/
 int SendingBuf_Resend(SendingBuf_t *send_buf, uint16_t seq)
 {
     PayloadData_t *payload, *temp;
     
     SENDING_BUF_LOCK(send_buf);
     
     payload = SendingBuf_SearchSentPayload(send_buf, seq);
     
     if(payload)
         TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"SendingBuf_Resend resend seq = %d payload = %p, seq = %d \n", seq, payload, payload->seq);
     
 
     if(!payload)
     {
         TL_DEBUG_MSG(TL_SENDING_BUF, DBG_MASK1,"resend search null\n");
         SENDING_BUF_UNLOCK(send_buf);
         return 0;
     }    
     
     payload->resend = 1; /*raise the resend flag*/
     
     /*check if there already has resend list*/
     if(send_buf->resend_list)
     {
         /*search the resend tail*/
         for(temp = send_buf->resend_list; temp->resend_next != NULL; temp = temp->resend_next)
         ;
         
         temp->resend_next = payload;   
     }
     else
     {
         send_buf->resend_list = payload;
     
     }
     
     SENDING_BUF_UNLOCK(send_buf);
     
     return 1;
     
 }
 
 
