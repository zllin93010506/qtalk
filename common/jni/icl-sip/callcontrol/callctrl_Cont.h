/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_Cont.h
 *
 * $Id: callctrl_Cont.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

/** \file callctrl_Cont.h
 *  \brief Declaration ccContentObj struct access functions
 *  \author PENG, Wei-Chiang, wchpeng@itri.org.tw
 *  \date Last modify at 2004/4/10 20:03
 *  \version 0.001
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#ifndef CALLCTRL_CONT_H
#define CALLCTRL_CONT_H


#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include "callctrl_cm.h"

/* create/destory content object */
/** \fn CCALPI RCODE ccContNew( OUT pCont *pcont, IN const char *type, IN char *body)
 *  \brief Function to new a content object
 *  \param pcont - a content object pointer
 *  \param type - content object type, like : application/sdp
 *  \param body - content body string 
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContNew( OUT pCont *pcont, IN const char *type, IN const char *body);
CCLAPI RCODE ccContNewEx( OUT pCont *pcont, IN const char *type,IN unsigned int len, IN const char *body);

/* new content from input request */
/** \fn CCALPI RCODE ccContNewFromReq( OUT pCont *const pcont,IN const SipReq req)
 *  \brief Function to new a content object by request message body
 *  \param pcont - a content object pointer
 *  \param req - request mossage
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContNewFromReq( OUT pCont *pcont,IN const SipReq req);

/* new content from input response */
/** \fn CCLAPI RCODE ccContNewFromRsp( OUT pCont *pcont,IN const SipRsp rsp)
 *  \brief Function to new a content object by response message body
 *  \param pcont - a content object pointer
 *  \param rsp - response mossage
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContNewFromRsp(OUT pCont *pcont,IN const SipRsp);

/* new sipfrag content from response;this function will alloc memory if return RC_OK*/
/** \fn CCLAPI RCODE ccRspNewSipFreg(OUT pCont *pcont,IN const SipRsp)
 *  \brief Function to new a content object by response status line with type=message/sipfrag;version=2.0
 *  \param pcont - a content object pointer
 *  \param rsp - response mossage
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccRspNewSipFrag(OUT pCont *pcont,IN const SipRsp rsp);

/* content free function */
/** \fn CCLAPI RCODE ccContFree( IN pCont *pcont)
 *  \brief Function to delete a content object
 *  \param pcont - a content object pointer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContFree( IN pCont *pcont);

/* duplicate content object */
/** \fn CCLAPI RCODE ccContDup( IN pCont *const porg, OUT pCont *pdup)
 *  \brief Function to duplicate a content object
 *  \param porg - original content object pointer
 *  \param pdup - duplicate content object pointer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContDup( IN pCont *const porg, OUT pCont *pdup);

/* get/set content object type string */
/** \fn CCLAPI RCODE ccContGetType( IN pCont *const pcont, OUT char *pszType, OUT INT32 *pnLen)
 *  \brief Function to get type of content object
 *  \param pcont - a content object pointer
 *  \param pszType - content object type
 *  \param pnLen - length of content object type
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContGetType( IN pCont *const pcont, OUT char *pszType, OUT UINT32 *pnLen);

/** \fn CCLAPI RCODE ccContSetType( IN pCont *const pcont, IN const char *pszType)
 *  \brief Function to set type of content object
 *  \param pcont - a content object pointer
 *  \param pszType - content object type
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContSetType( IN pCont *const pcont, IN const char *pszType);

/* get/set content  Disposition */
/** \fn CCLAPI RCODE ccContGetDisposition( IN pCont *const pcont, OUT char *buffer, OUT UINT32 *bufferlen)
 *  \brief Function to get Disposition/Encoding/Language of content object
 *  \param pcont - a content object pointer
  *	\param paramtype - a enum type of content parameter
 *  \param buffer - buffer for return string
 *  \param bufferlen - length of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContGetParameter( IN pCont *const pcont,IN ContParam paramtype, OUT char *buffer, OUT UINT32 *bufferlen);

/** \fn CCLAPI RCODE ccContSetDisposition( IN pCont *const pcont, IN const char *pParam)
 *  \brief Function to set Disposition/Encoding/Language of content object
 *  \param pcont - a content object pointer
 *	\param paramtype - a enum type of content parameter
 *  \param pParam - content object parameter string
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContSetParameter( IN pCont *const pcont,IN ContParam paramtype, IN const char *pParam);

/* get content object length */
/** \fn CCLAPI RCODE ccContGetLen( IN pCont *const pcont, OUT INT32 *length)
 *  \brief Function to get length of content object
 *  \param pcont - a content object pointer
 *  \param length - content object length
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContGetLen( IN pCont *const pcont, OUT UINT32 *length);

/* get/set content object body string */
/** \fn CCLAPI RCODE ccContGetBody( IN pCont *const pcont, OUT char *pszBody, OUT INT32 *pnLen)
 *  \brief Function to get body of content object
 *  \param pcont - a content object pointer
 *  \param pszBody - content object body
 *  \param pnLen - length of content object body
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContGetBody( IN pCont *const, OUT char *pszBody, OUT UINT32 *pnLen);

/** \fn CCLAPI RCODE ccContSetBody( IN pCont *const pcont, IN const char *pszBody)
 *  \brief Function to set body of content object
 *  \param pcont - a content object pointer
 *  \param pszBody - content object body
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccContSetBody( IN pCont *const pcont, IN const char *pszBody);


#ifdef  __cplusplus
}
#endif

#endif /* CC_CONT_H */
