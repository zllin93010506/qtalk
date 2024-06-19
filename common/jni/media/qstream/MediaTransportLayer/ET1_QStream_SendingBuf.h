/*
 * FILE:     ET1_QStream_SendingBuf.h
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */
 
#ifndef _SENDING_BUF_
#define _SENDING_BUF_

#include "config_unix.h"

#define SENDING_BUF_LOCK(x)		pthread_mutex_lock(&(x->lock))
#define SENDING_BUF_UNLOCK(x)		pthread_mutex_unlock(&(x->lock))
#define SENDING_BUF_LOCK_DESTROY(x)        pthread_mutex_destroy(&(x->lock))

/*packet or payload level*/
 typedef struct _PayloadData
 {
     struct _PayloadData         *prev;
     struct _PayloadData         *next;
     struct _PayloadData         *resend_next; /*for resend list use*/
     
     uint32_t                    timestamp;
     unsigned char               *data;
     int                         data_len;
     int                         marker; /*for rtp marker bit*/
     unsigned char               *ext_hdr_data;
     int                         ext_len;
     int                         sent;
     int                         resend; /*resend flag for RFC4585*/
     uint16_t                    seq; /*for resend use, this is the rtp sequence assigned by rtp.c*/
     int                         redundant_resend;
     int                         rtp_event_flag;
 }PayloadData_t;
 
 /*frame level*/
 typedef struct _SendingBuf_Node
 {
     struct _SendingBuf_Node   *prev;
     struct _SendingBuf_Node   *next;
     
     PayloadData_t             *payload_data;
     
     uint32_t                  rtp_ts;    
     
     int                       payload_sent; /* count the sent fragment payloads */ 

     uint32_t                  info;
     
     /*start_seq is the first payload and end_seq is last, all for RFC4585*/
     uint16_t                  start_seq; 
     
     uint16_t                  end_seq;

 }SendingBuf_Node_t;
 
 /*buffer level*/
 typedef struct _SendingBuf
 {
      SendingBuf_Node_t	  *first;		/* First node to remove		*/
      SendingBuf_Node_t	  *first_out;	        /* Next node to be transmitted */
      SendingBuf_Node_t	  *last;		/* Last node inserted */
      pthread_mutex_t     lock;
      uint32_t            size;
      uint32_t            non_sent_size;
      uint32_t            drop_frame_count;
      PayloadData_t       *resend_list;
      PayloadData_t       *last_get_payload;
      uint32_t            last_redundant_ts;
 }SendingBuf_t;
 
SendingBuf_t* SendingBuf_Init(void);
 
int SendingBuf_Insert(SendingBuf_t * send_buf, uint8_t *payload, 
                        uint16_t payload_len, uint32_t timestamp, int marker, 
                        uint8_t *ext_hdr_data, uint16_t ext_len, int rtp_event_flag);
                        
int SendingBuf_RemoveSent( SendingBuf_t *send_buf);

int SendingBuf_RemoveOutofDate(SendingBuf_t *send_buf, int isEndOfFrame);

PayloadData_t *SendingBuf_GetNextToSend( SendingBuf_t *send_buf, 
                                         uint32_t *timestamp, int *end_flag);

int GetSendingBufSize(SendingBuf_t *send_buf);

int SendingBuf_Resend(SendingBuf_t *send_buf, uint16_t seq);

int SendingBuf_RemoveFrameBefore(SendingBuf_t *send_buf, uint32_t rtp_ts);

int SendingBuf_GetStatus(SendingBuf_t *send_buf, uint32_t *frame_count, uint32_t *drop_frame_count);

 void SendingBuf_MoveToNextFrame(SendingBuf_t *send_buf);

int SendingBuf_Destroy(SendingBuf_t *send_buf);
 #endif
