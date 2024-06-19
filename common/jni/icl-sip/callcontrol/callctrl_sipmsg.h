/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_sipmsg.h
 *
 * $Id: callctrl_sipmsg.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

/** \file callctrl_sipmsg.h
 *  \brief Declaration functions to access or modify SipReq and SipRsp 
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#ifndef CALLCTRL_SIPMSG_H
#define CALLCTRL_SIPMSG_H

#ifdef  __cplusplus
extern "C" {
#endif


#include <sip/sip.h>
#include "callctrl_cm.h"

/* add a request URI to request */
/** \fn RCODE ccReqAddURI(IN SipReq req,IN const char* urivalue)
 *  \brief Function to add a header to SIP request message
 *  \param req - a SipReq object pointer
 *  \param urivalue - uri value string ."sip:a@itri.org.tw"
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqAddURI(IN SipReq req,IN const char* urivalue);

/* add a header to request */
/** \fn RCODE ccReqAddHeader(IN SipReq req,IN SipHdrType hdrtype,IN const char* hdrvalue)
 *  \brief Function to add a header to SIP request message
 *  \param req - a SipReq object pointer
 *  \param hdrtype - sip header type, like : Accept
 *  \param hdrvalue - header value string .if hdrtype is UnknSipHdr,it will be the whole header.
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqAddHeader(IN SipReq req,IN SipHdrType hdrtype,IN const char* hdrvalue);

/* add a header to response */
/** \fn RCODE ccRspAddHeader(IN SipRsp rsp,IN SipHdrType hdrtype,IN const char* hdrvalue)
 *  \brief Function to add a header to SIP response message
 *  \param rsp - a SipRsp object pointer
 *  \param hdrtype - sip header type, like : Accept
 *  \param hdrvalue - header value string .if hdrtype is UnknSipHdr,it will be the whole header.
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccRspAddHeader(IN SipRsp rsp,IN SipHdrType hdrtype,IN const char* hdrvalue);

/* add authenticate header ;code=401 or 407 */
/** \fn RCODE ccRspAddAuthen(IN SipRsp rsp,IN INT32 code)
 *  \brief Function to add authenticate header to SIP response message
 *  \param rsp - a SipRsp object pointer
 *	\param code - input status code for add proxy-authentication(407) or www-authentication(401)
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccRspAddAuthen(IN SipRsp rsp,IN INT32 code);

/* get authorization from request */
/** \fn RCODE ccReqAddAuthz(IN SipReq req,IN SipRsp rsp,IN const char* username,IN const char* passwd)
 *  \brief Function to add Authorization header to SIP request message
 *  \param req - a SipReq object pointer
 *	\param rsp - response carries an authentication header,it must be 401 or 407 statuscode also.
 *	\param username - username to created digest
 *	\param passwd - password to created digest
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqAddAuthz(IN SipReq req,IN SipRsp rsp,IN const char* username,IN const char* passwd);

/* get authorization from request */
/** \fn RCODE ccReqGetAuthz(IN SipReq req,IN const char*realm,OUT SipAuthz **authz)
 *  \brief Function to get Authorization header from SIP request message
 *  \param req - a SipReq object pointer
 *	\param realm - realm wanted to match
 *	\param authz - match authz header will be returned
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqGetAuthz(IN SipReq req,IN const char*realm,OUT SipAuthz **authz);

/* get username from authorization */
/** \fn RCODE ccReqGetAuthzUser(IN SipAuthz* authz,OUT char*buffer,IN UINT32 *len)
 *  \brief Function to get username from authorization data
 *  \param authz - a SipAuthz object pointer
 *  \param buffer - buffer for return valed
 *  \param len - length of username
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqGetAuthzUser(IN SipAuthz* authz,OUT char*buffer,IN UINT32 *len);

/* do compare response-digest to check authzdata */
/** \fn RCODE ccReqCheckAuth(IN SipAuthz* authz,IN const char* method,IN const char* passwd)
 *  \brief Function to check if response of authorization is right or not.
 *	\param authz - input auth data to be compared
 *  \param method - method to gerenate auth
 *  \param passwd - password of user to do auth
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqCheckAuth(IN SipAuthz* authz,IN const char* method,IN const char* passwd);

/* get Branch id of Via header */
/** \fn RCODE ccReqGetID(IN SipReq req,IN char* buffer,IN UINT32 *length)
 *  \brief Function to get branch id from top-most Via header
 *	\param req - input sip request
 *  \param buffer - output data buffer
 *  \param length - length of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccReqGetID(IN SipReq req,IN char* buffer,IN UINT32 *length);

/*
 *	internal function
 */
RCODE ccReqModify(SipReq *req,SipReq refer); 
RCODE ccRspModify(SipRsp rsp,SipRsp refer);
RCODE ReqAddBody(SipReq req,pCont msgbody);
RCODE RspAddBody(SipRsp rsp,pCont msgbody);

/* will alloc memory for return string */
char* UnEscapeURI(const char* strUri);
char* EscapeURI(const char* strUri);

#ifdef  __cplusplus
}
#endif

#endif

