/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_media.c
 *
 * $Id: sess_media.c,v 1.4 2008/12/05 02:53:38 tyhuang Exp $
 */

#include <stdio.h>

#include <adt/dx_lst.h>
#include <adt/dx_str.h>
#include <common/cm_utl.h>
#include <sdp/sdp_mses.h>

#include "sess_media.h"
#include "sess_payload.h"


struct sessmediaObj{
	MediaType type;
	char *address;
	UINT32 port;
	UINT16 numofport;
	char* transport;
	UINT32 ptime;
	StatusAttr status;
	DxLst	rtpmaplist;
	DxLst	fmtplist;
	DxLst	bandwidthlist;
	DxLst	attribelist;

	/* for sRTP */
	DxLst	cryptolist;
};

struct BWobj{
	char*	modifier;
	UINT32	value;
};

struct Attobj{
	char*	name;
	char*	value;
};

typedef struct BWobj BWInfo;
typedef struct BWobj* pBWInfo;

typedef struct Attobj MediaAtt;
typedef struct Attobj* pMediaAtt;

DxLst SessMediaGetlist(IN pSessMedia media,IN RTPParamType rtype);
pMediaPayload GetMatchPayload(DxLst paramlist,const char* payloadtype,UINT32 *listpos);
void bwInfoFree(pBWInfo b);
void mediaattFree(pMediaAtt m);

CCLAPI 
RCODE SessMediaNew(OUT pSessMedia *pMedia,
				   IN MediaType type,
				   IN const char* address,
				   IN UINT32 port,
				   IN const	char* transport,
				   IN UINT32 ptime,
				   IN RTPParamType rtype,
				   IN pMediaPayload rtpmap)
{
	pSessMedia media;

	/* check parameter */
	if ( ! pMedia ) {
		SESSWARN ("[SessMediaNew] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if( ( type < MEDIA_AUDIO )||(type > MEDIA_MESSAGE) ) {
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaNew] Invalid argument : media type is unknow!\n" );
		return RC_ERROR;
	}
	if ((port<1024)||(port>65535)) {
		SESSWARN ("[SessMediaSetPort] Invalid argument : port should in the range 1024 to 65535 inclusive\n" );
		return RC_ERROR;
	}
	if (!rtpmap) {
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaNew] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}

	/* memory alloc */
	media=(pSessMedia)calloc(1,sizeof(SessMedia));
	if (!media) {
		SESSERR("[SessMediaNew] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* set value of object */
	if (RC_OK!=SessMediaAddRTPParam(media,rtype,rtpmap)) {
		SESSERR("[SessMediaNew] Add rtpmap is failed\n" );
		free(media);
		return RC_ERROR;
	}
	
	media->type=type;
	if (address) media->address=strDup(address);

	media->port=port;
	media->numofport=1;

	if (transport) media->transport=strDup(transport);
	else
		media->transport=strDup("RTP/AVP");
	
	media->ptime=ptime;
	media->status=SENDRECV;

	/* set value for return */
	*pMedia=media;
	return RC_OK;
}

CCLAPI 
RCODE SessMediaFree(IN pSessMedia *pMedia)
{
	pSessMedia media=NULL;
	pMediaPayload pl;
	INT32 pos,size;
	
	/* check parameter */
	if ( !pMedia ) {
		SESSWARN ("[SessMediaFree] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	media=*pMedia;
	if ( !media ) {
		SESSWARN ("[SessMediaFree] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (media->address) 
		free(media->address);
	if (media->transport) 
		free(media->transport);
	if (media->rtpmaplist) {
		size=dxLstGetSize(media->rtpmaplist);
		for(pos=0;pos<size;pos++){
			pl=dxLstGetAt(media->rtpmaplist,0);
			MediaPayloadFree(&pl);
		}
		dxLstFree(media->rtpmaplist,NULL);
	}
	if (media->fmtplist) {
		size=dxLstGetSize(media->fmtplist);
		for(pos=0;pos<size;pos++){
			pl=dxLstGetAt(media->fmtplist,0);
			MediaPayloadFree(&pl);
		}
		dxLstFree(media->fmtplist,NULL);
	}

	if(media->bandwidthlist)
		dxLstFree(media->bandwidthlist,(void (*) (void*)) bwInfoFree);

	if(media->attribelist)
		dxLstFree(media->attribelist,(void (*) (void*)) mediaattFree);

	if(media->cryptolist)
		dxLstFree(media->cryptolist,NULL);

	memset(media,0,sizeof(SessMedia));
	free(media);
	*pMedia=NULL;
	return RC_OK;
}

CCLAPI 
RCODE SessMediaSetStatus(IN pSessMedia media,IN StatusAttr newstatus)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaSetStatus] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	
	if((newstatus<SENDRECV)||(newstatus>INACTIVE)) {
		SESSWARN ("[SessMediaSetStatus] Invalid argument : wroing value to StatusAttr\n" );
		return RC_ERROR;
	}
	media->status=newstatus;
	return RC_OK;
}

CCLAPI 
RCODE SessMediaGetStatus(IN pSessMedia media,OUT StatusAttr *status)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetStatus] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!status) {
		SESSWARN ("[SessMediaGetStatus] Invalid argument : Null pointer to status\n" );
		return RC_ERROR;
	}

	*status=media->status;
	return RC_OK;
}

