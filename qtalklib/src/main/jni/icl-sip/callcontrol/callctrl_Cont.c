/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_Cont.c
 *
 * $Id: callctrl_Cont.c,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/** \file callctrl_Cont.c
 *  \brief Define ccContentObj struct access functions
 *  \author PENG, Wei-Chiang, wchpeng@itri.org.tw
 *  \date Last modify at 2004/4/11 14:19
 *  \version 0.001
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#include <stdio.h>
#include <common/cm_utl.h>
#include <sip/sip.h>
#include "callctrl_cm.h"
#include "callctrl_Cont.h"

/** \struct ccContentObj
 *  \brief ccContentObj
 */
struct ccContentObj
{
	char*	pContType;	/* example : "application/sdp" */
	UINT32	nContLen;	/* generating by strlen(pContBody) */
	char*	pContBody;	

	/* other content descriptions */
	char*	pDisposition;
	char*	pEncoding;
	char*	pLanguage;
};

CCLAPI
RCODE 
ccContNew( OUT pCont *pcont, IN const char *type, IN const char *body)
{
	pCont _this = NULL;

	/* check input parameter is NULL ? */
	if ( !pcont) {
		CCERR("[ccContNew] NULL point to  pcont!\n");
		return RC_ERROR;
	}

	if( !type ) {
		CCERR("[ccContNew] NULL point to type!\n");
		return RC_ERROR;
	}

	if( !body ) {
		CCERR("[ccContNew] NULL point to type!\n");
		return RC_ERROR;
	}

	/* new content object */
	_this = (pCont)calloc( 1, sizeof(struct ccContentObj));
	if ( !_this) {
		CCERR("[ccContNew] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	/* set content object value */
	_this->pContType = strDup( type);
	/*_this->nContLen = strlen( body);*/
	_this->nContLen = strlen(body); 
	_this->pContBody = strDup( body);

	_this->pDisposition = NULL;
	_this->pEncoding = NULL;
	_this->pLanguage = strDup("en");

	if (!_this->pContType || !_this->pContBody) {
		CCERR("[ccContNew] strDup failed!\n");
		ccContFree(&_this);
		return RC_ERROR;
	}

	/* pass out content object address */
	*pcont = _this;

	return RC_OK;
}

CCLAPI
RCODE 
ccContNewEx( OUT pCont *pcont, IN const char *type,IN unsigned int len, IN const char *body)
{
	pCont _this = NULL;
	
	/* check input parameter is NULL ? */
	if ( !pcont) {
		CCERR("[ccContNew] NULL point to  pcont!\n");
		return RC_ERROR;
	}
	
	if( !type ) {
		CCERR("[ccContNew] NULL point to type!\n");
		return RC_ERROR;
	}
	
	if( !body ) {
		CCERR("[ccContNew] NULL point to type!\n");
		return RC_ERROR;
	}
	
	/* new content object */
	_this = (pCont)calloc( 1, sizeof(struct ccContentObj));
	if ( !_this) {
		CCERR("[ccContNew] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	/* set content object value */
	_this->pContBody=calloc(1,len);
	if(NULL==_this){
		free(_this);
		return RC_ERROR;
	}
	_this->pContType = strDup( type);
	_this->nContLen = len;
	memcpy(_this->pContBody,body,len);
	
	_this->pDisposition = NULL;
	_this->pEncoding = NULL;
	_this->pLanguage = strDup("en");
	
	if (!_this->pContType || !_this->pContBody) {
		CCERR("[ccContNew] strDup failed!\n");
		ccContFree(&_this);
		return RC_ERROR;
	}
	
	/* pass out content object address */
	*pcont = _this;
	
	return RC_OK;
}

CCLAPI 
RCODE 
ccContNewFromReq( OUT pCont *pcont,IN const SipReq req)
{

	pCont _this = NULL;

	SipBody	*pbody=NULL;
	SipContType *type=NULL;
	SipParam *tmpparam;
	char fulltype[CCBUFLEN]={'\0'};
	INT32 pos=0,i;
	SipContentDisposition *cdisp;
	SipContEncoding *cenc;
	SipContentLanguage *clang;

	/* check parameter */
	if(!pcont){
		CCPrintEx( TRACE_LEVEL_API, "[ccContNewFromReq] NULL point to  pcont!\n");
		return RC_ERROR;
	}

	if( !req ) {
		CCWARN("[ccContNewFromReq] NULL point to  request!\n");
		return RC_ERROR;
	}
	
	/* get content-type */
	type=sipReqGetHdr(req,Content_Type);
	if(type == NULL){
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromReq] NULL point to  content-type!\n");
		return RC_ERROR;
	}
	if(type->type==NULL){
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromReq] NULL point to  content-type!\n");
		return RC_ERROR;
	}

	pbody=sipReqGetBody(req);
	if (!pbody) {
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromReq] NULL point to  request body!\n");
		return RC_ERROR;
	}
	
	_this = (pCont)calloc( 1, sizeof(struct ccContentObj));
	if ( !_this) {
		CCWARN("[ccContNewFromReq] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	/* set new pcont*/
	pos=snprintf(fulltype, CCBUFLEN*sizeof(char),"%s",type->type);
	if(type->subtype)
		pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"/%s",type->subtype);
	
	/* add parameter of content-type */
	tmpparam=type->sipParamList;
	while (tmpparam) {
		if (tmpparam->name) 
			pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";%s",tmpparam->name);
		else
			break;
		if (tmpparam->value)
			pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"=%s",tmpparam->value);
		tmpparam=tmpparam->next;
	}

	_this->pContType = strDup(fulltype);
	/*_this->nContLen = pbody->length;*/
	if(pbody->content){
		/*
		_this->nContLen = strlen(pbody->content);
		_this->pContBody = strDup((const char*)pbody->content);
		*/
		_this->nContLen = pbody->length;
		_this->pContBody = calloc(1,_this->nContLen+1);
		memcpy(_this->pContBody,pbody->content,_this->nContLen);
	}
	
	if (!_this->pContType ) {
		CCERR("[ccContNewFromReq] strDup failed!\n");
		ccContFree(&_this);
		return RC_ERROR;
	}

	/* get content-disposition */
	cdisp=(SipContentDisposition*)sipReqGetHdr(req,Content_Disposition);
	if (cdisp && cdisp->disptype) {
		pos=snprintf(fulltype, CCBUFLEN*sizeof(char),"%s",cdisp->disptype);
		tmpparam=cdisp->paramList;
		while (tmpparam) {
			if (tmpparam->name) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";%s",tmpparam->name);
			else
				break;
			if (tmpparam->value)
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"=%s",tmpparam->value);
			tmpparam=tmpparam->next;
		}
		_this->pDisposition=strDup(fulltype);
	}

	/* get content-encoding */
	cenc=(SipContEncoding*)sipReqGetHdr(req,Content_Encoding);
	if (cenc && cenc->sipCodingList ) {
		SipCoding *_coding;
		_coding=cenc->sipCodingList;
		pos=0;
		for (i=0;i<cenc->numCodings;i++){
			if (_coding->coding) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"%s",_coding->coding);
			else
				break;
			if (_coding->qvalue) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";q=%s",_coding->qvalue);
			_coding=_coding->next;
			if (_coding) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),",");
			else
				break;
		}
		_this->pEncoding=strDup(fulltype);
	}
	
	/* get content-Language */
	clang=(SipContentLanguage*)sipReqGetHdr(req,Content_Language);
	if (clang && clang->langList ) {
		SipStr *langtag;
		langtag=clang->langList;
		pos=0;
		for (i=0;i<clang->numLang;i++){
			if (langtag->str) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"%s",langtag->str);
			else
				break;
			langtag=langtag->next;
			if (langtag) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),",");
			else
				break;
		}
		_this->pLanguage=strDup(fulltype);
	}

	*pcont=_this;
	return RC_OK;
}

