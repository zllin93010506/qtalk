/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_call.h
 *
 * $Id: callctrl_call.h,v 1.3 2008/11/03 07:00:02 tyhuang Exp $
 */

/** \file callctrl_call.h
 *  \brief Declaration ccCall struct access functions
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#ifndef CALLCTRL_CALL_H
#define CALLCTRL_CALL_H


#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include <DlgLayer/dlglayer.h>
#include "callctrl_cm.h"

typedef void (*CallEvtCB)(CallEvtType event, void* msg);

/* call control layer library initial */
/** \fn RCODE CallCtrlInit(IN CallEvtCB eventCB,IN SipConfigData config,IN const char *realm)
 *	\brief Function to initiate the callcontrol layer library
 *	\param eventCB - a pointer to event call back function
 *	\param config - an input config 
 *	\param realm - realm for authentication
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE CallCtrlInit(IN CallEvtCB eventCB,IN SipConfigData config,IN const char *realm);

/* library clean function */
/** \fn RCODE CallCtrlClean(void)
 *	\brief Function to clean the call control layer library
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE CallCtrlClean(void);

/* dispatch callcontrol events */
/** \fn RCODE CallCtrlDispatch(IN const int timeout)
 *	\brief Function to execute event dispatch
 *	\param timeout - a integer of period of timeout
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE CallCtrlDispatch(IN const int timeout);

/* input callid for new call object */
/*ccCallNew(callobject,host_ip_addr) */
/** \fn RCODE ccCallNew(IN OUT pCCCall *pcall)
 *	\brief Function to new a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallNew(IN OUT pCCCall *pcall);
CCLAPI RCODE ccCallNewEx(IN OUT pCCCall *pcall,IN const char* callid);

/* free call object */
/** \fn RCODE ccCallFree(IN OUT pCCCall *pcall)
 *	\brief Function to free a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallFree(IN OUT pCCCall *pcall);

/* get callid from call object */
/** \fn RCODE ccCallGetCallID(IN  pCCCall *const pcall ,OUT char *callid, IN OUT UINT32* length)
 *	\brief Function to get CALL-ID from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param callid - for output buffer
 *	\param length - to report the length of CALL-ID
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallGetCallID(IN pCCCall *const pcall,OUT char *callid, IN OUT UINT32* length);

/* get call state from call object */
/** \fn RCODE ccCallGetCallState(IN  pCCCall *const pcall ,OUT ccCallState *cs)
 *	\brief Function to get call state from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param cs - for output callstate
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallGetCallState(IN pCCCall *const pcall,OUT ccCallState *cs);

/* set parent data to call object */
/** \fn RCODE ccCallSetParent(IN pCCCall *const pcall,IN void * parent)
 *	\brief Function to set parent to a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param parent - for input parent
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallSetParent(IN pCCCall *const pcall,IN void * parent);

/* get parent data from call object */
/** \fn RCODE ccCallGetParent(IN pCCCall *const pcall,OUT void **parent)
 *	\brief Function to get parent from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param parent - for output parent
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallGetParent(IN pCCCall *const pcall,OUT void **parent);

/* set user information to call object */
/** \fn RCODE ccCallSetUserInfo(IN pCCCall *const pcall,IN pCCUser user)
 *	\brief Function to set user information from a call object;it's used in from/contact header
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param user - for input user information
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallSetUserInfo(IN pCCCall *const pcall,IN pCCUser user);

/* get user information from call object */
/** \fn RCODE ccCallGetUserInfo(IN pCCCall *const pcall,OUT pCCUser *user)
 *	\brief Function to get user information from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param user - for output user information
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallGetUserInfo(IN pCCCall *const pcall,OUT pCCUser *user);

/* generate referto url from call object */
/** \fn RCODE ccGetReferToUrl(IN pCCCall *const pcall,OUT char *rurl, IN OUT UINT32* length)
 *	\brief Function to generate refer-to url from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param rurl - a buffer for output refer-to url
 *	\param length - the buffer length
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccGetReferToUrl(IN pCCCall *const pcall,OUT char *rurl, IN OUT UINT32* length);

/* generate replace-id from call object */
/** \fn RCODE ccGetReplaceID(IN pCCCall *const pcall,OUT char *rid, IN OUT UINT32* length,IN BOOL uas);
 *	\brief Function to generate replace-id from a call object;it's used when attended transfer
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param rid - a buffer for output replace-id
 *	\param length - the buffer length
 *	\param uas - if uas replaceid=callid;to-tag=localtag;from-tag=remotetag,else replaceid=callid;to-tag=remotetag;from-tag=localtag
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccGetReplaceID(IN pCCCall *const pcall,OUT char *rid, IN OUT UINT32* length,IN BOOL uas);

/* get invite dialog(callleg) from call object */
/** \fn RCODE ccGetInviteDlg(IN pCCCall *const pcall,OUT pSipDlg *pdlg)
 *	\brief Function to get invite dialog from a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param pdgl - a buffer for output dialog
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccGetInviteDlg(IN pCCCall *const pcall,OUT pSipDlg *pdlg);

/* ckeck if transaction is complete(get response) */
/** \fn RCODE ccCallCheckComplete(IN pCCCall *const pcall,OUT BOOL *progress)
 *	\brief Function to ckeck if transaction is complete(get response)
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param progress - BOOL;TRUE for completed,FALSE for in-completed
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccCallCheckComplete(IN pCCCall *const pcall,OUT BOOL *progress);

/* basic call api*/
/* send INVITE */
/** \fn RCODE ccMakeCall(IN  pCCCall *const pcall,IN const char *dest,IN pCont msgbody,IN SipReq refer)
 *	\brief Function to send INVITE by a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param dest - to indicate the remote URL in initiate a session
 *	\param msgbody - an additional message body 
 *	\param refer - a Sip Request of REFER.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccMakeCall(IN pCCCall *const pcall,IN const char* dest,IN pCont msgbody,IN SipReq refer);

/* send 1xx message ex.180 */
/** \fn RCODE ccRingCall(IN  pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody)
 *	\brief Function to send a provisional response (1xx),except 100.
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param code - the status code of response;MUST in the range (101~199)
 *	\param reason - the reason phase of response;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccRingCall(IN pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody);
CCLAPI RCODE ccRingCallEx(IN pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN SipRsp rsp);


/* send 2xx response for invite */
/** \fn RCODE ccAnswerCall(IN pCCCall *const pcall,IN pCont msgbody)
 *	\brief Function to send "200 OK" response by a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccAnswerCall(IN pCCCall *const pcall,IN pCont msgbody);
CCLAPI RCODE ccAnswerCallEx(IN pCCCall *const pcall,IN pCont msgbody,IN SipRsp rsp);

/* send ACK */
/** \fn RCODE ccAckCall(IN  pCCCall *const pcall,IN pCont msgbody,IN SipRsp rsp)
 *	\brief Function to send ACK by a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccAckCall(IN pCCCall *const,IN pCont msgbody);

/* send bye or cancel depend on call state */
/** \fn RCODE ccDropCall(IN  pCCCall *const pcall,IN pCont msgbody)
 *	\brief Function to send BYE or CANCEL by a call object to drop a call.before connect,the method will be CANCEL
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccDropCall(IN pCCCall *const pcall,IN pCont msgbody);

/* send 302 response ,must input a new SIP URL */
/** \fn RCODE ccForwardCall(IN pCCCall *const pcall,IN const char *newtarget,IN pCont msgbody)
 *	\brief Function to send a 302 response for redirect call
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param newtarget - the new Remote URL for 302 response.
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccForwardCall(IN pCCCall *const pcall,IN const char *newtarget,IN pCont msgbody);

/* send 4XX~6XX response */
/** \fn RCODE ccRejectCall(IN  pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody)
 *	\brief Function to send a failure final response (4xx~6xx),except 481.
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param code - the status code of response;MUST in the range (400~699)
 *	\param reason - the reason phase of response;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccRejectCall(IN pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody);
CCLAPI RCODE ccRejectCallEx(IN pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN SipRsp rsp);

/** \fn RCODE ccModifyCall(IN pCCCall *const pcall,IN pCont msgbody)
 *	\brief Function to send a re-Invite message to modify call
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccModifyCall(IN pCCCall *const pcall,IN pCont msgbody);

/** \fn RCODE ccUpdateCall(IN pCCCall *const pcall,IN pCont msgbody)
 *	\brief Function to send a UPDATE message to update call
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccUpdateCall(IN pCCCall *const pcall,IN pCont msgbody);

/** \fn RCODE (IN pCCCall *const pcall,IN const char* target,IN const char* replaceid,IN pCont msgbody)
 *	\brief Function to send a REFER message to transfer call
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param target - sip url of transfer target
 *	\param replaceid - the replace identity for attended transfer
 *	\param msgbody - an additional message body 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccTransferCall(IN pCCCall *const pcall,IN const char* target,IN const char* replaceid,IN pCont msgbody);

/*
 * other API to handle messages out of dialog
 * register/message/info/publish/options  
 */
