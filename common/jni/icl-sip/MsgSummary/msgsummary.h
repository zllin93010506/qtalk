/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * msgsummary.h
 *
 * $Id: msgsummary.h,v 1.3 2009/05/25 02:48:17 tyhuang Exp $
 */

#ifndef MSGSUMMARY_H
#define MSGSUMMARY_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "msgs_def.h"
#include "msgcount.h"

/* TRUE for "yes", and FALSE for "no" */
CCLAPI pMsgSum MsgSumNew(BOOL msgstatus,const char* accounturi,pMsgCount voicesummary);

CCLAPI void MsgSumFree(pMsgSum* msg);

CCLAPI BOOL MsgSumGetStatus(pMsgSum msg);
CCLAPI RCODE MsgSumSetStatus(pMsgSum msg,BOOL newstate);

CCLAPI char* MsgSumGetAccount(pMsgSum msg);

CCLAPI pMsgCount MsgSumGetSummary(pMsgSum msg,MsgContext type); /* voice-message only */
CCLAPI RCODE MsgSumSetSummary(pMsgSum msg,MsgContext type,pMsgCount mc);

CCLAPI pMsgSum MsgSumNewFromStr(const char* body);

/* this function will return a new string .
 * user need to free it by himself 
 */
CCLAPI char* MsgSumToStr(pMsgSum msg);

#ifdef  __cplusplus
}
#endif

#endif /* MSGSUMMARY_H */