CCLAPI 
RCODE 
ccContNewFromRsp( OUT pCont *pcont,IN const SipRsp rsp)
{
	pCont _this = NULL;

	SipBody	*pbody=NULL;
	SipContType *type=NULL;
	SipParam *tmpparam;
	char fulltype[CCBUFLEN]={'\0'};
	INT32 pos=0,i;
	SipContentDisposition *cdisp;
	SipContEncoding *cenc;
	SipContentLanguage *clang;

	/* check parameter */
	if(!pcont){
		CCPrintEx( TRACE_LEVEL_API, "[ccContNewFromRsp] NULL point to  pcont!\n");
		return RC_ERROR;
	}

	if( !rsp ) {
		CCWARN("[ccContNewFromRsp] NULL point to  response!\n");
		return RC_ERROR;
	}
	
	/* get content-type */
	type=sipRspGetHdr(rsp,Content_Type);
	if(type == NULL){
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromRsp] NULL point to  content-type!\n");
		return RC_ERROR;
	}
	if(type->type==NULL){
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromRsp] NULL point to  content-type!\n");
		return RC_ERROR;
	}

	pbody=sipRspGetBody(rsp);
	if (!pbody) {
		CCPrintEx( TRACE_LEVEL_API,"[ccContNewFromRsp] NULL point to  request body!\n");
		return RC_ERROR;
	}
	
	/* set new pcont*/
	_this = (pCont)calloc( 1, sizeof(struct ccContentObj));
	if ( !_this) {
		CCWARN("[ccContNewFromRsp] memory allocation failed!\n");
		return RC_ERROR;
	}

	pos=snprintf(fulltype, CCBUFLEN*sizeof(char),"%s",type->type);
	if(type->subtype)
		snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"/%s",type->subtype);
	
	/* add parameter of content-type */
	tmpparam=type->sipParamList;
	while (tmpparam) {
		if (tmpparam->name) 
			pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";%s",tmpparam->name);
		else
			break;
		if (tmpparam->value)
			pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"=%s",tmpparam->value);
		tmpparam=tmpparam->next;
	}
	_this->pContType = strDup(fulltype);
	
	if(pbody->content){
		_this->nContLen = strlen(pbody->content);
		_this->pContBody = strDup((const char*)pbody->content);
	}
	if (!_this->pContType) {
		CCERR("[ccContNewFromRsp] strDup failed!\n");
		ccContFree(&_this);
		return RC_ERROR;
	}

	/* get content-disposition */
	cdisp=(SipContentDisposition*)sipRspGetHdr(rsp,Content_Disposition);
	if (cdisp && cdisp->disptype) {
		pos=snprintf(fulltype, CCBUFLEN*sizeof(char),"%s",cdisp->disptype);
		tmpparam=cdisp->paramList;
		while (tmpparam) {
			if (tmpparam->name) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";%s",tmpparam->name);
			else
				break;
			if (tmpparam->value)
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"=%s",tmpparam->value);
			tmpparam=tmpparam->next;
		}
		_this->pDisposition=strDup(fulltype);
	}

	/* get content-encoding */
	cenc=(SipContEncoding*)sipRspGetHdr(rsp,Content_Encoding);
	if (cenc && cenc->sipCodingList ) {
		SipCoding *_coding;
		_coding=cenc->sipCodingList;
		pos=0;
		for (i=0;i<cenc->numCodings;i++){
			if (_coding->coding) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"%s",_coding->coding);
			else
				break;
			if (_coding->qvalue) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),";q=%s",_coding->qvalue);
			_coding=_coding->next;
			if (_coding) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),",");
			else
				break;
		}
		_this->pEncoding=strDup(fulltype);
	}
	
	/* get content-Language */
	clang=(SipContentLanguage*)sipRspGetHdr(rsp,Content_Language);
	if (clang && clang->langList ) {
		SipStr *langtag;
		langtag=clang->langList;
		pos=0;
		for (i=0;i<clang->numLang;i++){
			if (langtag->str) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),"%s",langtag->str);
			else
				break;
			langtag=langtag->next;
			if (langtag) 
				pos+=snprintf(fulltype+pos, (CCBUFLEN-pos)*sizeof(char),",");
			else
				break;
		}
		_this->pLanguage=strDup(fulltype);
	}
	*pcont=_this;
	return RC_OK;
}

