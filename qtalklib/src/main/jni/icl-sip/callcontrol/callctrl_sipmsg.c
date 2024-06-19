/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_sipmsg.c
 *
 * $Id: callctrl_sipmsg.c,v 1.4 2008/12/29 08:54:51 tyhuang Exp $
 */
/** \file callctrl_sipmsg.c
 *  \brief Define functions to access or modify SipReq and SipRsp 
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#include <stdlib.h>
#include <stdio.h>

#include <sip/sip.h>
#include <DlgLayer/dlg_sipmsg.h>

#include "callctrl_Cont.h"
#include "callctrl_sipmsg.h"

extern char* g_ccrealm;

RCODE ccRspGetDigestAuthn(SipRsp _rsp,INT32 statuscode,SipDigestWWWAuthn* _auth);
RCODE ccReqAddDigestAuthz(SipReq _req,INT32 statuscode,SipDigestAuthz _auth);


CCLAPI 
RCODE 
ccReqAddURI(IN SipReq req,IN const char* urivalue)
{
	SipReqLine *pReqline=NULL;
	
	/* check parameter */
	if( !req ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqAddURI] NULL point to request!\n");
		return RC_ERROR;
	}
	
	if( !urivalue ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqAddURI] NULL point to uri!\n");
		return RC_ERROR;
	}

	/* request line */
	pReqline=(SipReqLine*)calloc(1,sizeof(SipReqLine));
	if( !pReqline ){
		CCWARN("[ccReqAddURI] Memory allocation error\n" );
		return RC_ERROR;
	}

	pReqline->iMethod=INVITE;
	pReqline->ptrRequestURI=strDup(urivalue);
	pReqline->ptrSipVersion=strDup("SIP/2.0");
	
	if(sipReqSetReqLine(req,pReqline) != RC_OK){
		CCERR("[urivalue] To set request line is failed!\n");
		sipReqLineFree(pReqline);
		return RC_ERROR;
	}else{
		sipReqLineFree(pReqline);
		return RC_OK;
	}
}

CCLAPI 
RCODE 
ccReqAddHeader(IN SipReq req,IN SipHdrType hdrtype,IN const char* hdrvalue)
{
	RCODE rcode;

	/* check parameter */
	if( !req ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqAddHeader] NULL point to request!\n");
		return RC_ERROR;
	}

	if( !hdrvalue ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqAddHeader] NULL point to header!\n");
		return RC_ERROR;
	}

	if(hdrtype==UnknSipHdr)
		rcode=sipReqAddUnknowHdr(req,hdrvalue);
	else{
		int buffer_len = 2*CCBUFLEN+20;
		char buffer[2*CCBUFLEN+20];

		memset(buffer,0,buffer_len*sizeof(char));
		if(hdrvalue && (strlen(hdrvalue)<(2*CCBUFLEN))){
			snprintf(buffer,buffer_len*sizeof(char),"%s:%s\r\n",sipHdrTypeToName(hdrtype),hdrvalue);
			rcode=sipReqAddHdrFromTxt(req,hdrtype,buffer);
		}else
			rcode=RC_ERROR;
	}

	return rcode;
}

CCLAPI 
RCODE 
ccRspAddHeader(IN SipRsp rsp,IN SipHdrType hdrtype,IN const char* hdrvalue)
{
	RCODE rcode;

	/* check parameter */
	if( !rsp ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccRspAddHeader] NULL point to response!\n");
		return RC_ERROR;
	}

	if( !hdrvalue ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccRspAddHeader] NULL point to header!\n");
		return RC_ERROR;
	}

	if(hdrtype==UnknSipHdr)
		rcode=sipRspAddUnknowHdr(rsp,hdrvalue);
	else{
		char buffer[2*CCBUFLEN+20];

		memset(buffer,0,sizeof buffer);
		if(hdrvalue && (strlen(hdrvalue)<(2*CCBUFLEN))){
			sprintf(buffer,"%s:%s\r\n",sipHdrTypeToName(hdrtype),hdrvalue);
			rcode=sipRspAddHdrFromTxt(rsp,hdrtype,buffer);
		}else
			rcode=RC_ERROR;
	}

	return rcode;
}