CCLAPI 
RCODE SessMediaGetType(IN pSessMedia media,OUT MediaType *type)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetType] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!type) {
		SESSWARN ("[SessMediaGetStatus] Invalid argument : Null pointer to type\n" );
		return RC_ERROR;
	}
	*type=media->type;
	return RC_OK;
}

CCLAPI 
RCODE SessMediaSetPort(IN pSessMedia media,OUT UINT32 port)
{
	if ((port<1024)||(port>65535)) {
		SESSWARN ("[SessMediaSetPort] Invalid argument : port should in the range 1024 to 65535 inclusive\n" );
		return RC_ERROR;
	}
	return SessMediaSetParam(media,PORT,port);
}

CCLAPI 
RCODE SessMediaGetPort(IN pSessMedia media,OUT UINT32 *port)
{
	return SessMediaGetParam(media,PORT,port);
}

/* set media transport type */
CCLAPI 
RCODE SessMediaSetTransport(IN pSessMedia media,OUT const char *transport)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaSetTransport] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if ( !transport ) {
		SESSWARN ("[SessMediaSetTransport] Invalid argument : Null pointer to transport\n" );
		return RC_ERROR;
	}

	if(media->transport)
		free(media->transport);
	media->transport=strDup(transport);
	return RC_OK;	

}
/* get media transport type */
CCLAPI 
RCODE SessMediaGetTransport(IN pSessMedia media,OUT char* buffer,IN OUT INT32* length)
{
	/* check parameter */
	INT32 len;
	if ( ! media ) {
		SESSWARN("[SessMediaGetTransport] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (!media->transport) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetTransport] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	if( !length||!buffer){
		SESSWARN ("[SessMediaGetTransport] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	len=strlen(media->transport);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetTransport] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,media->transport);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessMediaGetTransport] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}	
}

/* set media transport type */
CCLAPI 
RCODE SessMediaSetAddress(IN pSessMedia media,OUT const char *address)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaSetAddress] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}

	if(media->address)
		free(media->address);
	if (address) 
		media->address=strDup(address);
	else
		media->address=NULL;

	return RC_OK;	

}
/* get media transport type */
CCLAPI 
RCODE SessMediaGetAddress(IN pSessMedia media,OUT char* buffer,IN OUT INT32* length)
{
	/* check parameter */
	INT32 len;
	if ( ! media ) {
		SESSWARN("[SessMediaGetAddress] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (!media->address) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetAddress] Null pointer to address\n" );
		return RC_ERROR;
	}

	if( !length||!buffer){
		SESSWARN ("[SessMediaGetAddress] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	len=strlen(media->address);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetAddress] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,media->address);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessMediaGetAddress] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}	
}

CCLAPI 
RCODE SessMediaSetParam(IN pSessMedia media,IN MediaParamType mptype,IN UINT32 value)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaSetParam] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if ((mptype==PORT)&&(value==0)) {
		SESSWARN ("[SessMediaSetParam] Invalid argument : port must be larger than zero\n" );
		return RC_ERROR;
	}
	
	if (PORT==mptype) 
		media->port=value;
	else if (PTIME==mptype) 
		media->ptime=value;
	else{
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaSetParam] Invalid argument : wrong MediaParamType\n" );
		return RC_ERROR;
	}
	return RC_OK;
}

CCLAPI 
RCODE SessMediaGetParam(IN pSessMedia media,IN MediaParamType mptype,OUT UINT32 *value)
{
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetParam] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if ( !value ) {
		SESSWARN ("[SessMediaGetParam] Invalid argument : Null pointer to value\n" );
		return RC_ERROR;
	}

	if (PORT==mptype) 
		*value=media->port;
	else if (PTIME==mptype) 
		*value=media->ptime;
	else{
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetParam] Invalid argument : wrong MediaParamType\n" );
		return RC_ERROR;
	}
	return RC_OK;
}