/* get sipfrag from response */
CCLAPI 
RCODE 
ccRspNewSipFrag(OUT pCont *pcont,IN const SipRsp rsp)
{
	SipRspStatusLine *statusline;
	char buffer[2*CCBUFLEN];
	UINT32 pos=0;
	/* check parameter */
	if (!rsp) {
		CCERR("[ccRspNewSipFrag] NULL point to response!\n");
		return RC_ERROR;
	}

	if (!pcont) {
		CCERR( "[ccRspNewSipFrag] NULL point to content!\n");
		return RC_ERROR;
	}

	statusline=sipRspGetStatusLine(rsp);
	if (!statusline) {
		CCPrintEx( TRACE_LEVEL_API,"[ccRspNewSipFrag] NULL point to status line!\n");
		return RC_ERROR;
	}

	memset(buffer,0,2*CCBUFLEN*sizeof(char));
	if (statusline->ptrVersion) 
		pos=snprintf(buffer, sizeof(buffer),"%s",statusline->ptrVersion);

	if (statusline->ptrStatusCode) 
		pos+=snprintf(buffer+pos, sizeof(buffer+pos)," %s",statusline->ptrStatusCode);

	if (statusline->ptrReason) 
		pos+=snprintf(buffer+pos, sizeof(buffer+pos)," %s\r\n",statusline->ptrReason);

	if (RC_OK!=ccContNew(pcont,"message/sipfrag;version=2.0",buffer)) {
		CCERR( "[ccRspNewSipFrag] new content is failed!\n");
		return RC_ERROR;
	}
	
	return RC_OK;
}