CCLAPI 
RCODE 
ccRspAddAuthen(IN SipRsp rsp,IN INT32 code)
{
	SipAuthnStruct auth;
	SipDigestWWWAuthn wwwdiget;
	RCODE rcode;
	UINT32 nc;
	
	/* check parameter */
	if (!rsp) {
		CCERR("[ccRspAddAuthen] NULL point to response!\n");
		return RC_ERROR;
	}

	if (!g_ccrealm) {
		CCPrintEx( TRACE_LEVEL_API,"[ccRspAddAuthen] NULL point to realm!\n");
		return RC_ERROR;
	}
	auth.iType=SIP_DIGEST_CHALLENGE;
	/* set www-auth */
	wwwdiget.flags_=0 ;
	/* set realm*/
	strcpy(wwwdiget.realm_,g_ccrealm);
	
	/* algorithm */
	wwwdiget.flags_ |= SIP_DWAFLAG_ALGORITHM;
	strcpy(wwwdiget.algorithm_,"MD5");

	/* nonce */
	wwwdiget.flags_ |= SIP_DWAFLAG_NONCE;
	nc=rand();
	sprintf(wwwdiget.nonce_,"%08x",nc);
#if 0
	/* opaque */
	wwwdiget.flags_ |=SIP_DWAFLAG_OPAQUE;
	strcpy(wwwdiget.opaque_,"");
#endif
	/* qop */
	wwwdiget.flags_ |= SIP_DWAFLAG_QOP;
	strcpy(wwwdiget.qop_,"auth");
#if 0
	/* stale */
	wwwdiget.flags_ |=SIP_DWAFLAG_STALE;
	wwwdiget.stale_ =FALSE;
#endif
	auth.auth_.digest_=wwwdiget;

	if (code==407) {
		rcode=sipRspAddPxyAuthn(rsp,auth);
	}else
		rcode=sipRspAddWWWAuthn(rsp,auth);
	return rcode;
}

char* GetAuthParam(SipAuthz *authz,char*name)
{
	SipParam *param = NULL;
	INT32		 num,pos;

	/* check parameter */
	if(!authz||!name)
		return NULL;

	param=authz->sipParamList;
	num=authz->numParams;
	
	for(pos=0;pos<num;pos++){
		if(!param)
			break;
		if(strCmp(param->name,name)==0)
			return param->value;
		param=param->next;
	}
	
	return NULL;
}

