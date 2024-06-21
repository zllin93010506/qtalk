/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_event.h
 *
 * $Id: cx_NU_event.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#ifndef CX_EVENT_H 
#define CX_EVENT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "cx_NU_sock.h"

typedef struct cxEventObj*	CxEvent;

typedef enum {
	CX_EVENT_UNKNOWN =	0,
	CX_EVENT_RD =		0x01,
	CX_EVENT_WR =		0x02,
	CX_EVENT_EX =		0x04
} CxEventType;

typedef void (*CxEventCB)(CxSock sock, CxEventType event, int err, void* context);

CxEvent	cxEventNew(void);
RCODE	cxEventFree(CxEvent);
RCODE	cxEventRegister(CxEvent, CxSock, CxEventType, CxEventCB, void* context);
RCODE	cxEventUnregister(CxEvent, CxSock, void** context);
int	cxEventDispatch(CxEvent, int timeout/*msec; -1 for blocking*/);

#ifdef  __cplusplus
}
#endif

#endif /* CX_EVENT_H */