CCLAPI
RCODE 
ccContFree( IN pCont *pcont)
{
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContFree] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContFree] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}

	if ( _this->pContType)
		free( _this->pContType);

	if ( _this->pContBody)
		free( _this->pContBody);

	if (_this->pDisposition) {
		free(_this->pDisposition);
	}

	if (_this->pEncoding) {
		free(_this->pEncoding);
	}

	if (_this->pLanguage) {
		free(_this->pLanguage);
	}

	memset(_this,0,sizeof(Cont));
	free( _this);
	*pcont=NULL;

	return RC_OK;
}

CCLAPI
RCODE ccContDup( IN pCont *const porg, OUT pCont *pdup)
{
	pCont _org ;
	pCont _dup = NULL;

	if (!porg) {
		CCERR("[ccContDup] invalid argument content object pointer is NULL!\n");
		return RC_ERROR;
	}
	if (!pdup) {
		CCERR("[ccContDup] invalid argument dup content object pointer is NULL!\n");
		return RC_ERROR;
	}
	_org = *porg;
	/* check original content object not NULL */
	if ( !_org) {
		CCERR("[ccContDup] invalid argument content object pointer is NULL!\n");
		return RC_ERROR;
	}

	/* new duplicate content object */
	_dup=(pCont)calloc( 1, sizeof(struct ccContentObj));
	if ( !_dup ) {
		CCERR("[ccContDup] new duplication content object failed!\n");
		return RC_ERROR;
	}

	/* copy object value from original content object */
	_dup->pContType = strDup( _org->pContType);
	_dup->nContLen = _org->nContLen;
	_dup->pContBody = strDup( _org->pContBody);

	_dup->pDisposition = strDup( _org->pDisposition );
	_dup->pEncoding = strDup(_org->pEncoding);
	_dup->pLanguage = strDup(_org->pLanguage);
	
	/*
	if (!_dup->pContType || !_dup->pContBody) {
		CCERR("[ccContDup] strDup failed!\n");
		ccContFree(&_dup);
		return RC_ERROR;
	}
	*/

	/* pass out duplication content object address */
	*pdup = _dup;

	return RC_OK;
}

CCLAPI
RCODE ccContGetType( IN pCont *const pcont, OUT char *pszType, OUT UINT32 *pnLen)
{
	UINT32 nLen = 0;
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContGetType] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContGetType] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	if (!pszType || !pnLen){
		CCWARN("[ccContGetType] input buffer is NULL!\n");
		return RC_ERROR;
	}
	if (!_this->pContType) {
		CCWARN("[ccContGetType] content type is NULL!\n");
		return RC_ERROR;
	}

	if (*pnLen==0) {
		*pnLen=strlen( _this->pContType)+1;
		return RC_ERROR;
	}
	nLen = strlen( _this->pContType);
	if ( nLen >= *pnLen) {
		/* total len=strlen()+1 "\0" */
		*pnLen = nLen+1;
		return RC_ERROR;
	}

	*pnLen = nLen+1;
	strcpy( pszType, _this->pContType);

	return RC_OK;
}

