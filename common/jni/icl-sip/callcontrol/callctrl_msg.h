/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_msg.h
 *
 * $Id: callctrl_msg.h,v 1.2 2008/10/16 03:32:42 tyhuang Exp $
 */

/** \file callctrl_msg.h
 *  \brief Declaration ccMsg struct access functions
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#ifndef CALLCTRL_MSG_H
#define CALLCTRL_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include <DlgLayer/dlglayer.h>
#include "callctrl_cm.h"

/* get call object from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetCALL(IN pCCMsg *const pMsg,OUT pCCCall *pcall)
 *	\brief Function to get call object from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param pcall - pointer will be set as returned call object.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetCALL(IN pCCMsg* const pMsg,OUT pCCCall *pcall);

/* get call state from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetCALLState(IN pCCMsg *const pMsg,OUT ccCallState *cstate)
 *	\brief Function to get call state from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param cstate - pointer will be set as returned call state.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetCALLState(IN pCCMsg* const pMsg,OUT ccCallState *cstate);

/* get request  from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetReq(IN pCCMsg *const pMsg,OUT SipReq *pReq)
 *	\brief Function to get sip request from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param pReq - pointer will be set as returned sip request object.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetReq(IN pCCMsg* const pMsg,OUT SipReq* pReq);

/* get response  from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetRsp(IN pCCMsg *const pMsg,OUT SipRsp *pRsp)
 *	\brief Function to get sip response from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param pRsp is pointer will be set as returned sip response object.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetRsp(IN pCCMsg* const pMsg,OUT SipRsp* pRsp);

/* get dialog(callleg)  from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetCallLeg(IN pCCMsg *const pMsg,OUT pSipDlg *pDlg)
 *	\brief Function to get sip dialog(call leg) object from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param pDlg - pointer will be set as returned sipdlg object.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetCallLeg(IN pCCMsg* const pMsg,OUT pSipDlg *pDlg);


/* get evnt  from callctrl Msg */
/** \fn CCLAPI RCODE ccMsgGetEvent(IN pCCMsg* const pMsg,OUT CallEvtType *pEvt)
 *	\brief Function to get posted event type from a callctrl message object
 *	\param pMsg - pointer to callctrl Msg
 *	\param pEvt - get event with callctrl Msg
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgGetEvent(IN pCCMsg* const pMsg,OUT CallEvtType *pEvt);

/* duplicate a callctrl Msg */
/** \fn CCLAPI RCODE ccMsgDup(IN pCCMsg*const source,OUT pCCMsg* dest)
 *	\brief Function to duplicate a callctrl message object
 *	\param source - source msg
 *	\param dest - new msg after successful deplication.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgDup(IN pCCMsg*const source,OUT pCCMsg* dest);

/* free a callctrl Msg */
/** \fn CCLAPI RCODE ccMsgFree(IN OUT pCCMsg* pMsg)
 *	\brief Function to free a callctrl message object
 *	\param pMsg - pointer ti callctrl Msg will be free
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMsgFree(IN OUT pCCMsg* pMsg);


CCLAPI RCODE ccMsgGetReplacedCall(IN pCCMsg* const pMsg,OUT pCCCall *rpcall );
/* 
 * internal function 
 */
/*create a new callctrl layer message*/
RCODE ccMsgNew(IN  pCCMsg*,IN pCCCall,IN SipReq,IN SipRsp,IN pSipDlg);
RCODE ccMsgSetEvent(IN pCCMsg*,IN CallEvtType);
RCODE ccMsgSetReplacedCall(IN pCCMsg*,IN pCCCall);

#ifdef  __cplusplus
}
#endif

#endif /* CALLCTRL_MSG_H */