CCLAPI 
RCODE SessMediaAddRTPParam(IN pSessMedia media,IN RTPParamType rtype,IN pMediaPayload pl)
{
	pMediaPayload tmppl;
	DxLst mlist;
	UINT32 pos;
	char payloadtype[64];
	INT32 len=63;

	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaAddRTPParam] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!pl) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (RC_OK!=MediaPayloadGetType(pl,payloadtype,&len)){
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] Invalid argument : Null pointer to payload type\n" );
		return RC_ERROR;
	}
	mlist=SessMediaGetlist(media,rtype);
	if (!mlist) {
		if(NULL==(mlist=dxLstNew(DX_LST_POINTER))){
			SESSERR("[SessMediaAddRTPParam] Memory allocation error\n" );
			return RC_ERROR;
		}
		if (RTPMAP==rtype)
			media->rtpmaplist=mlist;
		else
			media->fmtplist=mlist;
	}

	/* check for fmtp */
	if ( (FMTP==rtype)&&!GetMatchPayload(media->rtpmaplist,payloadtype,&pos)) {
		/*
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] format type doesnt  in rtpmap list\n" );
		return RC_ERROR;
		*/
		/*
		 * add a payload for rtp
		 */
		pMediaPayload rtpmappl;
		if (RC_OK!=MediaPayloadNew(&rtpmappl,payloadtype,NULL)) {
			SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] format type does not  in rtpmap list\n" );
			return RC_ERROR;
		}
		if (!media->rtpmaplist) 
			media->rtpmaplist=dxLstNew(DX_LST_POINTER);
		if(RC_OK!=dxLstPutTail(media->rtpmaplist,(void*)rtpmappl)){
			MediaPayloadFree(&rtpmappl);
			SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] format type does not  in rtpmap list\n" );
			return RC_ERROR;
		}
	}

	if (GetMatchPayload(mlist,payloadtype,&pos)) {
		/*
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] payload type exists in list\n" );
		return RC_ERROR;
		*/
		/* free old payload */
		tmppl=dxLstGetAt(mlist,pos);
		MediaPayloadFree(&tmppl);
	}
	
	return dxLstPutTail(mlist,(void*)pl);
}

CCLAPI 
RCODE SessMediaGetRTPParam(IN pSessMedia media,IN RTPParamType rtype,IN const char* payloadtype,OUT pMediaPayload *pPl)
{
	DxLst mlist;
	UINT32 pos;
	
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaAddRTPParam] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!pPl) {
		SESSWARN ("[SessMediaAddRTPParam] Invalid argument : Null pointer to MediaPayload\n" );
		return RC_ERROR;
	}
	if (!payloadtype) {
		SESSWARN ("[SessMediaAddRTPParam] Invalid argument : Null pointer to payload type\n" );
		return RC_ERROR;
	}

	mlist=SessMediaGetlist(media,rtype);
	if (!mlist) {
		SESSWARN ("[SessMediaAddRTPParam] Invalid argument : Null pointer to list\n" );
		return RC_ERROR;
	}
	*pPl=GetMatchPayload(mlist,payloadtype,&pos);
	if (NULL==*pPl) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] rtp paramter is not found\n" );
		return RC_ERROR;
	}
	
	return RC_OK;
}

CCLAPI 
RCODE SessMediaGetRTPParamByNum(IN pSessMedia media,IN RTPParamType rtype,IN INT32 codecnum,OUT pMediaPayload* pPl)
{
	char buf[8];

	memset(buf,0,8);
	i2a(codecnum,buf,8);
	return SessMediaGetRTPParam(media,rtype,buf,pPl);
}

CCLAPI 
RCODE SessMediaRemoveRTPParam(IN pSessMedia media,IN RTPParamType rtype,IN const char* payloadtype)
{
	DxLst mlist;
	UINT32 pos;
	pMediaPayload pl;
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaRemoveRTPParam] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	mlist=SessMediaGetlist(media,rtype);
	if (!mlist) {
		SESSWARN ("[SessMediaRemoveRTPParam] Invalid argument : Null pointer to list\n" );
		return RC_ERROR;
	}
	if (!payloadtype) {
		SESSWARN ("[SessMediaRemoveRTPParam] Invalid argument : Null pointer to payload type\n" );
		return RC_ERROR;
	}
	
	if (NULL==GetMatchPayload(mlist,payloadtype,&pos)) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaRemoveRTPParam] rtp paramter is not found\n" );
		return RC_ERROR;
	}
	pl=dxLstGetAt(mlist,pos);
	return MediaPayloadFree(&pl);
}