/* new request */
/** \fn RCODE ccNewReqFromCall(IN  pCCCall *const pcall,OUT SipReq *preq,IN SipMethodType method,IN const char *remoteurl,IN pCont msgbody)
 *	\brief Function to create a request message from call with a remote target,it is used to out of dialog message			
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param preq - pointer to sip request pointer
 *	\param method - the SIP METHOD of the request
 *	\param remoteurl - the URL of remote party
 *  \Param msgbody - the message body
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccNewReqFromCall(IN pCCCall *const pcall,OUT SipReq* preq,IN SipMethodType method,IN const char* remoteurl,IN pCont msgbody);

/* new response */
/** \fn RCODE ccNewRspFromCall(IN  pCCCall *const pcall,OUT SipRsp *prsp,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN SipReq req)
 *	\brief Function to create a response message from call with original request,it is used to out of dialog message			
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param prsp - a point to the output SipReq
 *	\param code - the status code of response;MUST in the range (101~999)
 *	\param reason - the reason phase of response;NOT NULL
 *  \Param msgbody - the message body
 *	\param reqid - pointer to string ,is id of sip request ;NOT NULL
 *	\param dlg - pointer to pSipDlg ,is callleg of request
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccNewRspFromCall(IN pCCCall *const pcall,OUT SipRsp* prsp,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN const char* reqid,IN pSipDlg dlg);

/* send request */
/** \fn RCODE ccSendReq(IN  pCCCall *const pcall,IN SipReq req,IN pSipDlg dlg)
 *	\brief Function to send request message by a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param req - sip request object;NOT NULL
 *	\param dlg - the dialog user want to use to send request
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccSendReq(IN pCCCall *const,IN SipReq,IN pSipDlg);

/* new response */
/** \fn RCODE ccSendRsp(IN  pCCCall *const pcall,IN SipRsp rsp)
 *	\brief Function to send response message by a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param rsp - sip response object;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccSendRsp(IN pCCCall *const pcall,IN SipRsp rsp);

/*
 * other API to handle messages in of dialog
 * subscribe/NOTIFY/REFER 
 */
