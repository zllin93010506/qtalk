/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_payload.c
 *
 * $Id: sess_payload.c,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#include <common/cm_utl.h>

#include "sess_payload.h"


struct plObj{
	char *type;
	char *parameter;
};

/** \fn RCODE MediaPayloadNew(OUT pMediaPayload *pPl,IN const char* payloadtype,IN const char *parameter)
 *	\brief to create a media payload like 0 for G711u
 *	\param pPl - a pointer to MediaPayload pointer;NOT NULL
 *	\param payloadtype - payload type of this new media payload;NOT NULL and length must below 63 chars
 *	\param parameter - other parameter for this payload if parameter present,it will be print as "a=rtpmap:0 G711u/8000/1" for example
 *
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI 
RCODE MediaPayloadNew(OUT pMediaPayload *pPl,IN const char* payloadtype,IN const char *parameter)
{
	pMediaPayload pl=NULL;
	/* check parameter */
	if ( ! pPl ) {
		SESSWARN ("[MediaPayloadNew] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if( !payloadtype ) {
		SessPrintEx ( TRACE_LEVEL_API, "[MediaPayloadNew] Invalid argument : Null pointer to payload type\n" );
		return RC_ERROR;
	}
	if (strlen(payloadtype)>63) {
		SessPrintEx ( TRACE_LEVEL_API, "[MediaPayloadNew] payload type is too large \n" );
		return RC_ERROR;
	}

	pl=(pMediaPayload)calloc(1,sizeof(MediaPayload));
	if (!pl) {
		SESSERR("[MediaPayloadNew] Memory allocation error\n" );
		return RC_ERROR;
	}

	pl->type=strDup(payloadtype);
	if (parameter) 
		pl->parameter=strDup(parameter);

	*pPl=pl;
	return RC_OK;
}

/** \fn RCODE MediaPayloadDup(IN pMediaPayload oldpl,OUT pMediaPayload* pPl)
 *	\brief to duplicate a media payload
 *	\param oldpl - original MediaPayloadNew;NOT NULL
 *	\param pPl - a pointer to MediaPayload pointer;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI 
RCODE MediaPayloadDup(IN pMediaPayload oldpl,OUT pMediaPayload* pPl)
{
	pMediaPayload pl=NULL;
	/* check parameter */
	if ( ! oldpl ) {
		SESSWARN ("[MediaPayloadDup] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}	
	
	pl=(pMediaPayload)calloc(1,sizeof(MediaPayload));
	if (!pl) {
		SESSERR("[MediaPayloadDup] Memory allocation error\n" );
		return RC_ERROR;
	}

	if(oldpl->type)
		pl->type=strDup(oldpl->type);
	if(oldpl->parameter)
		pl->parameter=strDup(oldpl->parameter);

	*pPl=pl;
	return RC_OK;
}

/** \fn RCODE MediaPayloadFree(IN pMediaPayload *pPl)
 *	\brief to free a media payload
 *	\param pPl - a pointer to MediaPayload pointer;NOT NULL
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI 
RCODE MediaPayloadFree(IN pMediaPayload *pPl)
{
	pMediaPayload pl;
	/* check parameter */
	if ( ! pPl ) {
		SESSWARN ("[MediaPayloadFree] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	pl=*pPl;
	if( !pl ) {
		SESSWARN ("[MediaPayloadFree] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (pl->type) 
		free(pl->type);
	if (pl->parameter) 
		free(pl->parameter);
	memset(pl,0,sizeof(MediaPayload));
	free(pl);
	*pPl=NULL;

	return RC_OK;
}

/** \fn RCODE MediaPayloadGetType(IN pMediaPayload pl,OUT char*buffer,IN OUT INT32 *length)
 *	\brief to get payload type of a media payload
 *	\param pPl - a pointer to MediaPayload pointer;NOT NULL
 *	\param buffer - buffer for output values
 *	\param length - information of the length of buffer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI 
RCODE MediaPayloadGetType(IN pMediaPayload pl,OUT char*buffer,IN OUT INT32 *length)
{
	INT32 len;
	/* check parameter */
	if ( ! pl ) {
		SESSWARN ("[MediaPayloadGetType] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (!buffer||!length) {
		SESSWARN ("[MediaPayloadGetType] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}
	if (!pl->type) {
		SESSWARN ("[MediaPayloadGetType] Invalid argument : Null pointer to type\n" );
		return RC_ERROR;
	}
		
	/* check input buffer size */
	len=strlen(pl->type);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[MediaPayloadGetType] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,pl->type);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[MediaPayloadGetType] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/** \fn RCODE MediaPayloadGetType(IN pMediaPayload pl,OUT char*buffer,IN OUT INT32 *length)
 *	\brief to get payload parameter of a media payload
 *	\param pPl - a pointer to MediaPayload pointer;NOT NULL
 *	\param buffer - buffer for output values
 *	\param length - information of the length of buffer
 *	\retval RCODE RC_OK for success;RC_ERROR for failure
 */
CCLAPI 
RCODE MediaPayloadGetParameter(IN pMediaPayload pl,OUT char*buffer,IN OUT INT32 *length)
{
	/* check parameter */
	INT32 len;
	if ( ! pl ) {
		SESSWARN("[MediaPayloadGetParameter] Invalid argument : Null pointer to payload\n" );
		return RC_ERROR;
	}
	if (!pl->parameter) {
		*length=0;
		SessPrintEx ( TRACE_LEVEL_API, "[MediaPayloadGetParameter] Null pointer to parameter\n" );
		return RC_ERROR;
	}

	if( !length||!buffer){
		SESSWARN ("[MediaPayloadGetParameter] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	len=strlen(pl->parameter);
	if(*length==0){
		*length=len+1;
		SessPrintEx ( TRACE_LEVEL_API, "[MediaPayloadGetParameter] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(buffer,0,sizeof(buffer));
		/* copy data */
		strcpy(buffer,pl->parameter);
		*length=len+1;
		return RC_OK;
	}else{
		SESSWARN ("[MediaPayloadGetParameter] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}
