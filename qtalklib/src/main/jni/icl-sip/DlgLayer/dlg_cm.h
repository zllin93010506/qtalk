/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * Dlg_cm.h
 *
 * $Id: dlg_cm.h,v 1.5 2009/03/11 07:53:40 tyhuang Exp $
 */
#ifndef DLG_CM_H
#define DLG_CM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>
	
#ifndef DLGBUFFERSIZE
#define DLGBUFFERSIZE 256
#endif
	
#define DLGDBSIZE 2048

#define SipTxCount 10240
#define OWNER "iclsip" /* may change for user and maxlength is 20 chars */
#define DLG_SIP_VERSION	"SIP/2.0" /* SIP/2.0 is available only */
#define SIP_MAX_FORWARDS 70
#define INVITE_CALL "invitecall" /* must not change */
#define SUPPORTEVENT "presence" /* if support new event should add it */

#define ALLOW_METHOD "INVITE,CANCEL,BYE,REFER,SUBSCRIBE,NOTIFY,MESSAGE,UPDATE,OPTIONS"

	typedef enum {	  /* based on RFC 4235 */
	DLG_STATE_NULL=0, /* use for a non-dialog state */
	DLG_STATE_TRYING, /* change from INI to TRYING */
	DLG_STATE_PROCEEDING,  /*1xx no-tag ,for report noly */
	DLG_STATE_EARLY,	  /* 1xx with tag */
	DLG_STATE_CONFIRM,
	DLG_STATE_TERMINATED
}DlgState;

typedef enum{
	/* message receive event ,should take care of them */
	DLG_MSG_REQ=0,
	DLG_MSG_RSP,
	DLG_MSG_ACK,
	/*error report event */
	DLG_MSG_TRANSPORTERROR,
	DLG_2XX_EXPIRE,
	DLG_MSG_NOMATCH_REQ,
	DLG_MSG_NOMATCH_RSP,
	DLG_MSG_TX_TERMINATED,
	DLG_MSG_TX_TIMEOUT,
	DLG_CANCEL_EVENT,
	DLG_MSG_TX_ETIME
}DlgEvtType;

typedef enum
{
	txu_call=0,
	txu_dlg
}SipTxUser;

typedef struct sipDlgObj		SipDlg;
typedef struct sipDlgObj*		pSipDlg;
typedef struct DlgMsgObj		DlgMsg;
typedef struct DlgMsgObj*		pDlgMsg;

typedef struct UserDataObj{
	SipTxUser	usertype;
	void		*data;
}UserData;


#ifdef ICL_DLGLAYER_ENABLE_TRACE /* CCL_DLGLAYER_ENABLE_TRACE */
#define DlgPrintEx(X,Y)	TCRPrint ( X, "[DLGLAYER] %s", Y);
#define DlgPrintEx1(X,Y,Z)	TCRPrint ( X, "[DLGLAYER] " Y , Z );
#define DlgPrintEx2(X,Y,Z,W)	TCRPrint ( X, "[DLGLAYER] " Y , Z ,W );
#else
#define DlgPrintEx(X,Y)
#define DlgPrintEx1(X,Y,Z)
#define DlgPrintEx2(X,Y,Z,W)
#endif

#define DLGWARN(W) TCRPrint ( TRACE_LEVEL_WARNING, "[DLGLAYER] WARNING: %s\n",W )
#define DLGERR(W) TCRPrint ( TRACE_LEVEL_ERROR, "[DLGLAYER] ERROR: %s (at %s, %d)\n",W, __FILE__, __LINE__)

#define SUPPORT_EXTEND "replaces,100rel"
#define SIP_USER_AGENT "Quanta_SIP_V_001" /* ICL_SIP_V_171 */

/* memory function */

void FreeMemory(void *pv);

#ifdef  __cplusplus
}
#endif

#endif
