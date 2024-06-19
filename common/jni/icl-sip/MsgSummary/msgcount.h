/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * msgcount.h
 *
 * $Id: msgcount.h,v 1.2 2009/05/25 02:48:17 tyhuang Exp $
 */

#ifndef MSGCOUNT_H
#define MSGCOUNT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "msgs_def.h"

CCLAPI pMsgCount MsgCountNew(int newmsg,int oldmsg,int newumsg,int oldumsg);
CCLAPI pMsgCount MsgCountDup(pMsgCount oldmc);
CCLAPI void MsgCountFree(pMsgCount* mc);

CCLAPI int MsgCountGetCount(pMsgCount mc,MsgCountType);
CCLAPI RCODE MsgCountSetCount(pMsgCount mc,MsgCountType ,int size);

CCLAPI pMsgCount MsgCountNewFromStr(const char* body);
CCLAPI char* MsgCountToStr(pMsgCount mc);

#ifdef  __cplusplus
}
#endif

#endif /* MSGSUMMARY_H */