CCLAPI
RCODE 
ccContSetType( IN pCont *const pcont, IN const char *pszType)
{
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContSetType] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContSetType] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	if ( !pszType) {
		CCWARN("[ccContSetType] input pszType is NULL!\n");
		return RC_ERROR;
	}

	if ( _this->pContType)
		free(_this->pContType);

	_this->pContType = strDup( pszType);

	if (!_this->pContType) {
		CCERR("[ccContSetType] set contype is failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

CCLAPI 
RCODE
ccContGetParameter( IN pCont *const pcont,IN ContParam paramtype, OUT char *buffer, OUT UINT32 *bufferlen)
{
	UINT32 nLen = 0;
	pCont _this;
	char* pValue=NULL;

	if (!pcont) {
		CCWARN("[ccContGetParameter] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContGetParameter] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	if (!buffer || !bufferlen){
		CCWARN("[ccContGetParameter] input buffer is NULL!\n");
		return RC_ERROR;
	}

	if (paramtype==Cont_Disposition) 
		pValue=_this->pDisposition;
	else if(paramtype==Cont_Encoding)
		pValue=_this->pEncoding;
	else if (paramtype==Cont_Language) 
		pValue=_this->pLanguage;
	else{
		CCWARN("[ccContGetParameter] input parameter is wrong!\n");
		return RC_ERROR;
	}

	memset(buffer,0,strlen(buffer));
	
	if (!pValue) {
		*bufferlen=0;
		return RC_ERROR;
	}

	nLen = strlen( pValue);
	if ((*bufferlen<1)||( nLen >= *bufferlen)) {
		/* total len=strlen()+1 "\0" */
		*bufferlen = nLen+1;
		return RC_ERROR;
	}

	*bufferlen = nLen+1;
	strcpy( buffer, pValue);

	return RC_OK;
}

CCLAPI
RCODE 
ccContSetParameter( IN pCont *const pcont,IN ContParam paramtype, IN const char *pParam)
{
	pCont _this;
	char *pValue=NULL;

	if (!pcont) {
		CCWARN("[ccContSetParameter] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContSetParameter] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}

	if (paramtype==Cont_Disposition) 
		pValue=_this->pDisposition;
	else if(paramtype==Cont_Encoding)
		pValue=_this->pEncoding;
	else if (paramtype==Cont_Language) 
		pValue=_this->pLanguage;
	else{
		CCWARN("[ccContSetParameter] input parameter is wrong!\n");
		return RC_ERROR;
	}

	if ( pValue)
		free(pValue);

	pValue = strDup( pParam);

	if (paramtype==Cont_Disposition) 
		_this->pDisposition=pValue;
	else if(paramtype==Cont_Encoding)
		_this->pEncoding=pValue;
	else
		_this->pLanguage=pValue;

	return RC_OK;
}


CCLAPI
RCODE 
ccContGetBody( IN pCont *const pcont, OUT char *pszBody, OUT UINT32 *pnLen)
{
	UINT32 nLen = 0;
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContGetBody] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContGetBody] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	if (!pszBody || !pnLen){
		CCWARN("[ccContGetBody] input buffer is NULL!\n");
		return RC_ERROR;
	}

	if (!_this->pContBody) {
		CCWARN("[ccContGetBody] content body is NULL!\n");
		return RC_ERROR;
	}

	/*nLen=strlen(_this->pContBody);*/
	nLen=_this->nContLen;
	if (*pnLen==0) {
		*pnLen=nLen;
		return RC_ERROR;
	}

	/*
	if ( nLen >= *pnLen) {
		*pnLen = nLen+1;
		return RC_ERROR;
	}
	*/
	if( nLen > *pnLen ) {
		*pnLen = nLen;
		return RC_ERROR;
	}

	memset(pszBody,0,_this->nContLen);

	/* *pnLen = nLen+1;*/
	*pnLen=nLen;

	memcpy( pszBody, _this->pContBody,_this->nContLen);

	return RC_OK;
}

CCLAPI
RCODE 
ccContSetBody( IN pCont *const pcont, IN const char *pszBody)
{
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContSetBody] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContSetBody] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}

	if ( !pszBody) {
		CCWARN("[ccContSetBody] input pszBody ptr is NULL!\n");
		return RC_ERROR;
	}
	/* free original content body */
	if ( _this->pContBody) {
		free( _this->pContBody);
		_this->nContLen = 0;
	}

	_this->pContBody = strDup( pszBody);
	_this->nContLen = strlen( pszBody);

	if (!_this->pContBody) {
		_this->nContLen=0;
		CCERR("[ccContSetType] set contbody is failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

CCLAPI
RCODE 
ccContGetLen( IN pCont *const pcont, OUT UINT32 *length)
{
	pCont _this;

	if (!pcont) {
		CCWARN("[ccContGetLen] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	_this = *pcont;
	if ( !_this) {
		CCWARN("[ccContGetLen] input pcont ptr is NULL!\n");
		return RC_ERROR;
	}
	if (!length) {
		CCWARN("[ccContGetLen] input length ptr is NULL!\n");
		return RC_ERROR;
	}

	*length = _this->nContLen;

	return RC_OK;
}