/* get media rtp parameter by index */
CCLAPI 
RCODE SessMediaGetRTPParamAt(IN pSessMedia media,IN RTPParamType rtype,IN UINT32 pos,OUT pMediaPayload* pPl)
{
	DxLst mlist;
	UINT32 size;
	
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetRTPParamAt] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!pPl) {
		SESSWARN ("[SessMediaGetRTPParamAt] Invalid argument : Null pointer to MediaPayload\n" );
		return RC_ERROR;
	}
	mlist=SessMediaGetlist(media,rtype);
	if (!mlist) {
		SESSWARN ("[SessMediaGetRTPParamAt] Invalid argument : Null pointer to list\n" );
		return RC_ERROR;
	}

	size=dxLstGetSize(mlist);
	if (pos>=size) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaGetRTPParamAt] position is too large\n" );
		return RC_ERROR;
	}
	*pPl=(pMediaPayload)dxLstPeek(mlist,pos);
	if (NULL==*pPl) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaGetRTPParamAt] rtp paramter is not found\n" );
		return RC_ERROR;
	}
	
	return RC_OK;
}

/* get count of rtp parameters */
CCLAPI 
RCODE SessMediaGetRTPParamSize(IN pSessMedia media,IN RTPParamType rtype,OUT UINT32* size)
{
	DxLst mlist;
	
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetRTPParamSize] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!size) {
		SESSWARN ("[SessMediaGetRTPParamSize] Invalid argument : Null pointer to MediaPayload\n" );
		return RC_ERROR;
	}
	mlist=SessMediaGetlist(media,rtype);
	if (!mlist) {
		SESSWARN ("[SessMediaGetRTPParamSize] Invalid argument : Null pointer to list\n" );
		return RC_ERROR;
	}
	*size=dxLstGetSize(mlist);	
	return RC_OK;
}

/* get Application Specific Maximum bandwidth of this media */
/* modifier is like : AS,CT,RR,RS */
CCLAPI 
RCODE SessMediaGetBandwidth(IN pSessMedia media,IN const char* modifier,IN UINT32* value)
{
	pBWInfo temp;
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaGetBandwidth] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!modifier) {
		SESSWARN ("[SessMediaGetBandwidth] Invalid argument : Null pointer to modifier\n" );
		return RC_ERROR;
	}
	if (!value) {
		SESSWARN ("[SessMediaGetBandwidth] Invalid argument : Null pointer to value\n" );
		return RC_ERROR;
	}

	dxLstResetIter(media->bandwidthlist);
	while(NULL!=(temp=(pBWInfo)dxLstPeekByIter(media->bandwidthlist))){
		if(temp->modifier && (strICmp(temp->modifier,modifier)==0)){
			*value=temp->value;
			return RC_OK;
		}
	}

	/* not match modifier  return error */
	return RC_ERROR;
	
}

CCLAPI 
RCODE SessMediaAddBandwidth(IN pSessMedia media,IN const char* modifier,IN UINT32 value)
{
	pBWInfo b=NULL;
	/* check parameter */
	if ( !media ) {
		SESSWARN ("[SessMediaAddBandwidth] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if (!modifier) {
		SESSWARN ("[SessMediaAddBandwidth] Invalid argument : Null pointer to modifier\n" );
		return RC_ERROR;
	}

	b=(pBWInfo)calloc(1,sizeof (struct BWobj));
	if(!b){
		SESSERR("[SessMediaAddBandwidth] memory alloc failed!");
		return RC_ERROR;
	}
	b->modifier=strDup(modifier);
	b->value=value;

	/* add to bandwidth list */
	if(!media->bandwidthlist)
		media->bandwidthlist=dxLstNew(DX_LST_POINTER);
	
	return dxLstPutTail(media->bandwidthlist,(void *)b);
}


/* get Application Specific Maximum bandwidth of this media */
/* modifier is like : AS,CT,RR,RS */
CCLAPI 
RCODE SessMediaGetAttribute(IN pSessMedia media,IN const char* attrib,IN char* buffer,IN UINT32 *length)
{
	UINT32 len;

	int size;
	int pos;
	
	char *value=NULL;
	pMediaAtt temp;

	/* check parameter */
	if ((NULL==media) || (NULL==media->attribelist)) {
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}

	if(NULL==attrib){
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : Null pointer to attrib\n" );
		return RC_ERROR;
	}

	if (NULL==buffer) {
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : Null pointer to buffer\n" );
		return RC_ERROR;
	}

	if( 0==*length) {
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : buffer size is zero!\n" );
		return RC_ERROR;
	}
	
	size=dxLstGetSize(media->attribelist);
	for(pos=0;pos<size;pos++){
		temp=(pMediaAtt)dxLstPeek(media->attribelist,pos);
		if(temp&&temp->name&&(strICmp(temp->name,attrib)==0)){
			value=temp->value;
			break;
		}
	}

	if( NULL==value) {
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : wrong attribe!\n" );
		return RC_ERROR;
	}

	len=(UINT32)strlen(value);
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,value);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessMediaGetAttribute] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	

	/* not match modifier  return error */
	return RC_ERROR;
	
}

