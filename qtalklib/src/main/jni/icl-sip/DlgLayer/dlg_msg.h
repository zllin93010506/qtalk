/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 *
 * $Id: dlg_msg.h,v 1.2 2008/10/16 03:32:33 tyhuang Exp $
 */
#ifndef DLG_MSG_H
#define DLG_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "dlg_cm.h"
#include <sip/sip.h>

/* get dialog from SipDlgMsg */
/** \fn RCODE sipDlgMsgGetDlg(IN pDlgMsg *const pMsg,OUT pSipDlg *pDlg)
 *	\brief Function to get Dialog reference from posted message
 *	\param *pMsg is a pointer to Dialog Message pointer
 *	\param *pDlg is a pointer to Dialog pointer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgMsgGetDlg(IN pDlgMsg *const pMsg,OUT pSipDlg*);

/* get request  from SipDlgMsg */
/** \fn RCODE sipDlgMsgGetReq(IN pDlgMsg *const pMsg,OUT SipReq *pReq)
 *	\brief Function to get SIP request message from posted message
 *	\param *pMsg is a pointer to Dialog Message pointer
 *	\param *pDlg is a pointer to SIP request pointer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgMsgGetReq(IN pDlgMsg *const pMsg,OUT SipReq*);

/* get response  from SipDlgMsg */
/** \fn RCODE sipDlgMsgGetRsp(IN pDlgMsg *const pMsg,OUT SipRsp *pRsp)
 *	\brief Function to get SIP response message from posted message
 *	\param *pMsg is a pointer to Dialog Message pointer
 *	\param *pDlg is a pointer to SIP response pointer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgMsgGetRsp(IN pDlgMsg *const pMsg,OUT SipRsp *pRsp);

/* get handle(tx) from SipDlgMsg */
/** \fn RCODE sipDlgMsgGetHandle(IN pDlgMsg *pMsg,OUT void **pHandle)
 *	\brief Function to get handle of this event from posted message
 *	\param *pMsg is a pointer to Dialog Message pointer
 *	\param *pDlg is a pointer to SIP response pointer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI RCODE sipDlgMsgGetHandle(IN pDlgMsg *const pMsg,OUT void**);

CCLAPI RCODE sipDlgMsgGetReplacedDlg(IN pDlgMsg *const pMsg,OUT pSipDlg*);
/* internal function */
/*create a new dialog layer message*/
RCODE sipDlgMsgNew(OUT pDlgMsg*,IN pSipDlg,IN SipReq,IN SipRsp,IN void*);
RCODE sipDlgMsgFree(IN OUT pDlgMsg*);
RCODE sipDlgMsgSetReplacedDlg(IN pDlgMsg *const pMsg,IN pSipDlg);


#ifdef  __cplusplus
}
#endif

#endif /* DLG_MSG_H */
