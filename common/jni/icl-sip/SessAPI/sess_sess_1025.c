/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_sess.c
 *
 * $Id: sess_sess.c,v 1.3 2008/10/15 08:34:01 tyhuang Exp $
 */

#include <stdio.h>

#ifdef _WIN32_WCE /*WinCE didn't support time.h*/
#include <winbase.h>
#elif defined(_WIN32)
#include <windows.h>
#include <time.h>
#elif defined (UNIX)
#include <sys/time.h>
#endif

#include <sdp/sdp.h>
#include <adt/dx_lst.h>
#include <adt/dx_str.h>
#include <common/cm_utl.h>

#include "sess_sess.h"
#include "sess_media.h"

struct sessObj{
	/*unsigned short protocolversion;*/
	char *owner;
	char *owneraddr;
	ULONG sessionid;
	ULONG version;
	char *sessionname;
	ULONG starttime;
	ULONG stoptime;
	char *address;
	StatusAttr status;
	UINT32 bandwidth;
	DxLst	medialist;

	/* other attribute */
	char *tool;
};

ULONG GetTimeTick();
RCODE GetSessionStatus(SdpSess sdp,StatusAttr *pStatus);
pSessMedia GetMediaByType(pSess,MediaType);

CCLAPI 
RCODE SessNew(OUT pSess* pSe,
			  IN const char *addr,	 
			  IN pSessMedia	media,	
			  IN UINT32 bandwidth)
{
	pSess se;
	/* check parameter */
	if ( !pSe ) {
		SESSWARN ("[SessNew] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !addr ) {
		SESSWARN ("[SessNew] Invalid argument : Null pointer to addr\n" );
		return RC_ERROR;
	}
	if ( !media ) {
		SESSWARN ("[SessNew] Invalid argument : Null pointer to media\n" );
		return RC_ERROR;
	}

	se=(pSess)calloc(1,sizeof(Sess));
	if (!se) {
		SESSERR ("[SessNew] Memory allocation error\n" );
		return RC_ERROR;
	}
	if(NULL==(se->medialist=dxLstNew(DX_LST_POINTER))){
		free(se);
		SESSERR ("[SessNew] Memory allocation error when add media \n" );
		return RC_ERROR;
	}
	dxLstPutTail(se->medialist,(void*)media);
	/*se->protocolversion=0;*/
	se->owner=strDup(DEFULT_OWNER);
	se->owneraddr=strDup(addr);
	se->sessionid=GetTimeTick();
	se->version=GetTimeTick();
	se->sessionname=strDup(SESSIONNAME);
	se->starttime=0; /* should set it by time() */
	se->stoptime=0; /* should set it by time() */
	se->address=strDup(addr);
	se->status=SENDRECV;
	se->bandwidth=bandwidth;
	
	*pSe=se;
	return RC_OK;

}
/* free sess object */
CCLAPI 
RCODE SessFree(IN pSess *pSe)
{
	pSess se;
	pSessMedia media;
	INT32 size,pos;

	/* check parameter */
	if ( !pSe ) {
		SESSWARN ("[SessFree] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	se=*pSe;
	if ( !se ) {
		SESSWARN ("[SessFree] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}

	if (se->medialist) {
		size=dxLstGetSize(se->medialist);
		for(pos=0;pos<size;pos++){
			media=dxLstGetAt(se->medialist,0);
			if (media) {
				SessMediaFree(&media);
			}else
				break;
		}
		dxLstFree(se->medialist,NULL);
	}

	if (se->owner) free(se->owner);
	if (se->owneraddr) free(se->owneraddr);
	if (se->sessionname) free(se->sessionname);
	if (se->address)	free(se->address); 
	if (se->tool) free(se->tool);
	
	memset(se,0,sizeof(Sess));
	free(se);
	*pSe=NULL;
	return RC_OK;
}
/* new sess from string */
CCLAPI 
RCODE SessNewFromStr(OUT pSess* pSe,IN const char*sdp)
{
	pSess se;
	pSessMedia media;

	SdpSess sdpstruct;
	SdpTo owner; /* o= */
	SdpTb	bandwidth; /* b= */
	SdpTc conn; /* c= */
	SdpTsess times;
	SdpTt	activetimes;
	SdpTa MediaAttr;
	SdpMsess sdpMedia; 

	UINT16	version;
	UINT32	buflen;
	INT32	mediasize,pos,size;
	char	sessionname[256];
	char	addressbuf[512];

	BOOL bConn=FALSE;
	
	/* check parameter */
	if ( !pSe ) {
		SESSWARN ("[SessNewFromStr] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!sdp) {
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] Invalid argument : Null pointer to sdp\n" );
		return RC_ERROR;
	}	
	sdpstruct=sdpSessNewFromText(sdp);
	if (!sdpstruct) {
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] parse error in input sdp\n" );
		return RC_ERROR;
	}
	/* check sdpsess */
	/* Protocol Version */
	if (RC_OK!=sdpSessGetTv(sdpstruct,&version)){
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no version in input sdp\n" );
		return RC_ERROR;
	}
	/* Origin */
	if (RC_OK!=sdpSessGetTo(sdpstruct,&owner)) {
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no owner in input sdp\n" );
		return RC_ERROR;
	}
	/* Session Name */
	buflen=255;
	if (RC_OK!=sdpSessGetTs(sdpstruct,sessionname,&buflen)) {
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no session id or too large one in input sdp\n" );
		return RC_ERROR;
	}
	/* Connection Data */
	if (RC_OK==sdpSessGetTc(sdpstruct,&conn)) {
		bConn=TRUE;
	}
	/* media */
	if ((mediasize=sdpSessGetMsessSize(sdpstruct))<1) {
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no media in input sdp\n" );
		return RC_ERROR;
	}
	/* Times */
	if ((times=sdpSessGetTsessAt(sdpstruct,0))==NULL) {
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no times in input sdp\n" );
		return RC_ERROR;
	}
	if (RC_OK!=sdpTsessGetTt(times,&activetimes)) {
		sdpSessFree(sdpstruct);
		SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no active times in input sdp\n" );
		return RC_ERROR;
	}
		
	/* new sess object and set value from sdpstruct */
	se=(pSess)calloc(1,sizeof(Sess));
	if (!se) {
		sdpSessFree(sdpstruct);
		SESSERR ("[SessNewFromStr] Memory allocation error\n" );
		return RC_ERROR;
	}
	if(NULL==(se->medialist=dxLstNew(DX_LST_POINTER))){
		sdpSessFree(sdpstruct);
		free(se);
		SESSERR ("[SessNewFromStr] Memory allocation error when add media \n" );
		return RC_ERROR;
	}
	/* Protocol Version */
	/*se->protocolversion=version;*/
	/* 
	 *	Origin ;o=<username><session id><version> <network type> <address type><address>
	 *	only save username sessionid version
	 */
	se->owner=strDup(owner.pUsername_);
	se->owneraddr=strDup(owner.pAddress_);
	se->sessionid=owner.sessionID_;
	se->version=owner.version_;
	/* Session Name */
	se->sessionname=strDup(sessionname);
	/* Connection Data */
	if(bConn){
		if(conn.conn_flags_&SDP_TC_FLAG_TTL){
			if (conn.conn_flags_&SDP_TC_FLAG_NUMOFADDR)  
				sprintf(addressbuf,"%s/%u/%u",conn.pConnAddr_,conn.ttl_,conn.numOfAddr_);
			else
				sprintf(addressbuf,"%s/%u",conn.pConnAddr_,conn.ttl_);
			se->address=strDup(addressbuf);
		}else
			se->address=strDup(conn.pConnAddr_);
	}
	/* bandwidth ;only get first sdpSessTb*/
	if (RC_OK==sdpSessGetTbAt(sdpstruct,0,&bandwidth)) 
			se->bandwidth=bandwidth.bandwidth_;
	/* Times */
	se->starttime=0; /* should set it by t= */
	se->stoptime=0; /* should set it by t= */
	/* status attribute */
	GetSessionStatus(sdpstruct,&se->status);

	/* tool attribute */
	/* "a=tool:value" */
	size=sdpSessGetTaSize(sdpstruct);
	for(pos=0;pos<size;pos++){
		if(RC_OK!=sdpSessGetTaAt(sdpstruct,pos,&MediaAttr))
			continue;
		if (strICmp("tool",MediaAttr.pAttrib_)==0) {
			se->tool=strDup(MediaAttr.pValue_+1); /* add one for skip ':' */
			break;
		}
	}

	/* media*/
	mediasize=sdpSessGetMsessSize(sdpstruct);
	for(pos=0;pos<mediasize;pos++){
		sdpMedia=sdpSessGetMsessAt(sdpstruct,pos);
		if (RC_OK==SessMediaNewFromSdp(&media,sdpMedia,se->status)) {
			if (RC_OK!=SessAddMedia(se,media)) 
				SessPrintEx (TRACE_LEVEL_API,"[SessNewFromStr] no active times in input sdp\n" );
		}
	}
	/* clean local alloc memory */
	sdpSessFree(sdpstruct);
	*pSe=se;
	return RC_OK;
}

/* print sess object to string */
CCLAPI 
RCODE SessToStr(IN pSess se,OUT char *buffer,IN OUT INT32*length)
{
	pSessMedia media=NULL;
	DxStr	SdpStr;
	INT32	len,size,pos,buflen;
	char	format[256]={'\0'},connect[256]={'\0'};
	char	mediabuf[512]={'\0'};

	/* check parameter */
	if ( !se ) {
		SESSWARN("[SessToStr] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !se->owner || !se->sessionname ||!se->owneraddr ||!se->medialist){
		SESSWARN("[SessToStr] Invalid argument : Null pointer to sess parameter\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if( !length || !buffer){
		SESSWARN ("[SessToStr] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}

	/* start to print sdp */
	SdpStr=dxStrNew();
	if (!SdpStr) {
		SESSERR ("[SessToStr] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* Protocol Version */
	dxStrCat(SdpStr, "v=0\r\n");
	/* Origin ;o=<username><session id><version> <network type> <address type><address>*/
	sprintf(format,"o=%s %lu %lu ",se->owner,se->sessionid,se->version);
	dxStrCat(SdpStr,format);

	if(NULL!=se->owneraddr){
		if (strchr(se->owneraddr,':')) {
			sprintf(connect,"IN IP6 %s\r\n",se->owneraddr);
		}else
			sprintf(connect,"IN IP4 %s\r\n",se->owneraddr);
		dxStrCat(SdpStr,connect);
	}
	
	/* Session Name */
	sprintf(format,"s=%s\r\n",se->sessionname);
	dxStrCat(SdpStr,format);
	/* Connection Data */
	if(NULL!=se->address){
		if (strchr(se->address,':')) {
			sprintf(connect,"IN IP6 %s\r\n",se->address);
		}else
			sprintf(connect,"IN IP4 %s\r\n",se->address);
		
		dxStrCat(SdpStr,"c=");
		dxStrCat(SdpStr,connect);
	}
	
	/* bandwidth */
	if (se->bandwidth>0) {
		sprintf(format,"b=%s:%u\r\n",DEFAUL_MODIFIER,se->bandwidth);
		dxStrCat(SdpStr,format);
	}

	/* Times */
	sprintf(format,"t=%lu %lu\r\n",se->starttime,se->stoptime);
	dxStrCat(SdpStr,format);

	/* status attribute */
	if (SHOW_SENDRECV || (se->status != SENDRECV)) {
		sprintf(format,"a=%s\r\n",StatusAttrToStr(se->status));
		dxStrCat(SdpStr,format);
	}

	/* tool attribute */
	if (se->tool !=NULL){
		sprintf(format,"a=tool:%s\r\n",se->tool);
		dxStrCat(SdpStr,format);
	}
	
	/* print media */
	size=dxLstGetSize(se->medialist);
	for(pos=0;pos<size;pos++){
		media=(pSessMedia)dxLstPeek(se->medialist,pos);
		buflen=511;
		
		if (RC_OK==SessMediaToStr(media,se->status,mediabuf,&buflen)) 
			dxStrCat(SdpStr,mediabuf);
	}

	memset(buffer,0,sizeof buffer);
	len=dxStrLen(SdpStr);
	if(len >= *length){
		*length=len+1;
		dxStrFree(SdpStr);
		return RC_ERROR;
	}else {
		*length=len+1;
		strcpy(buffer,(const char*)dxStrAsCStr(SdpStr));
		dxStrFree(SdpStr);
		return RC_OK;
	}	
}

/* set address of sess object */
CCLAPI 
RCODE SessSetAddress(IN pSess se,IN const char *addr)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetAddress] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !addr ) {
		SESSWARN ("[SessSetAddress] Invalid argument : Null pointer to addr\n" );
		return RC_ERROR;
	}

	if (se->address) free(se->address);
	se->address=strDup(addr);

	return RC_OK;
}

/* get address of sess object */
CCLAPI 
RCODE SessGetAddress(IN pSess se,OUT char*buffer,IN OUT INT32*length)
{
	INT32 len;

	/* check parameter */
	if ( ! se ) {
		SESSWARN("[SessGetAddress] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!se->address) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetAddress] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if( !length || !buffer){
		SESSWARN ("[SessGetAddress] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	len=strlen(se->address);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetAddress] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,se->address);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessGetAddress] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/* set sess status attribute.status attribute:SENDRECV/SENDONLY/RECVONLY/INACTIVE*/
CCLAPI 
RCODE SessSetStatus(IN pSess se,IN StatusAttr status)
{
	INT32 size,pos;
	pSessMedia media;
	StatusAttr mstatus,sstatus;
	DxLst mlist;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetStatus] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	/* should set media if status of media is the same as one of session */
	mlist=se->medialist;
	sstatus=se->status;
	size=dxLstGetSize(mlist);
	for(pos=0;pos<size;pos++){
		media=(pSessMedia)dxLstPeek(mlist,pos);
		if (RC_OK==SessMediaGetStatus(media,&mstatus)) 
			if (mstatus==sstatus)
				SessMediaSetStatus(media,status);
	}
	se->status=status;
	return RC_OK;
}

/* get status attribute */
CCLAPI 
RCODE SessGetStatus(IN pSess se,OUT StatusAttr *pStatus)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessGetStatus] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!pStatus) {
		SESSWARN ("[SessGetStatus] Invalid argument : Null pointer to status\n" );
		return RC_ERROR;
	}
	*pStatus=se->status;
	return RC_OK;
}

/* set sess tool attribute */
CCLAPI 
RCODE SessSetTool(IN pSess se,IN const char *tool)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !tool ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to tool\n" );
		return RC_ERROR;
	}

	if (se->tool) free(se->tool);
	se->tool=strDup(tool);

	return RC_OK;
}

/* get sess tool attribute */
CCLAPI 
RCODE SessGetTool(IN pSess se,OUT char*buffer,IN OUT INT32*length)
{
	INT32 len;
	/* check parameter */
	if ( ! se ) {
		SESSWARN("[SessGetOwner] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!se->tool) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetOwner] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if( !length || !buffer){
		SESSWARN ("[SessGetOwner] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	len=strlen(se->tool);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetOwner] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,se->tool);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessGetOwner] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/* set/get sess owner */
CCLAPI 
RCODE SessSetOwner(IN pSess se,IN const char*owner)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !owner ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to owner\n" );
		return RC_ERROR;
	}

	if (se->owner) free(se->owner);
	se->owner=strDup(owner);

	return RC_OK;	
}
CCLAPI 
RCODE SessGetOwner(IN pSess se,OUT char*buffer,IN OUT INT32*length)
{
	INT32 len;
	/* check parameter */
	if ( ! se ) {
		SESSWARN("[SessGetOwner] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!se->owner) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetOwner] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if( !length || !buffer){
		SESSWARN ("[SessGetOwner] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	len=strlen(se->owner);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetOwner] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,se->owner);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessGetOwner] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/* set/get sess name */
CCLAPI 
RCODE SessSetSessName(IN pSess se,IN const char* name)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !name ) {
		SESSWARN ("[SessSetOwner] Invalid argument : Null pointer to owner\n" );
		return RC_ERROR;
	}

	if (se->sessionname) free(se->sessionname);
	se->sessionname=strDup(name);

	return RC_OK;
}
CCLAPI 
RCODE SessGetSessName(IN pSess se,OUT char*buffer,IN OUT INT32*length)
{
	INT32 len;
	/* check parameter */
	if ( ! se ) {
		SESSWARN("[SessGetSessName] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!se->sessionname) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetSessName] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if( !length || !buffer){
		SESSWARN ("[SessGetSessName] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	len=strlen(se->sessionname);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[SessGetSessName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,se->sessionname);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[SessGetSessName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/* get session id */
CCLAPI 
RCODE SessGetSessID(IN pSess se,OUT ULONG* sid)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessGetSessID] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}

	if ( !sid ) {
		SESSWARN ("[SessGetSessID] Invalid argument : Null pointer to sid\n" );
		return RC_ERROR;
	}

	*sid=se->sessionid;
	return RC_OK;
}

/* increase session version by one */
CCLAPI RCODE SessAddVersion(IN pSess se)
{
	ULONG version;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessAddVersion] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	version=se->version;
	se->version++;
	if (se->version<version) 
		SESSERR("[SessAddVersion] session version is overflow\n" );

	return RC_OK;
}

/* get session version */
CCLAPI 
RCODE SessGetVersion(IN pSess se,OUT ULONG* sver)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessGetVersion] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}

	if( !sver ) {
		SESSWARN ("[SessGetVersion] Invalid argument : Null pointer to sver\n" );
		return RC_ERROR;
	}

	*sver=se->version;
	return RC_OK;
}

/* get media from sess object */
CCLAPI 
RCODE SessGetMedia(IN pSess se,IN INT32 pos,OUT pSessMedia *pMedia)
{
	INT32 size;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessGetMedia] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!pMedia) {
		SESSWARN ("[SessGetMedia] Invalid argument : Null pointer to media\n" );
		return RC_ERROR;
	}

	if (!se->medialist) {
		SESSWARN ("[SessGetMedia] Invalid argument : Null pointer to medialist\n" );
		return RC_ERROR;
	}
	size=dxLstGetSize(se->medialist);
	if (pos>=size) {
		SessPrintEx (TRACE_LEVEL_API,"[SessGetMedia] position is too large\n" );
		return RC_ERROR;
	}
	*pMedia=(pSessMedia)dxLstPeek(se->medialist,pos);

	return RC_OK;
}

/* Add media from sess object */
CCLAPI 
RCODE SessAddMedia(IN pSess se,IN pSessMedia media)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessAddMedia] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}	
	if (!media) {
		SESSWARN ("[SessAddMedia] Invalid argument : Null pointer to media\n" );
		return RC_ERROR;
	}

	if (!se->medialist) {
		if(NULL==(se->medialist=dxLstNew(DX_LST_POINTER))){
			SESSERR ("[SessAddMedia] Memory allocation error when add media \n" );
			return RC_ERROR;
		}
	}

	return dxLstPutTail(se->medialist,media);
}

/* Remove media from sess object */
CCLAPI 
RCODE SessRemoveMedia(IN pSess se,IN INT32 pos)
{
	INT32 size;
	pSessMedia media;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessRemoveMedia] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if (!se->medialist) {
		SESSWARN ("[SessRemoveMedia] Invalid argument : Null pointer to medialist\n" );
		return RC_ERROR;
	}
	size=dxLstGetSize(se->medialist);
	if (pos>=size) {
		SessPrintEx (TRACE_LEVEL_API,"[SessRemoveMedia] position is too large\n" );
		return RC_ERROR;
	}
	
	media=(pSessMedia)dxLstGetAt(se->medialist,pos);
	
	return SessMediaFree(&media);	
}

