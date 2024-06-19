/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_payload.h
 *
 * $Id: sess_payload.h,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#ifndef SESS_PAYLOAD_H
#define SESS_PAYLOAD_H

#ifdef  __cplusplus
extern "C" {
#endif
#include "sess_cm.h"

/* new mediapayload : <payloadtype> <encodingparameters> */
CCLAPI RCODE MediaPayloadNew(OUT pMediaPayload*,IN const char* payloadtype,IN const char*parameter);

/* free mediapayload */
CCLAPI RCODE MediaPayloadDup(IN pMediaPayload oldpl,OUT pMediaPayload*);

/* free mediapayload */
CCLAPI RCODE MediaPayloadFree(IN pMediaPayload*);

/* get payload type */
CCLAPI RCODE MediaPayloadGetType(IN pMediaPayload,OUT char*buffer,IN OUT INT32*length);

/* get encoding parameters */
CCLAPI RCODE MediaPayloadGetParameter(IN pMediaPayload,OUT char*buffer,IN OUT INT32*length);


#ifdef  __cplusplus
}
#endif

#endif

