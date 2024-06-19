/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 *
 * $Id: dlg_sipmsg.h,v 1.7 2009/03/04 05:13:50 tyhuang Exp $
 */
#ifndef DLG_SIPMSG_H
#define DLG_SIPMSG_H

#ifdef  __cplusplus
extern "C" {
#endif
	
#include "dlg_cm.h"
#include <sip/sip.h>
#include <adt/dx_lst.h>

/*
 * internal function 
 */
/* check sip message for callid,to,from,cseq */
/* RCODE sipDlgReqChk(IN SipReq); */
char * sipDlgReqChk(IN SipReq);

RCODE sipDlgRspChk(IN SipRsp);

/* get to url from request */
char*	GetReqToUrl(IN SipReq);
/* get from url from request */
char*	GetReqFromUrl(IN SipReq);
/* get to tag from request */
char*	GetReqToTag(IN SipReq);
/* get from tag from request */
char*	GetReqFromTag(IN SipReq);
/* get contact address from request */
char*	GetReqContact(IN SipReq);
/* get request url from request */
char*	GetReqUrl(IN SipReq);
/* get request From display name */
char*	GetReqFromDisplayname(IN SipReq);
/* get request To display name */
char*	GetReqToDisplayname(IN SipReq);
/* get refer-to url */
char*	GetReqReferToUrl(IN SipReq);

/* get to url from response */
char*	GetRspToUrl(IN SipRsp);
/* get from url from response */
char*	GetRspFromUrl(IN SipRsp);
/* set to tag to response */
RCODE	AddRspToTag(IN SipRsp,IN const char* tag);
/* get to tag from response */
char*	GetRspToTag(IN SipRsp);
/* get from tag from response */
char*	GetRspFromTag(IN SipRsp);
/* get status code from response */
int		GetRspStatusCode(IN SipRsp);
/* get contact address from response */
char*	GetRspContact(IN SipRsp);
/* get displayname from to header of siprsp */
char*	GetRspToDisplayname(IN SipRsp);

/* get expire parameter from contact header field */
INT32 GetRspContactExpires(IN SipRsp);

/* get expire value from expires header field */
INT32 GetReqExpires(IN SipReq);
INT32 GetRspExpires(IN SipRsp);

/* get realm from response */
char* NewRealmFromRsp(IN SipRsp);

/* get Method from sip message */
SipMethodType GetReqMethod(IN SipReq);
SipMethodType GetRspMethod(IN SipRsp);

/* get reason phase from response */
char* GetRspReason(IN SipRsp);
/* get event type from request */
char* GetReqEvent(IN SipReq);
char* GetReqSubState(IN SipReq);

/* get branch id from Via header */
char* GetReqViaBranch(IN SipReq);
char* GetRspViaBranch(IN SipRsp);

char* GetRspViaSentBy(IN SipRsp);

/* not thread safe function */
char* PrintRequestline(IN SipReq);
char* PrintStatusline(IN SipRsp);

/* get all record-route headers from sip message */
/* it will alloc a dxlst */
DxLst	GetReqRecRoute(IN SipReq);
DxLst	GetRspRecRoute(IN SipRsp);

unsigned int GetReqCSeq(IN SipReq);
int GetReqEventID(IN SipReq);

/* NewToSIPURL */
/* remove port from original sip url
 * from sip:xxx@dddd to sip:xxx
 */
char* NewNOPortURL(const char* url);

/* tag generator */
/* this function is not thread safe */
UINT32 GetRand();
char* taggen(void);


/* check options */
BOOL Be100rel(IN SipRsp rsp);
BOOL CheckOptionTag(IN SipReq req);

RCODE AddRspRSeq(IN SipRsp,IN UINT32);

BOOL BeRequire100rel(IN SipReq req);
BOOL BeSupport100rel(IN SipReq req);

BOOL BeSupportOption(IN SipReq req,IN const char *tag);
BOOL BeRequireOption(IN SipReq req,IN const char *tag);

BOOL BeMatchTag(const char* s1,const char* s2);

BOOL BeSIPSURL(const char* url);

BOOL BeReplacesEarlyonly(IN SipReq req);

#ifdef  __cplusplus
}
#endif

#endif