RCODE			
GetDigestAuthz(SipAuthz *authorization,SipDigestAuthz* auth)
{
	INT32		i = 0;
	SipParam	*piter = NULL;
	INT32		numParm;

	/* check parameter */
	if( !auth )				return RC_ERROR;
	if( !authorization )			return RC_ERROR;
	if( !authorization->sipParamList )	return RC_ERROR;
	if( !authorization->scheme )		return RC_ERROR;
	if( strICmp(authorization->scheme,"Digest")!=0 )
		return RC_ERROR;

	auth->flag_ = SIP_DAFLAG_NONE;
	piter = authorization->sipParamList;
	numParm = authorization->numParams;
	
	for( i=0; (i<numParm)&&piter; i++, piter=piter->next) {
		if( strCmp(piter->name,"username")==0 ) {
			strcpy(auth->username_,unquote(piter->value));
			continue;
		}
		if( strCmp(piter->name,"realm")==0 ) {
			strcpy(auth->realm_,unquote(piter->value));
			continue;
		}
		if( strCmp(piter->name,"nonce")==0 ) {
			strcpy(auth->nonce_,unquote(piter->value));
			continue;
		}

		/*if( strcmp(piter->name,"digest-uri")==0 ) {*/
		if( strCmp(piter->name,"uri")==0 ) {
			strcpy(auth->digest_uri_,unquote(piter->value));
			continue;
		}
		if( strCmp(piter->name,"response")==0 ) {
			strcpy(auth->response_,unquote(piter->value));
			continue;
		}
		if( strCmp(piter->name,"algorithm")==0 ) {
			strcpy(auth->algorithm_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_ALGORITHM;
			continue;
		}
		if( strCmp(piter->name,"cnonce")==0 ) {
			strcpy(auth->cnonce_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_CNONCE;
			continue;
		}
		if( strCmp(piter->name,"opaque")==0 ) {
			strcpy(auth->opaque_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_OPAQUE;
			continue;
		}
		if( strCmp(piter->name,"qop")==0 ) {
			strcpy(auth->message_qop_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_MESSAGE_QOP;
			continue;
		}
		if( strCmp(piter->name,"nc")==0 ) {
			strcpy(auth->nonce_count_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_NONCE_COUNT;
			continue;
		}
	}
	return RC_OK;
}

CCLAPI 
RCODE ccReqAddAuthz(IN SipReq req,IN SipRsp rsp,IN const char* username,IN const char* passwd)
{
		static UINT32  nc;
		INT32 statuscode,RetCode;
		char buf[512]={'\0'};
	
		SipDigestWWWAuthn wwwauth;
		SipDigestAuthz author;
		SipAuthz *reqAuth;

		INT32 ltime;
		ltime=rand();

		/* check parameter */
		if (!req || !GetReqUrl(req)) {
			CCERR("[ccReqAddAuthz] NULL point to request!\n");
			return RC_ERROR;
		}

		if (!rsp) {
			CCERR("[ccReqAddAuthz] NULL point to response!\n");
			return RC_ERROR;
		}
		
		if (!username) {
			CCERR("[ccReqAddAuthz] NULL point to username!\n");
			return RC_ERROR;
		}

		if (!passwd) {
			CCERR("[ccReqAddAuthz] NULL point to passwd!\n");
			return RC_ERROR;
		}

		/*check response status is WWW-Authenticate or Proxy-Authenticate*/	
		statuscode=GetRspStatusCode(rsp);
		if((statuscode != 401)&&(statuscode != 407))
			return RC_ERROR;
	
		RetCode=ccRspGetDigestAuthn(rsp,statuscode,&wwwauth);
		author.flag_ =0;

		if(wwwauth.flags_ & SIP_DWAFLAG_ALGORITHM){
			strcpy(author.algorithm_,wwwauth.algorithm_); 
			author.flag_ |= SIP_DAFLAG_ALGORITHM;
		}


		strcpy(author.digest_uri_,GetReqUrl(req));
		
		if(wwwauth.flags_ & SIP_DWAFLAG_QOP) {
			strcpy(author.message_qop_,"auth"); 
			author.flag_ |= SIP_DAFLAG_MESSAGE_QOP;
			sprintf(buf,"%08x",ltime);
		
			strcpy(author.cnonce_,buf);
			author.flag_ |= SIP_DAFLAG_CNONCE;

		}

		if(wwwauth.flags_ & SIP_DWAFLAG_NONCE) {
			strcpy(author.nonce_,wwwauth.nonce_);
			if(nc==0) nc++;
			sprintf(buf,"%08x",nc++);
		
			strcpy(author.nonce_count_,buf);
			author.flag_ |= SIP_DAFLAG_NONCE_COUNT;
		}
		if(wwwauth.flags_ &  SIP_DWAFLAG_OPAQUE){
			strcpy(author.opaque_,wwwauth.opaque_); 
			author.flag_ |= SIP_DAFLAG_OPAQUE;
		}

		strcpy(author.realm_,wwwauth.realm_);	
		strcpy(author.username_,username);

		if(401==statuscode)
			reqAuth=(SipAuthz *)sipReqGetHdr(req,Authorization);
		else
			reqAuth=(SipAuthz *)sipReqGetHdr(req,Proxy_Authorization);

		RetCode=sipDigestAuthzGenRspKey(&author,(char*)passwd,(char*)sipMethodTypeToName(GetReqMethod(req)),NULL);
		
		if(RetCode==RC_OK)
			RetCode=ccReqAddDigestAuthz(req,statuscode,author);

		return RC_OK;
}

CCLAPI 
RCODE 
ccReqGetAuthz(IN SipReq req,IN const char*realm,OUT SipAuthz **authz)
{
	SipAuthz *matchauthz;
	char reqrealm[128]={'\0'};

	/* check parameter */
	if (!req) {
		CCERR("[ccReqGetAuthenUser] NULL point to request!\n");
		return RC_ERROR;
	}
	if (!realm) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetAuthenUser] NULL point to realm!\n");
		return RC_ERROR;
	}
	if (!authz) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetAuthenUser] NULL point to authz!\n");
		return RC_ERROR;
	}

	/* get authorization header from sip request */
	*authz=NULL;
	matchauthz=(SipAuthz*)sipReqGetHdr(req,Authorization);
	if (!matchauthz) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetAuthenUser] authorization header is NULL!\n");
		return RC_ERROR;
	}
	
	/* match authorization by realm */
	while(matchauthz){
		memset(reqrealm,0,128);
		strcpy(reqrealm,unquote(GetAuthParam(matchauthz,"realm")));
		if(strcmp(reqrealm,realm)==0){
			*authz=matchauthz;
			break;
		}
		matchauthz=matchauthz->next;
	}	

	if(*authz==NULL)
		return RC_ERROR;

	return RC_OK;
}

