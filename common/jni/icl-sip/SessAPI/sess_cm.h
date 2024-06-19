/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_cm.h
 *
 * $Id: sess_cm.h,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#ifndef SESS_CM_H
#define SESS_CM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif
/* default value for sdp */
#define PROTOCOLVERSION "0" /* use as v=<version> */
#define SESSIONNAME "CCLSDP" /* use as s=<session name> */
#define DEFULT_OWNER "CCLSIP" /* use as o's <username> */
#define DEFAUL_MODIFIER "AS" /* use as b=<modifier><bandwidth-value> */
#define SHOW_SENDRECV 0		/* use as a=sendrecv */

typedef struct sessObj		Sess;
typedef struct sessObj*		pSess;
typedef struct sessmediaObj	SessMedia;
typedef struct sessmediaObj*	pSessMedia;
typedef struct plObj	MediaPayload;
typedef struct plObj*	pMediaPayload;

typedef struct BWobj BWInfo;
typedef struct BWobj* pBWInfo;

typedef enum{
	MEDIA_AUDIO=0,
	MEDIA_VIDEO,
	MEDIA_APPLICATION,
	MEDIA_IMAGE,
	MEDIA_MESSAGE
}MediaType;

typedef enum{
	SENDRECV=0,	/*default*/
	SENDONLY,
	RECVONLY,
	INACTIVE
}StatusAttr;

typedef enum{
	PORT=0,
	PTIME
}MediaParamType;

typedef enum{
	RTPMAP=0,	/*default*/
	FMTP
}RTPParamType;

typedef enum{
	NOT_HOLD=0,  /*default*/
	CZEROS,
	AUDIO_SENDONLY,
	AUDIO_RECVONLY,
	AUDIO_INACTIVE
}HoldStyle;

#define SessPrintEx(X,Y)	TCRPrint ( X, "[SESSAPI] %s", Y);
#define SessPrintEx1(X,Y,Z)	TCRPrint ( X, "[SESSAPI] " Y , Z );
#define SESSWARN(W) TCRPrint ( TRACE_LEVEL_WARNING, "[SESSAPI] WARNING: %s\n",W )
#define SESSERR(W) TCRPrint ( TRACE_LEVEL_ERROR, "[SESSAPI] ERROR: %s (at %s, %d)\n",W, __FILE__, __LINE__)

/************************************************************************/
/* internal function                                                    */
/************************************************************************/
void bwInfoFree(pBWInfo b);

#ifdef  __cplusplus
}
#endif

#endif