/** \fn RCODE ccNewCallLegFromCall(IN pCCCall *const pcall,IN const char* remoteurl, OUT pSipDlg *Legid)
 *	\brief Function to create a call leg with a remote target.			
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param remoteurl - the URL of remote party
 *  \Param Legid - new call leg(dialog)
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccNewCallLegFromCall(IN pCCCall *const pcall,IN const char* remoteurl,OUT pSipDlg *Legid);

/** \fn RCODE ccFreeCallLeg(IN pSipDlg *Legid)
 *	\brief Function to free a call leg of a call			
 *  \Param Legid - call leg(dialog) will be freed
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccFreeCallLeg(IN pSipDlg *Legid);

/** \fn RCODE ccNewReqFromCallLeg(IN pCCCall *const pcall,OUT SipReq *preq,IN SipMethodType method,IN const char* remoteurl,IN pCont msgbody,IN OUT pSipDlg *Legid)
 *	\brief Function to create a request message from call leg,it is used to in-dialog message			
 *	\param pcall - a pointer to pCCCall  pointer;NOT NULL
 *	\param preq - pointer to sip request pointer
 *	\param method - the SIP METHOD of the request
 *	\param remoteurl - the URL of remote party
 *  \Param msgbody - the message body
 *  \Param Legid - call leg(dialog) be used to gerenate sip request
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE ccNewReqFromCallLeg(IN pCCCall *const pcall,OUT SipReq* preq,IN SipMethodType method,IN const char* remoteurl,IN pCont msgbody,IN OUT pSipDlg *Legid);

/************************************************************************/
/* internal functions                                                   */
/************************************************************************/
RCODE ccGetCallDBSize(OUT INT32* ret);
RCODE ccSetCallToDB(IN pCCCall *pcall);
RCODE ccGetCallFromDB(IN pCCCall *pcall);

/* set out bound sip server to call object */
/** \fn RCODE ccCallSetOutBound(IN pCCCall *const pcall,IN const char *sipurl)
 *	\brief Function to set out bound sip server information to a call object
 *	\param pcall - a pointer to pCCCall pointer;NOT NULL
 *	\param sipurl - for output sip server information;like "sip:proxy address:port"
 *	\param localip - for use specific local ip(net interface) to send message;default is NULL
 *	\param localport - for use specific local port to send message;default is zero
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
RCODE ccCallSetOutBound(IN pCCCall *const pcall,IN const char *sipurl,IN const char* localip,IN UINT16 localport);

RCODE ccSetREALM(IN const char * newrealm);

RCODE ccSetRemoteDisplayName(IN const char * displayname);

#ifdef  __cplusplus
}
#endif

#endif /* CALLCTRL_CALL_H */