CCLAPI 
RCODE 
ccReqGetAuthzUser(IN SipAuthz* authz,OUT char*buffer,IN UINT32 *len)
{
	char username[128]={'\0'};
	UINT32 namelen;

	/* check parameter */
	if (!authz) {
		CCERR("[ccReqGetAuthenUser] NULL point to authz!\n");
		return RC_ERROR;
	}

	if (!buffer || !len ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetAuthenUser] NULL point to buffer!\n");
		return RC_ERROR;
	}

	/* get username from authz paramlist */
	strcpy(username,unquote(GetAuthParam(authz,"username")));
	
	if(strlen(username)==0) {
		*len=0;
		CCWARN("[ccReqGetAuthenUser] username parameter is NULL!\n");
		return RC_ERROR;
	}

	/* copy to buffer and check length */
	namelen=strlen(username);
	if(*len==0){
		*len=namelen+1;
		return RC_ERROR;
	}
	
	if(namelen < *len ){
		strcpy(buffer,username);
		*len=namelen+1;
		return RC_OK;
	}else
		return RC_ERROR;
	

	return RC_OK;
}

CCLAPI 
RCODE 
ccReqCheckAuth(IN SipAuthz* authz,IN const char* method,IN const char* passwd)
{
	char *response;
	char buffer[33]={'\0'};
	SipDigestAuthz author;

	/* check parameter */
	if (!authz) {
		CCERR("[ccReqCheckAuth] NULL point to authz!\n");
		return RC_ERROR;
	}

	if (!method) {
		CCERR("[ccReqCheckAuth] NULL point to method!\n");
		return RC_ERROR;
	}
	
	if (!passwd) {
		CCERR("[ccReqCheckAuth] NULL point to passwd!\n");
		return RC_ERROR;
	}

	if(RC_OK!=GetDigestAuthz(authz,&author)){
		CCPrintEx( TRACE_LEVEL_API,"[ccReqCheckAuth] get authorization data is failed!\n");
		return RC_ERROR;
	}
	
	/* check response and copy to buffer */
	response=author.response_;
	if (!response) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqCheckAuth] NULL point to response!\n");
		return RC_ERROR;
	}

	memset(buffer,0,sizeof(buffer));
	strcpy(buffer,response);

	/* generate response key */
	sipDigestAuthzGenRspKey(&author,(char*)passwd,(char*)method,NULL);

	if(strcmp(&author.response_,buffer)==0)
		return RC_OK;
	else
		return RC_ERROR;

}

CCLAPI 
RCODE ccReqGetID(IN SipReq req,IN char* buffer,IN UINT32 *length)
{
	char* branchid;
	UINT32 len;
	
	/* check parameter */
	if (!req) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetID]  NULL point to req!\n");
		return RC_ERROR;
	}

	if (!buffer || !length ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetID]  NULL point to buffer or length !\n");
		return RC_ERROR;
	}

	branchid=GetReqViaBranch(req);
	if (!branchid) {
		CCERR("[ccReqGetID]  NULL point to req's id!\n");
		return RC_ERROR;
	}
	len=strlen(branchid);
	if (len>=*length) {
		*length=len+1;
		CCPrintEx( TRACE_LEVEL_API,"[ccReqGetID]  buffer is too small !\n");
		return RC_ERROR;
	}else{
		*length=len+1;
		memset(buffer,0,sizeof(buffer));
		strcpy(buffer,branchid);
		return RC_OK;
	}
}

/*
 *	internal function
 */

/** \fn RCODE ccReqModify(SipReq* req,SipReq refer)
 *  \brief Function to add some default headers to Sip request message
 *  \param req - a pointer to SipReq object pointer
 *  \param refer - reference SipReq, will copy header from it.
 *  \return - RC_OK for success; RC_ERROR for failed
 */

