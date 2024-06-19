/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_media.h
 *
 * $Id: sess_media.h,v 1.2 2008/12/05 02:53:38 tyhuang Exp $
 */

#ifndef SESS_MEDIA_H
#define SESS_MEDIA_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sdp/sdp_mses.h>

#include "sess_cm.h"
/* new a media with a rtpmap;
 * port must not be zero,
 * rtpmap must not be NULL
 */
CCLAPI RCODE SessMediaNew(OUT pSessMedia*,
						   IN MediaType type,
						   IN const char* address,
						   IN UINT32 port,
						   IN const	char* transport,
						   IN UINT32 ptime,
						   IN RTPParamType rtype,
						   IN pMediaPayload rtpmap);

/* free media */
CCLAPI RCODE SessMediaFree(IN pSessMedia*);

CCLAPI 
RCODE SessMediaDup(IN pSessMedia *pMedia,IN pSessMedia *pnewMedia);

/* get media type:AUDIO/VIDEO/DATA/APPLICATION */
CCLAPI RCODE SessMediaGetType(IN pSessMedia,OUT MediaType*);

/* set media port */
CCLAPI RCODE SessMediaSetPort(IN pSessMedia,OUT UINT32);
/* get media port */
CCLAPI RCODE SessMediaGetPort(IN pSessMedia,OUT UINT32*);

/* set media transport type */
CCLAPI RCODE SessMediaSetTransport(IN pSessMedia,OUT const char*);
/* get media transport type */
CCLAPI RCODE SessMediaGetTransport(IN pSessMedia,OUT char* buffer,IN OUT INT32* length);

/* set media connection address */
CCLAPI RCODE SessMediaSetAddress(IN pSessMedia,OUT const char*);
/* get media connection address */
CCLAPI RCODE SessMediaGetAddress(IN pSessMedia,OUT char* buffer,IN OUT INT32* length);

/* set media status attribute:SENDRECV/SENDONLY/RECVONLY/INACTIVE */
CCLAPI RCODE SessMediaSetStatus(IN pSessMedia ,IN StatusAttr);
/* set media status attribute */
CCLAPI RCODE SessMediaGetStatus(IN pSessMedia,OUT StatusAttr*);

/* set media parameters;MediaParamType:ptime/bandwidth */
CCLAPI RCODE SessMediaSetParam(IN pSessMedia,IN MediaParamType mptype,IN UINT32 intvalue);
/* get media parameters;MediaParamType:ptime/bandwidth */
CCLAPI RCODE SessMediaGetParam(IN pSessMedia,IN MediaParamType mptype,OUT UINT32* value);

/* Add rtp parameter to media;RTPParamType: RTPMAP/FMTP */
CCLAPI RCODE SessMediaAddRTPParam(IN pSessMedia,IN RTPParamType rtype,IN pMediaPayload);

/* Get rtp parameter from media ; match parameter by payloadtype */
CCLAPI RCODE SessMediaGetRTPParam(IN pSessMedia,IN RTPParamType rtype,IN const char* payloadtype,OUT pMediaPayload*);
CCLAPI RCODE SessMediaGetRTPParamByNum(IN pSessMedia,IN RTPParamType rtype,IN INT32 codecnum,OUT pMediaPayload*);

/* Remove rtp parameter from media ; match parameter by payloadtype */
CCLAPI RCODE SessMediaRemoveRTPParam(IN pSessMedia,IN RTPParamType rtype,IN const char* payloadtype);

/* get media rtp parameter by index */
CCLAPI RCODE SessMediaGetRTPParamAt(IN pSessMedia,IN RTPParamType rtype,IN UINT32 pos,OUT pMediaPayload*);

/* get count of rtp parameters */
CCLAPI RCODE SessMediaGetRTPParamSize(IN pSessMedia,IN RTPParamType rtype,OUT UINT32*);

/* get Application Specific Maximum bandwidth of this media */
/* modifier is like : AS,CT,RR,RS */
CCLAPI RCODE SessMediaGetBandwidth(IN pSessMedia,IN const char* modifier,IN UINT32* value);
CCLAPI RCODE SessMediaAddBandwidth(IN pSessMedia,IN const char* modifier,IN UINT32 value);
CCLAPI RCODE SessMediaSetBandwidth(IN pSessMedia media,IN const char* modifier,IN UINT32 value);

/* get "a=" */
CCLAPI RCODE SessMediaGetAttribute(IN pSessMedia media,IN const char* attrib,IN char* buffer,IN UINT32 *length);
CCLAPI RCODE SessMediaAddAttribute(IN pSessMedia media,IN const char* attrib,IN const char* value);

/* get "a=crypto:" */
CCLAPI RCODE SessMediaGetCrypto(IN pSessMedia media,IN int index,IN char* buffer,IN UINT32 *length);
CCLAPI RCODE SessMediaAddCrypto(IN pSessMedia media,IN const char* value);

/*
 *	internal function
 */
char* SessMediaTypetoStr(MediaType mtype);
/* print sess object to string */
RCODE SessMediaToStr(IN pSessMedia,IN StatusAttr,OUT char*buffer,IN OUT INT32*length);
char *StatusAttrToStr(IN StatusAttr status);
RCODE StatusAttrFromStr(IN const char *ta,OUT StatusAttr *pStatus);
/* transfer from sdpMsess */
RCODE SessMediaNewFromSdp(OUT pSessMedia*,IN SdpMsess,IN StatusAttr);
#ifdef  __cplusplus
}
#endif

#endif
