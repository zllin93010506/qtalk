/*
 * FILE:     ET1_QStream_RFC3984.h
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 * This is the RFC3984's routines header file 
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */

#include "config_unix.h"
#include "rtp_callback.h"
#include "ET1_QStream_TransportLayer.h"



int RFC3984_Init();
 
int RFC3984_Packetize(struct TL_Session_t *session, TL_Send_Para_t *para);
 
int RFC3984_FrameComplete(struct TL_Session_t *session, TL_Poll_Para_t *para);
                 
int RFC3984_Unpacketize(struct TL_Session_t *session,TL_Recv_Para_t *para);
                         
 typedef struct _MemoryBlock
 {
 
     unsigned char *memory_base;
     unsigned char *memory_tail;
     int memory_ref;
     
 }MemoryBlock_t;
 
 
 typedef struct _DataBlock
 {
      struct _DataBlock *prev;
      struct _DataBlock *next;
      struct _DataBlock *concate;
      MemoryBlock_t *mb_ptr;
     
     unsigned char *start_ptr;
     unsigned char *end_ptr;
     
 }DataBlock_t;
 
 
 