RCODE 
ccReqModify(SipReq* req,SipReq refer)
{

	RCODE rcode=RC_OK;
	char buffer[CCBUFLEN];
	SipReq newreq,oldreq;

	if(!refer) {
		/* no need to modify */
		return RC_OK;
	}

	if (!req ||!*req) {
		CCWARN("[ccReqModify] input request is NULL!\n");
		return RC_ERROR;
	}

	oldreq=*req;

	newreq=sipReqDup(refer);

	if(INVITE == GetReqMethod(oldreq)){
		/* add Accept for invite */
		if(sipReqGetHdr(newreq,Accept)==NULL){
			sprintf(buffer,"%s",ACCEPT_CTYPE);
			if((rcode|=ccReqAddHeader(newreq,Accept,buffer))!=RC_OK)
				CCERR("[ccReqModify] add accept error\n");
		}


		if(sipReqGetHdr(newreq,Allow)==NULL){
			sprintf(buffer,"%s",ALLOW_METHOD);
			if((rcode|=ccReqAddHeader(newreq,Allow,buffer))!=RC_OK)
				CCERR("[ccReqModify] add allow error\n");
		}

		/* supported */
		if(sipReqGetHdr(newreq,Supported)==NULL){	
			sprintf(buffer,"%s",SUPPORT_EXTEND);
			if((rcode|=ccReqAddHeader(newreq,Supported,buffer))!=RC_OK)
				CCERR("[ccReqModify] add Supported error\n");
		}
	}

	/* update request URI */
	sipReqSetReqLine(newreq,sipReqGetReqLine(oldreq));
	
	/* remove old */
	sipReqDelHdr(newreq,Via);
	sipReqDelHdr(newreq,Contact);

	/* update new header to old request */
	sipReqAddHdr(newreq,Call_ID,sipReqGetHdr(oldreq,Call_ID));
	sipReqAddHdr(newreq,Contact,sipReqGetHdr(oldreq,Contact));
	sipReqAddHdr(newreq,CSeq,sipReqGetHdr(oldreq,CSeq));
	sipReqAddHdr(newreq,From,sipReqGetHdr(oldreq,From));
	sipReqAddHdr(newreq,To,sipReqGetHdr(oldreq,To));

	/* if no maxforwd, add from oringal request */
	if(*(int*)sipReqGetHdr(newreq,Max_Forwards)<0)
		sipReqAddHdr(newreq,Max_Forwards,sipReqGetHdr(oldreq,Max_Forwards));

	/* copy refer-to to replace */
	if(refer) {
		char *replace=NULL,*uncapreplace;
		SipReferTo	*referto;
		SipURL		refertourl;
		SipURLParam	*tmpparam;
		INT32 pos,size;

		
		/* get replace */
		referto=(SipReferTo	*)sipReqGetHdr(refer,Refer_To);
		if(referto && referto->address && referto->address->addr){
			refertourl=sipURLNewFromTxt(referto->address->addr);
			if (refertourl) {
				size=sipURLGetParamSize(refertourl);
				for(pos=0;pos<size;pos++){
					tmpparam=sipURLGetParamAt(refertourl,pos);
					if (strICmp(tmpparam->pname_,"Replaces")==0) {
						replace=tmpparam->pvalue_;
						break;
					}
				}
				/* if replaces is presented,add Replaces header */
				if (replace) {
					uncapreplace=UnEscapeURI(replace);
					/* add expire for invite */
					sprintf(buffer,"%s",uncapreplace);
					if((rcode|=ccReqAddHeader(newreq,Replaces,buffer))!=RC_OK)
						CCERR("[ccReqModify] add replaces error\n");
					if (uncapreplace)
						free(uncapreplace);
				}
				sipURLFree(refertourl);
			}
		}/* end referto*/
	}

	/* remove old */
	sipReqFree(oldreq);
	/* set new req */
	*req=newreq;

	return rcode;
}

/** \fn RCODE ccRspModify(SipRsp rsp,SipRsp refer)
 *  \brief Function to add some default headers to Sip response message
 *  \param rsp - a SipRsp object pointer
 *  \return - RC_OK for success; RC_ERROR for failed
 */