/* get media count from sess object */
CCLAPI 
RCODE SessGetMediaSize(IN pSess se,OUT INT32 *size)
{
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessGetMediaSize] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !size ) {
		SESSWARN ("[SessGetMediaSize] Invalid argument : Null pointer to count\n" );
		return RC_ERROR;
	}
	if (!se->medialist)
		*size=0;
	else
		*size=dxLstGetSize(se->medialist);
	 return RC_OK;
}

CCLAPI 
RCODE SessCheckHold(IN pSess se,OUT HoldStyle *hs)
{
	pSessMedia media;
	StatusAttr status;
	char address[256];
	INT32 len;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessCheckHold] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	if ( !hs ) {
		SESSWARN ("[SessCheckHold] Invalid argument : Null pointer to count\n" );
		return RC_ERROR;
	}
	*hs=NOT_HOLD;

	media=GetMediaByType(se,MEDIA_AUDIO);
	/* check connection */
	memset(address,0,sizeof(address));
	len=255;
	if (RC_OK!=SessMediaGetAddress(media,address,&len)) {
		len=255;
		if (RC_OK!=SessGetAddress(se,address,&len)) {
			SESSWARN ("[SessCheckHold] bad sdp without sdp\n" );
			return RC_ERROR;
		}
	}
	if (strstr(address,"0.0.0.0")){
		*hs=CZEROS;
		return RC_OK;
	}
	/* media status */
	if (RC_OK==SessMediaGetStatus(media,&status)) {
		if ((status<=INACTIVE)&&(status>SENDRECV))
				*hs=status+1;
		else
			SESSWARN("[SessCheckHold] bad status of media\n");
	}
	return RC_OK;
}

