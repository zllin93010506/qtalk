/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_cm.h
 *
 * $Id: callctrl_cm.h,v 1.4 2009/03/11 07:45:13 tyhuang Exp $
 */

#ifndef CALLCTRL_CM_H
#define CALLCTRL_CM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>

#define CCBUFLEN 256
#define CCDBSIZE 4096

#define CCREGISTER_EXPIRE 3600 /* sec,RFC3261 p61*/
#define CCVERSION "1.1" /* 2009/3/11 */

typedef enum {
	CALL_STATE_INITIAL=0, 
	CALL_STATE_PROCEED,
	CALL_STATE_RINGING,
	CALL_STATE_ANSWERED,
	CALL_STATE_CONNECT,
	CALL_STATE_OFFER,
	CALL_STATE_ANSWER,
	CALL_STATE_DISCONNECT,
	CALL_STATE_NULL
}ccCallState;

typedef enum {
	CALL_STATE=0,
	SIP_REQ_REV,
	SIP_RSP_REV,
	SIP_TX_ERR,
	SIP_TX_TIMEOUT,
	SIP_CANCEL_EVENT,

#ifdef ICL_NO_MATCH_NOTIFY
	SIP_NO_MATCH_REQ, /* event for message-waiting special edition */
#endif
	
	NULL_EVENT	
}CallEvtType;

typedef enum {
	Cont_Disposition=0,
	Cont_Encoding,
	Cont_Language
}ContParam;

typedef struct ccCallObj		CCCall;
typedef struct ccCallObj*		pCCCall;
typedef struct ccContentObj		Cont;
typedef struct ccContentObj*	pCont;
typedef struct ccMsgObj			CCMsg;
typedef struct ccMsgObj*		pCCMsg;
typedef struct ccUserObj		CCUser;
typedef struct ccUserObj*		pCCUser;

#define CCPrintEx(X,Y)	TCRPrint ( X, "[CallCtrl] %s", Y);
#define CCPrintEx1(X,Y,Z)	TCRPrint ( X, "[CallCtrl] " Y , Z );
#define CCPrintEx2(X,Y,Z,W)	TCRPrint ( X, "[CallCtrl] " Y , Z ,W);
#define CCWARN(W) TCRPrint ( TRACE_LEVEL_WARNING, "[CallCtrl] WARNING: %s",W )
#define CCERR(W) TCRPrint ( TRACE_LEVEL_ERROR, "[CallCtrl] ERROR: %s (at %s, %d)\n",W, __FILE__, __LINE__)

/* sip related define */
#define ACCEPT_CTYPE "application/sdp"
#define ACCEPT_LANGUAGE "en"
#define ACCEPT_ENCODING "identity" /* accept-encoding "" means no encode */

#define RFC_MIN_SE 90

#ifdef  __cplusplus
}
#endif

#endif
