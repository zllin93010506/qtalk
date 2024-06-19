/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * Dlg_Util.h
 *
 * $Id: dlg_util.h,v 1.3 2008/11/10 07:41:04 tyhuang Exp $
 */
#ifndef DLG_UTIL_H
#define DLG_UTIL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <adt/dx_lst.h>
#include <sip/sip_cm.h>
#include <sip/sip_cfg.h>
#include <sipTx/sipTx.h>
#include <DlgLayer/dlg_cm.h>

/* define for event callback function for dialog layer */
typedef void (*DlgEvtCB)(DlgEvtType event, void* msg);

/*Initiate Dialog Layer library */
/** \fn RCODE sipDlgInit(IN DlgEvtCB eventCB,IN SipConfigData config)
 *	\brief Function to initiate the dialog layer library
 *	\param eventCB is a pointer to event call back function
 *	\param config is an input config 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgInit(IN DlgEvtCB eventCB,IN SipConfigData config);

/*clean Dialog Layer library */
/** \fn RCODE sipDlgClean(void)
 *	\brief Function to clean the dialog layer library
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgClean(void);

/*Dialog layer event dispatch function */
/** \fn RCODE sipDlgDispatch(IN const int timeout)
 *	\brief Function to execute event dispatch
 *	\param timeout is a integer of period of timeout
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgDispatch(IN const INT32 timeout);

/* create a  Dialog object */
/*sipDlgNew(pDlg,callid,localurl,remoteurl,routeset) */
/** \fn RCODE sipDlgNew(OUT pSipDlg *pdlg,IN const char *callid,IN const char *localurl,IN const char *remoteurl,IN DxLst routeset)
 *	\brief Function to create a object of dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *callid is a input string of call-id;NOT NULL
 *	\param *localurl is input string of local url;NOT NULL
 *	\param *remoteurl is input string of remote url;NOT NULL
 *	\param routeset is a list of sip url for routing
 *	\param localtag is a input for specific from-tag
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgNew(OUT pSipDlg *pdlg,IN const char *callid,IN const char *localurl,IN const char *remoteurl,IN DxLst routeset,IN const char *localtag);

/* free a SipDlg object */
/** \fn RCODE sipDlgFree(IN OUT pSipDlg *pdlg)
 *	\brief Function to free a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgFree(IN OUT pSipDlg *pdlg);

/* get callid of this dialog */
/** \fn RCODE sipDlgGetCallID(IN pSipDlg *const pdlg,OUT char *callid ,IN OUT int *len)
 *	\brief Function to get callid of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *callid is point to buffer;NOT NULL
 *	\param *length the length of buffer,and indicate the length of callid when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetCallID(IN pSipDlg *const pdlg,OUT char *callid ,IN OUT INT32 *len);

/* get Local url of this dialog */
/** \fn RCODE sipDlgGetLocalURL(IN pSipDlg *const pdlg,OUT char *localurl,IN OUT int *len)
 *	\brief Function to get local url of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *localurl is point to buffer;NOT NULL
 *	\param *length the length of buffer,and indicate the length of local url when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetLocalURL(IN pSipDlg *const pdlg,OUT char *localurl,IN OUT INT32 *len);

/* get Local tag of this dialog */
/** \fn RCODE sipDlgGetLocalTag(IN pSipDlg *const pdlg,OUT char *localtag,IN OUT int *len)
 *	\brief Function to get local tag of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *localtag is point to buffer;NOT NULL
 *	\param *length the length of buffer,and indicate the length of local tag when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetLocalTag(IN pSipDlg *const pdlg,OUT char *localtag,IN OUT INT32 *len);

/* get Local command sequence of this dialog */
/** \fn RCODE sipDlgGetLocalCseq(IN pSipDlg *const pdlg,OUT int *lcseq)
 *	\brief Function to get local cseq of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *lcseq is to indicate the local cseq
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetLocalCseq(IN pSipDlg *const pdlg,OUT UINT32* lcseq);

/* set Local command sequence of this dialog */
/** \fn RCODE sipDlgSetLocalCseq(IN pSipDlg *const pdlg,IN int lcseq)
 *	\brief Function to get local cseq of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param lcseq is to new value for the local cseq and it must be larger than old one.
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSetLocalCseq(IN pSipDlg *const pdlg,IN UINT32 lcseq);

/* get Remote url of this dialog */
/** \fn RCODE sipDlgGetRemoteURL(IN pSipDlg *const pdlg,OUT char *remoteurl,IN OUT int *len)
 *	\brief Function to get remote url of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *remoteurl is point to buffer;NOT NULL
 *	\param *length of buffer,and indicate the length of remote url when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetRemoteURL(IN pSipDlg *const pdlg,OUT char *remoteurl,IN OUT INT32 *len);

/* get Remote Tag of this dialog */
/** \fn RCODE sipDlgGetRemoteTag(IN pSipDlg *const pdlg,OUT char *remotetag,IN OUT int *len)
 *	\brief Function to get remote tag of a dialog object
 *	\param *pdlg is a pointer t pSipDlg pointer;NOT NULL
 *	\param *remotetag is pointer to buffer;NOT NULL
 *	\param *length the length of buffer,and indicate the length of remote tag when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetRemoteTag(IN pSipDlg *const pdlg,OUT char *remotetag,IN OUT INT32 *len);

/* get Remote target(contact) of this dialog */
/** \fn RCODE sipDlgGetRemoteTarget(IN pSipDlg *const pdlg,OUT char *remotetarget,IN OUT int *len)
 *	\brief Function to get remote target url(contact address) of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *remotetarget is pointer to buffer;NOT NULL
 *	\param *length the length of buffer,and indicate the length of remote target when return
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetRemoteTarget(IN pSipDlg *const pdlg,OUT char *remotetarget,IN OUT INT32 *len);

/* get Remote command sequence of this dialog */
/** \fn RCODE sipDlgGetRemoteCseq(IN pSipDlg *const pdlg,OUT int *rcseq)
 *	\brief Function to get remote cseq of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *rcseq is pointer to integer to indicate the remote cseq
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetRemoteCseq(IN pSipDlg *const pdlg,OUT INT32 *rcseq);

/* get Route Set of this dialog */
/** \fn RCODE sipDlgGetRouteSet(IN pSipDlg *const pdlg,OUT DxLst* routeset)
 *	\brief Function to get route set of a dialog object,
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *routeset is point to DxList point;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetRouteSet(IN pSipDlg *const pdlg,OUT DxLst* routeset);

/* set Route Set sequence of this dialog */
/** \fn RCODE sipDlgSetRouteSet(IN pSipDlg *const pdlg,IN DxLst routeset)
 *	\brief Function to set route set of a dialog object;NOTE it will add route set in dialog to out-list
 *	\param *pdlg is a pointer to pSipDlg pointer
 *	\param *routeset is point to DxList point
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSetRouteSet(IN pSipDlg *const pdlg,IN DxLst routeset);

/* get state of this dialog */
/** \fn RCODE sipDlgGetDlgState(IN pSipDlg *const pdlg,OUT DlgState *state)
 *	\brief Function to set route set of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *state is point to DlgState
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetDlgState(IN pSipDlg *const pdlg,OUT DlgState *state);

CCLAPI BOOL beSipDlgTerminated(IN pSipDlg *const pdlg);

/* get secure flag of this dialog */
/** \fn RCODE sipDlgGetSecureFlag(IN pSipDlg *const pdlg,OUT BOOL *secureflag)
 *	\brief Function to get secure flag of a dialog object
 *	\param *pdlg is a pointer of pSipDlg pointer;NOT NULL
 *	\param *state is point to BOOL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetSecureFlag(IN pSipDlg *const pdlg,OUT BOOL *secureflag);

/* use Dialog to Send a Request */
/* if return RC_OK,dialog layer will take care request
 * if return RC_ERROR, user need to free request by himself 
 */
