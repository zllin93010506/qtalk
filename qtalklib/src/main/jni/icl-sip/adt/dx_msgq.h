/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_msgq.h
 *
 * $Id: dx_msgq.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_msgq.h
* @brief Declare dx message queue related functions
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/
#ifndef DX_MSGQ_H
#define DX_MSGQ_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>
#include "dx_vec.h"

typedef struct dxMsgQObj*	DxMsgQ;

/**
* @fn DxMsgQ       dxMsgQNew(IN int initSize)
* @brief create new dx message queue
* @param initSize - the number for allocating memeory
* @retval DxMsgQ - a pointer to dx message queue
*/
DxMsgQ       dxMsgQNew(IN int initSize);

/**
* @fn void         dxMsgQFree(IN OUT DxMsgQ _this)
* @brief free dx message queue
* @param _this - a pointer to dx message queue
*/
void         dxMsgQFree(IN OUT DxMsgQ _this);

/**
* @fn int          dxMsgQGetLen(IN DxMsgQ _this)
* @brief get current amount of elements in dx message queue
* @param _this - a pointer to dx message queue
* @retval int - current amount of elements in dx message queue
*/
int          dxMsgQGetLen(IN DxMsgQ _this);

/**
* @fn int          dxMsgQSendMsg(IN OUT DxMsgQ _this, IN char* msg, IN unsigned int len)
* @brief Send message to dx message queue; message store in queue as "len|msg"
* @param _this - a pointer to dx message queue
* @param msg - a pointer to message which will be inserted
* @param len - length of message
* @retval int - if success, return the length of message (>=0); if failed, return -1
*/
int          dxMsgQSendMsg(IN OUT DxMsgQ _this, IN char* msg, IN unsigned int len);

/**
* @fn unsigned int dxMsgQGetMsg(IN OUT DxMsgQ _this, OUT char* msg, IN unsigned int len, IN long timeout)
* @brief Get message from message queue.
				Set $timeout==-1 to block until get message.
* @param _this - a pointer to dx message queue
* @param msg - a pointer to memory used to receive message
* @param len - length of $msg
* @param timeout - timeout in millisecond, Set $timeout==-1 to block until get message
* @retval unsigned int - if success, return the length of message (>0); if time-out, return 0; if failed, return -1
*/
unsigned int dxMsgQGetMsg(IN OUT DxMsgQ _this, OUT char* msg, IN unsigned int len, IN long timeout);

/**
* @fn void	dxMsgQSetTraceFlag(IN OUT DxMsgQ _this, IN TraceFlag traceflag)
* @brief set the traceflag of dx message queue
* @param this - a pointer to dx message queue
* @param traceflag - int to indicate traceflag is ON or OFF
*/
void	dxMsgQSetTraceFlag(IN OUT DxMsgQ _this, IN TraceFlag traceflag);

#ifdef  __cplusplus
}
#endif

#endif /* DX_MSGQ_H */