CCLAPI 
RCODE SessMediaAddAttribute(IN pSessMedia media,IN const char* attrib,IN const char* value)
{
	RCODE retcode;
	pMediaAtt m;

	/* check parameter */
	if ( NULL==media ) {
		SESSWARN ("[SessMediaAddAttribute] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if ( NULL==attrib ) {
		SESSWARN ("[SessMediaAddAttribute] Invalid argument : Null pointer to attrib\n" );
		return RC_ERROR;
	}
	
	m=(pMediaAtt)calloc(1,sizeof (struct Attobj));
	if(NULL==m){
		SESSWARN ("[SessMediaAddAttribute] calloc memory is failed!\n" );
		return RC_ERROR;
	}

	m->name=strDup(attrib);
	if(value!=NULL)
		m->value=strDup(value);

	/* add to bandwidth list */
	if(!media->attribelist)
		media->attribelist=dxLstNew(DX_LST_POINTER);
	
	retcode=dxLstPutTail(media->attribelist,(void *)m);
	
	return retcode;
}

/* get "a=crypto:" */
CCLAPI RCODE SessMediaGetCrypto(IN pSessMedia media,IN int index,IN char* buffer,IN UINT32 *length)
{
	UINT32 len;
	char *retcrypto=NULL;

	/* check parameter */
	if ( ! media ) {
		SESSWARN("[SessMediaGetCrypto] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (!media->cryptolist) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetAddress] Null pointer to address\n" );
		return RC_ERROR;
	}

	retcrypto=(char *)dxLstPeek(media->cryptolist,index);

	if(!retcrypto){
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetAddress] Null pointer to address\n" );
		return RC_ERROR;
	}

	if( !length||!buffer){
		SESSWARN ("[SessMediaGetCrypto] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	len=strlen(retcrypto);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessMediaGetCrypto] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,retcrypto);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessMediaGetCrypto] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}	

}

CCLAPI RCODE SessMediaAddCrypto(IN pSessMedia media,IN const char* value)
{

	/* check parameter */
	if ( NULL==media ) {
		SESSWARN ("[SessMediaAddCrypto] Invalid argument : Null pointer to sessmedia\n" );
		return RC_ERROR;
	}
	if ( NULL==value ) {
		SESSWARN ("[SessMediaAddCrypto] Invalid argument : Null pointer to attrib\n" );
		return RC_ERROR;
	}

	if(NULL==media->cryptolist)
		media->cryptolist=dxLstNew(DX_LST_STRING);

	return dxLstPutTail(media->cryptolist,(void *)value);
}

/* print sess object to string */
RCODE SessMediaToStr(IN pSessMedia media,IN StatusAttr sessstatus,OUT char*buffer,IN OUT INT32* length)
{
	pMediaPayload pl;
	DxStr	MediaStr;
	INT32	len,size,pos,formatpos,plen;
	char	format[512]={'\0'};
	char	payload[128]={'\0'},ptype[64]={'\0'};
	pBWInfo tempbw;
	
	/* check parameter */
	if ( ! media ) {
		*length=0;
		SESSWARN("[SessMediaGetTransport] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}

	if( !length||!buffer){
		SESSWARN ("[SessMediaGetTransport] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	MediaStr=dxStrNew();
	if (!MediaStr) {
		SESSERR ("[SessMediaToStr] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* Media Announcements */
	/* m=<media> <port> <transport> <fmt list>*/
	formatpos=sprintf(format,"m=%s %u %s",SessMediaTypetoStr(media->type),media->port,media->transport);
	size=dxLstGetSize(media->rtpmaplist);
	for(pos=0;pos<size;pos++){
		pl=dxLstPeek(media->rtpmaplist,pos);
		plen=63;
		if (RC_OK==MediaPayloadGetType(pl,ptype,&plen))
			formatpos+=sprintf(format+formatpos," %s",ptype);
	}
	formatpos=sprintf(format+formatpos,"\r\n");
	dxStrCat(MediaStr,format);
	/* connection */
	if (media->address) {
		if(strchr(media->address,':'))
			sprintf(format,"c=IN IP6 %s\r\n",media->address);
		else
			sprintf(format,"c=IN IP4 %s\r\n",media->address);
		dxStrCat(MediaStr,format);
	}
	/* bandwidth information 
	if (media->bandwidthlist) {
		char* tmpbd=NULL;
		
		size=dxLstGetSize(media->bandwidthlist);
		for(pos=0;pos<size;pos++){
			sprintf(format,"b=%s\r\n",(char *)dxLstPeek(media->bandwidthlist,pos));
			dxStrCat(MediaStr,format);
		}
	}*/
	dxLstResetIter(media->bandwidthlist);
	while(NULL!=(tempbw=(pBWInfo)dxLstPeekByIter(media->bandwidthlist))){
		if(tempbw->modifier){
			sprintf(format,"b=%s:%d\r\n",tempbw->modifier,tempbw->value);
			dxStrCat(MediaStr,format);
		}
	}
	/* a=rtpmap*/
	for(pos=0;pos<size;pos++){
		pl=dxLstPeek(media->rtpmaplist,pos);
		plen=63;
		if (RC_OK==MediaPayloadGetType(pl,ptype,&plen)){
			plen=127;
			if (RC_OK==MediaPayloadGetParameter(pl,payload,&plen)){ 
				sprintf(format,"a=rtpmap:%s %s\r\n",ptype,payload);
				dxStrCat(MediaStr,format);
			}
		}
	}
	/* a=fmtp */
	if (media->fmtplist){ 
		size=dxLstGetSize(media->fmtplist);
		for (pos=0;pos<size;pos++) {
			pl=dxLstPeek(media->fmtplist,pos);
			plen=63;
			if (RC_OK==MediaPayloadGetType(pl,ptype,&plen)){
				plen=127;
				if (RC_OK==MediaPayloadGetParameter(pl,payload,&plen)){ 
					sprintf(format,"a=fmtp:%s %s\r\n",ptype,payload);
					dxStrCat(MediaStr,format);
				}
			}
		}
	}
	/* a=ptime */
	if (media->ptime>0) {
		sprintf(format,"a=ptime:%u\r\n",media->ptime);
		dxStrCat(MediaStr,format);
	}
	/* status attribute */
	if (media->status!=sessstatus) {
		sprintf(format,"a=%s\r\n",StatusAttrToStr(media->status));
		dxStrCat(MediaStr,format);
	}

	/* a=crypto */
	if (media->cryptolist){ 
		char *tmpcrypto=NULL;
		size=dxLstGetSize(media->cryptolist);
		for (pos=0;pos<size;pos++) {
			tmpcrypto=(char *)dxLstPeek(media->cryptolist,pos);
			sprintf(format,"a=crypto:%d %s\r\n",pos+1,tmpcrypto);
			dxStrCat(MediaStr,format);
				
		}
	}
	
	/* other a= */
	if(media->attribelist) {
		pMediaAtt tmpatt=NULL;
		size=dxLstGetSize(media->attribelist);

		for(pos=0;pos<size;pos++){
			tmpatt=(pMediaAtt)dxLstPeek(media->attribelist,pos);

			if((NULL==tmpatt) || (NULL==tmpatt->name)) continue;
			else{
				
				memset(format,0,sizeof format);

				if(tmpatt->value!=NULL)
					sprintf(format,"a=%s%s\r\n",tmpatt->name,tmpatt->value);
				else
					sprintf(format,"a=%s\r\n",tmpatt->name);
				
				dxStrCat(MediaStr,format);
			}
			
		}
		
	}

	memset(buffer,0,sizeof buffer);
	len=dxStrLen(MediaStr);
	if(len >= *length){
		*length=len+1;
		dxStrFree(MediaStr);
		return RC_ERROR;
	}else {
		*length=len+1;
		strcpy(buffer,(const char*)dxStrAsCStr(MediaStr));
		dxStrFree(MediaStr);
		return RC_OK;
	}	
}


DxLst SessMediaGetlist(IN pSessMedia media,IN RTPParamType rtype)
{
	/* check parameter */
	if (!media) {
		return NULL;
	}
	/* get list by input rtype */
	if (RTPMAP==rtype)
		return media->rtpmaplist;
	else if (FMTP==rtype)
		return media->fmtplist;
	else{
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaAddRTPParam] Invalid argument : wrong RTPParamType\n" );
		return NULL;
	}
}
pMediaPayload GetMatchPayload(DxLst paramlist,const char* payloadtype,UINT32* listpos)
{
	char mptype[64];
	UINT32 size,pos;
	INT32 len;
	pMediaPayload pl;
	
	/* check parameter */
	if (!paramlist || !listpos) 
		return NULL;

	size=dxLstGetSize(paramlist);
	for(pos=0;pos<size;pos++){
		pl=(pMediaPayload)dxLstPeek(paramlist,pos);
		len=63;
		if (RC_OK==MediaPayloadGetType(pl,mptype,&len)){
			if (strICmp(mptype,payloadtype)==0){
				*listpos=pos;
				return pl;
			}
		}	
	}
	return NULL;
}


char *StatusAttrToStr(IN StatusAttr status)
{
	switch(status){
	case SENDRECV:
		return "sendrecv";
	case SENDONLY:
		return "sendonly";
	case RECVONLY:
		return "recvonly";
	default:
		return "inactive";
	}
}

RCODE StatusAttrFromStr(IN const char *ta,OUT StatusAttr *pStatus)
{
	if (!ta || !pStatus) 
		return RC_ERROR;
	
	if(strICmp("sendonly",ta)==0){
		*pStatus=SENDONLY;
		return RC_OK;
	}	
	if(strICmp("recvonly",ta)==0){
		*pStatus=RECVONLY;
		return RC_OK;
	}

	if(strICmp("sendrecv",ta)==0){
		*pStatus=SENDRECV;
		return RC_OK;
	}
	
	if(strICmp("inactive",ta)==0){
		*pStatus=INACTIVE;
		return RC_OK;
	}
	return RC_ERROR;
}

char* SessMediaTypetoStr(IN MediaType mtype)
{
	switch(mtype){
	case MEDIA_AUDIO:
		return "audio";
	case MEDIA_VIDEO:
		return "video";
	case MEDIA_APPLICATION:
		return "application";
	case MEDIA_MESSAGE:
		return "message";
	default:
		return "data";
	}
}

/* transfer from sdpMsess */
RCODE SessMediaNewFromSdp(OUT pSessMedia *pMedia,IN SdpMsess msess,IN StatusAttr sessstatus)
{
	pSessMedia media;
	StatusAttr status;
	pMediaPayload payload;
	SdpTa MediaAttr;
	SdpTb	bandwidth;
	SdpTm  MediaAnn;
	SdpTc conn;
	INT32 size,pos,ptime;
	UINT32 mtype,mpos;
	char	addressbuf[512],*pparam,ptype[SDP_MAX_TA_VALUE_SIZE],*type;
	int bdsize;

	if (!pMedia || !msess) 
		return RC_ERROR;

	if (RC_OK!=sdpMsessGetTm(msess,&MediaAnn))
		return RC_ERROR;
	/* check media type */
	mtype=MediaAnn.Tm_type_;
	if (mtype<1) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaNewFromSdp] media type is not support\n" );
		return RC_ERROR;
	}
	
	/*
	if ((MediaAnn.protocol_!=1)&&(MediaAnn.protocol_!=2)) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaNewFromSdp] media protocol is not support\n" );
		return RC_ERROR;
	}
	*/
	
	/* check format list */
	size=MediaAnn.fmtSize_;
	if (size<1) {
		SessPrintEx ( TRACE_LEVEL_API,"[SessMediaNewFromSdp] no format list in media\n" );
		return RC_ERROR;
	}
	
	/* memory alloc */
	media=(pSessMedia)calloc(1,sizeof(SessMedia));
	if (!media) {
		SESSERR("[SessMediaNewFromSdp] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* "m=" set media type/port/transport */
	media->type=mtype-1;
	media->port=MediaAnn.port_;
	if (1==MediaAnn.protocol_) 
		media->transport=strDup("RTP/AVP");
	else if(2==MediaAnn.protocol_)
		media->transport=strDup("UDP");
	else if(3==MediaAnn.protocol_)
		media->transport=strDup("TCP/MSRP");
	else
		media->transport=strDup("NONE");

	/* "b=" bandwidth ;only get first sdpSessTb 
	if (RC_OK==sdpMsessGetTb(msess,&bandwidth)) 
			media->bandwidth=bandwidth.bandwidth_;
	*/

	/* "b= bandwidth information: get Tblist */
	bdsize=sdpMsessGetTbSize(msess);
	for(pos=0;pos<bdsize;pos++){
		if (RC_OK==sdpMsessGetTbAt(msess,pos,&bandwidth)) {		
			if(RC_OK!=SessMediaAddBandwidth(media,sdpTbModifer2Str(bandwidth.modifier_),bandwidth.bandwidth_)){
				SESSWARN("add bandwidth information failed!");
			}
		}
	}

	/* "c=" Connection Data */
	if ((sdpMsessGetTcSize(msess)>0)&&(RC_OK==sdpMsessGetTcAt(msess,0,&conn))) {
		if(conn.conn_flags_&SDP_TC_FLAG_TTL){
			if (conn.conn_flags_&SDP_TC_FLAG_NUMOFADDR) 
				sprintf(addressbuf,"%s/%u/%u",conn.pConnAddr_,conn.ttl_,conn.numOfAddr_);
			else
				sprintf(addressbuf,"%s/%u",conn.pConnAddr_,conn.ttl_);
		}else
			media->address=strDup(conn.pConnAddr_);
	}
	
	/* set rtpmap list */
	for(pos=0;pos<size;pos++){
		if (RC_OK==MediaPayloadNew(&payload,MediaAnn.fmtList_[pos],NULL)) {
			if (RC_OK!=SessMediaAddRTPParam(media,RTPMAP,payload)) {
				SESSERR("[SessMediaNewFromSdp] Add rtpmap is failed\n" );
				return RC_ERROR;
			}
		}
	}
	/* set default media status from session status */
	media->status=sessstatus;
	
	/* "a=" set rtpmap parameter / fmtp parameter / status attribute / ptime */
	size=sdpMsessGetTaSize(msess);
	for(pos=0;pos<size;pos++){
		if(RC_OK!=sdpMsessGetTaAt(msess,pos,&MediaAttr))
			continue;
		if (strICmp("rtpmap",MediaAttr.pAttrib_)==0) {
			strcpy(ptype,MediaAttr.pValue_);
			type=strchr(ptype,':');
			if (!type) continue;
			else
				*type++='\0';
			pparam=strchr(type,' ');
			if (pparam) 
				*pparam++='\0';
			/* add to list */
			if(GetMatchPayload(media->rtpmaplist,(const char*)type,&mpos)){
				if (RC_OK==MediaPayloadNew(&payload,type,pparam)) {
					if (RC_OK!=SessMediaAddRTPParam(media,RTPMAP,payload)){
						MediaPayloadFree(&payload);
						SessPrintEx ( TRACE_LEVEL_API,"[SessMediaNewFromSdp] add payload list to media is failed\n" );
					}	
				}
			}
		}else if(strICmp("ptime",MediaAttr.pAttrib_)==0){
			ptime=atoi(MediaAttr.pValue_+1);
			if (ptime>0) media->ptime=ptime;
		}else if(strICmp("fmtp",MediaAttr.pAttrib_)==0){
			strcpy(ptype,MediaAttr.pValue_);
			type=strchr(ptype,':');
			if (!type) continue;
			else
				*type++='\0';
			pparam=strchr(type,' ');
			if (pparam) 
				*pparam++='\0';
			/* add to list */
			if (RC_OK==MediaPayloadNew(&payload,type,pparam)) {
				if (RC_OK!=SessMediaAddRTPParam(media,FMTP,payload)){
					MediaPayloadFree(&payload);
					SessPrintEx ( TRACE_LEVEL_API,"[SessMediaNewFromSdp] add payload list to media is failed\n" );
				}	
			}
		}else if(strICmp("crypto",MediaAttr.pAttrib_)==0){
			strcpy(ptype,MediaAttr.pValue_);
			type=strchr(ptype,':');
			if (!type) continue;
			else
				*type++='\0';
			pparam=strchr(type,' ');
			if (pparam) 
				*pparam++='\0';
			/* add to list */
			SessMediaAddCrypto(media,pparam);
		}else{
			if (RC_OK==StatusAttrFromStr(MediaAttr.pAttrib_,&status) )
				media->status=status;
			else{
				/* add other a= to attribute list */
				if(strlen(MediaAttr.pAttrib_)>0)
					SessMediaAddAttribute(media,MediaAttr.pAttrib_,MediaAttr.pValue_);
			}
		}
	}

	*pMedia=media;
	return RC_OK;
}

void bwInfoFree(pBWInfo b)
{
	if(b) {
		if(b->modifier) free(b->modifier);
		memset(b,0,sizeof b);
		free(b);
	}
}

void mediaattFree(pMediaAtt m)
{
	if( m) {
		if(m->name) free(m->name);
		if(m->value) free(m->value);

		memset(m,0,sizeof m);
		free(m);
	}
}