CCLAPI 
RCODE SessSetHold(IN pSess se,IN HoldStyle hs)
{
	pSessMedia media;
	StatusAttr status;

	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetHold] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}
	media=GetMediaByType(se,MEDIA_AUDIO);
	if (!media) {
		SESSWARN ("[SessSetHold] Invalid argument : no audio media to hold\n" );
		return RC_ERROR;
	}
	/* set session connection 0.0.0.0 */
	if (CZEROS==hs)
		return SessSetAddress(se,"0.0.0.0");
	else if ((hs>CZEROS)&&(hs<=AUDIO_INACTIVE)){ 
		status=hs-1;
		return SessMediaSetStatus(media,status);
	}else{
		SESSWARN ("[SessSetHold] Invalid argument : wrong hold style\n" );
		return RC_OK;
	}
}
CCLAPI 
RCODE SessSetUnHold(IN pSess se,IN const char* address)
{
	pSessMedia media;
	/* check parameter */
	if ( !se ) {
		SESSWARN ("[SessSetUnHold] Invalid argument : Null pointer to sess\n" );
		return RC_ERROR;
	}

	/*
	if (!address) {
		SESSWARN ("[SessSetUnHold] Invalid argument : Null pointer to address\n" );
		return RC_ERROR;
	}
	*/

	media=GetMediaByType(se,MEDIA_AUDIO);
	if (!media) {
		SESSWARN ("[SessSetUnHold] Invalid argument : no audio media to hold\n" );
		return RC_ERROR;
	}

	/* set session connection address */
	if(address!=NULL){
	
		if (RC_OK!=SessSetAddress(se,address)) {
			SESSWARN ("[SessSetUnHold] set new address is failed\n" );
			return RC_ERROR;
		}
	}

	if (RC_OK!=SessMediaSetStatus(media,SENDRECV)) {
		SESSWARN ("[SessSetUnHold] set media status to sendrecv is failed\n" );
		return RC_ERROR;
	}
	return RC_OK;
}