/** \fn RCODE sipDlgSendReq(IN pSipDlg *const pDlg,IN SipReq req)
 *	\brief Function to send request message of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param req is sip request object;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSendReq(IN pSipDlg *const pDlg,IN SipReq req);

/** \fn RCODE sipDlgSendReqEx(IN pSipDlg *const pDlg,IN SipReq req,IN UserProfile up)
 *	\brief Function to send request message of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param req is sip request object;NOT NULL
 *	\param up is UserProfile(out bound proxy setting) for sending request
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSendReqEx(IN pSipDlg *const pDlg,IN SipReq req,IN UserProfile up);

/* use Dialog to Send a Response */
/* if return RC_OK,dialog layer will take care response
 * if return RC_ERROR, user need to free response by himself 
 */
/** \fn RCODE sipDlgSendRsp(IN pSipDlg *const pDlg,IN SipRsp rsp)
 *	\brief Function to send response message of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param rsp is sip response object;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSendRsp(IN pSipDlg *const pDlg,IN SipRsp rsp);

/* use Dialog to Send a Response */
/* if return RC_OK,dialog layer will take care response
 * if return RC_ERROR, user need to free response by himself 
 */
/** \fn RCODE sipDlgSendRspEx(IN void *handle,IN SipRsp rsp)
 *	\brief Function to send response message by handler without dialog
 *	\param *handle is a pointer to handler;NOT NULL
 *	\param rsp is sip response object;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSendRspEx(IN void *handle,IN SipRsp rsp);

/* use Dialog to create a Request */
/** \fn RCODE sipDlgNewReq(IN pSipDlg *const pdlg,OUT SipReq *preq,IN SipMethodType method)
 *	\brief Function to create a request message from dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *preq is pointer to sip request object;NOT NULL
 *	\param method is the SIP METHOD of the request
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgNewReq(IN pSipDlg *const pdlg,OUT SipReq *preq,IN SipMethodType method);

/* use Dialog to create a Response  */
/* default response code =200 ok	*/
/** \fn RCODE sipDlgNewRsp(OUT SipRsp* prsp,IN SipReq req,IN const char *statuscode,IN const char *phase)
 *	\brief Function to create a response message from request
 *	\param *prsp is a pointer to SipRsp;NOT NULL
 *	\param req is sip request object;NOT NULL
 *	\param *statuscode is the string of status code
 *	\param *phase is the string of reason phase
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgNewRsp(OUT SipRsp* prsp,IN SipReq req,IN const char *statuscode,IN const char *phase);

/* get inviteTx from this dialog */
/**	\fn CCLAPI RCODE sipDlgGetInviteTx(IN pSipDlg *const pdlg,IN TxStruct *pTx)
 *	\brief get a progressing TX_SERVER_INVITE transaction from dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *  \param beServerTx is to specify if invite server transactio or invite client transaction
 *	\param *pTx is a pointer to carry return Tx
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetInviteTx(IN pSipDlg *const pdlg,BOOL beServerTx,OUT TxStruct *pTx);

/* get request from this dialog */
/**	\fn CCLAPI RCODE sipDlgGetInviteTx(IN pSipDlg *const pdlg,IN TxStruct *pTx)
 *	\brief get a progressing TX_SERVER_INVITE transaction from dialog
 *	\param *pReq is a pointer to SipReq pointer to carry return request
 *	\param *branchid is via branchid used to match request ;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetReqByBranchID(IN pSipDlg *const pdlg,OUT SipReq *pReq,IN const char* branchid);

/* check if there is still a progressing/trying tx */
/**	\fn CCLAPI RCODE sipDlgCheckComplete(IN pSipDlg *const pdlg,OUT BOOL *progress)
 *	\brief get a progressing TX_SERVER_INVITE transaction from dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *progress is a pointer to identify if complete or not
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgCheckComplete(IN pSipDlg *const pdlg,OUT BOOL *progress);

/* get user data of this dialog */
/** \fn RCODE sipDlgGetUserData(IN pSipDlg *const pdlg,OUT void** user)
 *	\brief Function to get UserData of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param **user is void point,will set to UserData 
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgGetUserData(IN pSipDlg *const pdlg,OUT void** user);

/* set user data sequence of this dialog */
/** \fn RCODE sipDlgSetUserData(IN pSipDlg *const pdlg,IN void *user)
 *	\brief Function to set UserData of a dialog object
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param **user is void point;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSetUserData(IN pSipDlg *const pdlg,IN void *user);

/* set allow-event header for 489 response */
/** \fn RCODE sipDlgSetAllowEvent(IN const char *ae)
 *	\brief Function to set allow-event value.
 *	\param ae is string;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgSetAllowEvent(IN const char *ae);

CCLAPI RCODE sipDlgSetUserAgent(IN const char *ua);

/* check if PRACK is need */
/** \fn RCODE sipDlgNeedPRAck(IN pSipDlg *const pdlg,OUT BOOL *needprack)
 *	\brief get a progressing TX_SERVER_INVITE transaction from dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *needprack is a pointer to identify if still needs to prack or not
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgNeedPRAck(IN pSipDlg *const pdlg,OUT BOOL *needprack);

/* check if tags are the same */
/** \fn RCODE sipDlgBeMatchTag(IN pSipDlg *const pdlg,IN const char* localtag,IN const char* remotetag)
 *	\brief get a progressing TX_SERVER_INVITE transaction from dialog
 *	\param *pdlg is a pointer to pSipDlg pointer;NOT NULL
 *	\param *localtag is a string of local tag
 *	\param *remotetag is a string of remote tag
 *	\retval BOOL TRUE for matched;FALSE for dismatched.
 */
CCLAPI BOOL sipDlgBeMatchTag(IN pSipDlg *const pdlg,IN const char* localtag,IN const char* remotetag);

/************************************************************************/
/* internal function                                                    */
/************************************************************************/
RCODE sipDlgSetRRTrue(IN pSipDlg *const pdlg);
RCODE sipDlgSetDB(pSipDlg *pdlg);
RCODE sipDlgRemoveFromDB(pSipDlg *pdlg);

RCODE sipDlgSetTxEtimeInterval(UINT8 newvalve);

#ifdef  __cplusplus
}
#endif

#endif