RCODE ccRspModify(SipRsp rsp,SipRsp refer)
{
	INT32 statuscode;
	RCODE rcode=RC_OK;
	char buffer[CCBUFLEN];

	if (!rsp) {
		return RC_ERROR;
	}
	statuscode=GetRspStatusCode(rsp);

	
	if(refer){
		SipRequire *requirehdr=(SipRequire*)sipRspGetHdr(refer,Require);
		SipSessionExpires *se;
		SipMinSE *minse;
		
		if(requirehdr){
			if((rcode|=sipRspAddHdr(rsp,Require,requirehdr))!=RC_OK)
				CCWARN("[ccRspModify] add require is failed\n");
		}

		/* session-expires */
		se=(SipSessionExpires*)sipRspGetHdr(refer,Session_Expires);
		if (se) {
			if(RC_OK!=sipRspAddHdr(rsp,Session_Expires,(void*)se))
				CCERR("[ccReqModify] add Session_Expires error\n");
		}

		/* Min-SE */
		minse=(SipMinSE*)sipRspGetHdr(refer,Min_SE);
		if (minse) {
			if(RC_OK!=sipRspAddHdr(rsp,Min_SE,(void*)minse))
				CCERR("[ccReqModify] add Min_SE error\n");
		}

#ifdef ICL_IMS
		sipRspAddHdr(rsp,Privacy,sipRspGetHdr(refer,Privacy));
		sipRspAddHdr(rsp,P_Preferred_Identity,sipRspGetHdr(refer,P_Preferred_Identity));

		sipRspAddHdr(rsp,P_Access_Network_Info,sipRspGetHdr(refer,P_Access_Network_Info));
		
#endif
	}
	

	switch(statuscode) {
	case 200:
	case 420:
		
		if(sipRspGetHdr(refer,Allow)){
			sipRspAddHdr(rsp,Allow,sipRspGetHdr(refer,Allow));
		}else{
			sprintf(buffer,"%s\0",ALLOW_METHOD);
			if((rcode|=ccRspAddHeader(rsp,Allow,buffer))!=RC_OK)
				CCERR("[ccRspModify] add allow error\n");
		}
		
		if(sipRspGetHdr(refer,Supported)){
			sipRspAddHdr(rsp,Supported,sipRspGetHdr(refer,Supported));
		}else{
			sprintf(buffer,"%s\0",SUPPORT_EXTEND);
			if((rcode|=ccRspAddHeader(rsp,Supported,buffer))!=RC_OK)
				CCWARN("[ccRspModify] add supported is failed\n");
		}

		if(420==statuscode){
			/* add unsupported header */
			sipRspAddHdr(rsp,Unsupported,sipRspGetHdr(refer,Unsupported));
				
		}
		
		break;
	case 401:
	case 407:
		if((rcode|=ccRspAddAuthen(rsp,statuscode))!=RC_OK)
			CCERR("[ccRspModify] add www-authentication or proxy-authentication is failed\n");	
		break;
	case 405:
		sprintf(buffer,"%s\0",ALLOW_METHOD);
		if((rcode|=ccRspAddHeader(rsp,Allow,buffer))!=RC_OK)
			CCERR("[ccRspModify] add allow error\n");
		break;
	case 415:
		/* add Accept */
		sprintf(buffer,"%s\0",ACCEPT_CTYPE);
		if((rcode|=ccRspAddHeader(rsp,Accept,buffer))!=RC_OK)
			CCERR("[ccReqModify] add accept error\n");

		sprintf(buffer,"%s\0",ACCEPT_ENCODING);
		if((rcode|=ccRspAddHeader(rsp,Accept_Encoding,buffer))!=RC_OK)
			CCERR("[ccReqModify] add accept-encodig error\n");

		sprintf(buffer,"%s\0",ACCEPT_LANGUAGE);
		if((rcode|=ccRspAddHeader(rsp,Accept_Language,buffer))!=RC_OK)
			CCERR("[ccReqModify] add accept-language error\n");
		break;
	case 422:
		if(sipRspGetHdr(rsp,Min_SE)==NULL){
			sprintf(buffer,"%u\0",RFC_MIN_SE);
			if((rcode|=ccRspAddHeader(rsp,Min_SE,buffer))!=RC_OK)
				CCERR("[ccReqModify] add Min-SE error\n");
		}
		break;
	default:
		break;
	}

	return RC_OK;/* user may see log for error to add headers */
}

RCODE ReqAddBody(SipReq req,pCont msgbody)
{
	char body[SIP_MAX_CONTENTLENGTH+1]={'\0'},cttype[CCBUFLEN]={'\0'},buffer[CCBUFLEN];
	UINT32	 bodylen=SIP_MAX_CONTENTLENGTH,typelen=CCBUFLEN;
	SipBody	*tmpBody=NULL;

	if( !req ){
		CCPrintEx( TRACE_LEVEL_API,"[ReqAddBody] NULL point to request!\n");
		return RC_ERROR;
	}

	memset(body,0,sizeof(body));
	/* body raleted headers */
	if(msgbody){
		if(ccContGetLen(&msgbody,&bodylen)==RC_OK){
			if(bodylen>0){
				/* add content-type and length */
				if(ccContGetType(&msgbody,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Length:%d\r\n",bodylen);
					sipReqAddHdrFromTxt(req,Content_Length,buffer);
					sprintf(buffer,"Content-Type:%s\r\n",cttype);
					sipReqAddHdrFromTxt(req,Content_Type,buffer);
				}

#ifdef ICL_FULL_CONTENT_DISCREPTION
				/* add content-disposition */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Disposition,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Disposition:%s\r\n",cttype);
					sipReqAddHdrFromTxt(req,Content_Disposition,buffer);
				}
				/* add content-encoding */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Encoding,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Encoding:%s\r\n",cttype);
					sipReqAddHdrFromTxt(req,Content_Encoding,buffer);
				}
				/* add content-encoding */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Language,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Language:%s\r\n",cttype);
					sipReqAddHdrFromTxt(req,Content_Language,buffer);
				}
#endif /* ICL_FULL_CONTENT_DISCREPTION */

				/* add sip message body */
				bodylen=SIP_MAX_CONTENTLENGTH;
				if(ccContGetBody(&msgbody,body,&bodylen)==RC_OK){
					tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
					if( !tmpBody ){
						CCERR("[ReqAddBody] Memory allocation error\n" );
						return RC_ERROR;
					}
					tmpBody->length=bodylen;
					/*tmpBody->content=(unsigned char*)strDup(body);*/
					tmpBody->content=calloc(1,tmpBody->length+1);
					memcpy(tmpBody->content,body,bodylen);
					if(sipReqSetBody(req,tmpBody)!=RC_OK){
						CCPrintEx(TRACE_LEVEL_API,"[ReqAddBody] set request body failed!\n");
					}
					sipBodyFree(tmpBody);
				}
			}else{
				CCWARN("[ReqAddBody] get content length is failed!\n");
				sprintf(buffer,"Content-Length:0\r\n");
				sipReqAddHdrFromTxt(req,Content_Length,buffer);
			}
		}
	}else{
		sprintf(buffer,"Content-Length:0\r\n");
		sipReqAddHdrFromTxt(req,Content_Length,buffer);
	}
	
	return RC_OK;
}