pSessMedia GetMediaByType(pSess sess,MediaType mtype)
{
	pSessMedia media=NULL;
	INT32 size,pos;
	MediaType tmpmtype;

	if (!sess || !sess->medialist) 
		return media;
	size=dxLstGetSize(sess->medialist);
	for (pos=0;pos<size;pos++) {
		media=(pSessMedia)dxLstPeek(sess->medialist,pos);
		if (RC_OK==SessMediaGetType(media,&tmpmtype)) {
			if (mtype==tmpmtype) break;
		}else
			media=NULL;
	}

	return media;
}

ULONG GetTimeTick()
{
ULONG ltime;
#ifdef _WIN32
	ltime=(ULONG) GetTickCount();
#elif defined(UNIX)
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(ULONG) (tv.tv_sec * 1000 + tv.tv_usec / 1000);

#else
	ltime=(ULONG)rand();
#endif

	return ltime;
}

RCODE GetSessionStatus(SdpSess sdp,StatusAttr *pStatus)
{
	SdpTa SDPAttr;
	INT32 pos,size;
	StatusAttr status=SENDRECV;
	
	if (!sdp) 
		return RC_ERROR;
	if (!pStatus) 
		return RC_ERROR;

	size=sdpSessGetTaSize(sdp);
	for(pos=0;pos<size;pos++){
		sdpSessGetTaAt(sdp,pos,&SDPAttr);
		/* if found status attribute ,break else re-store to sendrecv  */
		if (RC_OK==StatusAttrFromStr(SDPAttr.pAttrib_,&status) )
			break;
		else
			status=SENDRECV;
	}
	*pStatus=status;
	return RC_OK;
}