RCODE RspAddBody(SipRsp rsp,pCont msgbody)
{
	char body[SIP_MAX_CONTENTLENGTH+1]={'\0'},cttype[CCBUFLEN]={'\0'},buffer[CCBUFLEN];
	UINT32 bodylen=SIP_MAX_CONTENTLENGTH,typelen=CCBUFLEN;
	SipBody	*tmpBody=NULL;
	
	if( !rsp ) {
		CCPrintEx( TRACE_LEVEL_API,"[RspAddBody] NULL point to request!\n");
		return RC_ERROR;
	}
	memset(body,0,sizeof(body));
	if(msgbody){
		if(ccContGetLen(&msgbody,&bodylen)==RC_OK){
			if(bodylen>0){
				/* add content-type */
				if(ccContGetType(&msgbody,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Length:%d\r\n",bodylen);
					sipRspAddHdrFromTxt(rsp,Content_Length,buffer);
					sprintf(buffer,"Content-Type:%s\r\n",cttype);
					sipRspAddHdrFromTxt(rsp,Content_Type,buffer);
				}

#ifdef ICL_FULL_CONTENT_DISCREPTION
				/* add content-disposition */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Disposition,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Disposition:%s\r\n",cttype);
					sipRspAddHdrFromTxt(rsp,Content_Disposition,buffer);
				}
				/* add content-encoding */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Encoding,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Encoding:%s\r\n",cttype);
					sipRspAddHdrFromTxt(rsp,Content_Encoding,buffer);
				}
				/* add content-encoding */
				typelen=CCBUFLEN;
				if(ccContGetParameter(&msgbody,Cont_Language,cttype,&typelen)==RC_OK){
					sprintf(buffer,"Content-Language:%s\r\n",cttype);
					sipRspAddHdrFromTxt(rsp,Content_Language,buffer);
				}
#endif /* ICL_FULL_CONTENT_DISCREPTION */
				
				/* add sip message body */
				bodylen=SIP_MAX_CONTENTLENGTH;
				if(ccContGetBody(&msgbody,body,&bodylen)==RC_OK){
					tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
					if( !tmpBody ){
						CCERR("[RspAddBody] Memory allocation error\n" );
						return RC_ERROR;
					}
					tmpBody->length=bodylen;
					tmpBody->content=(unsigned char*)strDup(body);
					if(sipRspSetBody(rsp,tmpBody)!=RC_OK){
						CCPrintEx(TRACE_LEVEL_API,"[RspAddBody] set response body failed!\n");
					}
					sipBodyFree(tmpBody);
				}
			}else{
				CCWARN("[RspAddBody] get content length is failed!\n");
				sprintf(buffer,"Content-Length:0\r\n");
				sipRspAddHdrFromTxt(rsp,Content_Length,buffer);
			}
		}
	}else{
		sprintf(buffer,"Content-Length:0\r\n");
		sipRspAddHdrFromTxt(rsp,Content_Length,buffer);
	}

	return RC_OK;
}

/* will alloc memory */ 
char* UnEscapeURI(const char* strUri)
{

    INT32 len,j=0,i=0 ;
    char *szBuf,*stopstring;
	char buf[3];

	if (!strUri) 
		return NULL;

	len=strlen(strUri);
	szBuf=calloc(1,sizeof(char)*(len+1));
	if (NULL==szBuf) 
		return NULL;

    while ( i < len){
        if (strUri[i] != '%')
            szBuf[j++] = strUri[i++];
        else{
			buf[0]=strUri[i+1];
			buf[1]=strUri[i+2];
			buf[2]=0;
            sprintf(szBuf+j,"%c",(int)strtol(buf,&stopstring,16));
			j++;
            i += 3;
        }

    }

     return szBuf;
}

/* will alloc memory */ 
char* EscapeURI(const char* strUri)
{
    char* szBuf;
	INT32 len,i=0 ,j=0;

	if(!strUri)
		return NULL;
	len=strlen(strUri);

	szBuf=calloc(1,(3*sizeof(char)*len)+1);
	if (NULL==szBuf) 
		return NULL;

    while ( i < len){

        switch (strUri[i])
        {
            case ';':
            case '/':
            case '?':
            case ':':
            case '@':
            case '&':
            case '=':
            case '+':
            case '$':
            case ',':
            case '<': /* avoid conflicting with SIP URI */
            case '>':
            case '{': 
            case '}':
            case '%': /* for multi-converting */
			case '\"':
			case '[':
			case ']':
                szBuf[j++] = '%';
                sprintf(szBuf+j,"%2X",strUri[i++]);
				j+=2;
                break;
            default:
                szBuf[j++] = strUri[i++];
                break;
        }
    }

    return szBuf;

}


RCODE	ccRspGetDigestAuthn(SipRsp _rsp,INT32 statuscode,SipDigestWWWAuthn* _auth)
{			    
	RCODE ret=RC_ERROR;
	SipAuthnStruct tmpAuth;

	tmpAuth.iType=SIP_DIGEST_CHALLENGE;
	switch(statuscode){
	case 401:
		ret=sipRspGetWWWAuthn(_rsp,&tmpAuth);
		break;
	case 407:
		ret=sipRspGetPxyAuthn(_rsp,&tmpAuth);
		break;
	}
	_auth->stale_=FALSE;
	if(ret != -1){
		_auth->flags_=tmpAuth.auth_.digest_.flags_;

		if(_auth->flags_ & SIP_DWAFLAG_ALGORITHM)
			strcpy(_auth->algorithm_,tmpAuth.auth_.digest_.algorithm_);
 
		if(_auth->flags_ & SIP_DWAFLAG_DOMAIN)
			strcpy(_auth->domain_,tmpAuth.auth_.digest_.domain_);
		
		if(_auth->flags_ & SIP_DWAFLAG_NONCE)
			strcpy(_auth->nonce_,tmpAuth.auth_.digest_.nonce_);

		if(_auth->flags_ & SIP_DWAFLAG_OPAQUE)
			strcpy(_auth->opaque_,tmpAuth.auth_.digest_.opaque_);
		if(_auth->flags_ & SIP_DWAFLAG_QOP)
			strcpy(_auth->qop_,tmpAuth.auth_.digest_.qop_);

		strcpy(_auth->realm_,tmpAuth.auth_.digest_.realm_);
		
		if(_auth->flags_ & SIP_DWAFLAG_STALE)
			_auth->stale_=tmpAuth.auth_.digest_.stale_;
	}
	return ret;
}

RCODE ccReqAddDigestAuthz(SipReq _req,INT32 statuscode,SipDigestAuthz _auth)
{
	RCODE ret=RC_ERROR;
	SipAuthzStruct tmpAuth;

	tmpAuth.iType=SIP_DIGEST_CHALLENGE;
	strcpy(tmpAuth.auth_.digest_.algorithm_,_auth.algorithm_);
	strcpy(tmpAuth.auth_.digest_.cnonce_,_auth.cnonce_);
	strcpy(tmpAuth.auth_.digest_.digest_uri_,_auth.digest_uri_);
	tmpAuth.auth_.digest_.flag_=_auth.flag_;
	strcpy(tmpAuth.auth_.digest_.message_qop_,_auth.message_qop_);
	strcpy(tmpAuth.auth_.digest_.nonce_,_auth.nonce_);
	strcpy(tmpAuth.auth_.digest_.nonce_count_,_auth.nonce_count_);
	strcpy(tmpAuth.auth_.digest_.opaque_,_auth.opaque_);
	strcpy(tmpAuth.auth_.digest_.realm_,_auth.realm_);
	strcpy(tmpAuth.auth_.digest_.response_,_auth.response_);
	strcpy(tmpAuth.auth_.digest_.username_,_auth.username_);
	switch(statuscode){
	case 401:
		ret=sipReqAddAuthz(_req,tmpAuth);
		break;
	case 407:
		ret=sipReqAddPxyAuthz(_req,tmpAuth);
		break;
	}
	return ret;
}
