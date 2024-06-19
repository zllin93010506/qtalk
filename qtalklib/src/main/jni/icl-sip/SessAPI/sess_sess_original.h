/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sess_sess.h
 *
 * $Id: sess_sess.h,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#ifndef SESS_SESS_H
#define SESS_SESS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "sess_cm.h"
/* new a sess object with a SessMedia
 * address must not be NULL
 * media must not be NULL
 */
CCLAPI RCODE SessNew(OUT pSess*,
					 IN const char *addr,	 /*rtp address*/
					 IN pSessMedia	media,	/*media list*/
					 IN UINT32 bandwidth);
/* free sess object */
CCLAPI RCODE SessFree(IN pSess*);

/* new sess from string */
CCLAPI RCODE SessNewFromStr(OUT pSess*,IN const char*);
/* print sess object to string */
CCLAPI RCODE SessToStr(IN pSess,OUT char*buffer,IN OUT INT32*length);

/* set address of sess object */
CCLAPI RCODE SessSetAddress(IN pSess,IN const char*);
/* get address of sess object */
CCLAPI RCODE SessGetAddress(IN pSess,OUT char*buffer,IN OUT INT32*length);
/* set sess status attribute.status attribute:SENDRECV/SENDONLY/RECVONLY/INACTIVE*/

CCLAPI RCODE SessSetStatus(IN pSess,IN StatusAttr);
/* get status attribute */
CCLAPI RCODE SessGetStatus(IN pSess,OUT StatusAttr*);

/* set/get sess owner */
CCLAPI RCODE SessSetOwner(IN pSess,IN const char*);
CCLAPI RCODE SessGetOwner(IN pSess,OUT char*buffer,IN OUT INT32*length);

/* set/get sess name */
CCLAPI RCODE SessSetSessName(IN pSess,IN const char*);
CCLAPI RCODE SessGetSessName(IN pSess,OUT char*buffer,IN OUT INT32*length);

/* get session id */
CCLAPI RCODE SessGetSessID(IN pSess,OUT ULONG*);

/* increase session version by one */
CCLAPI RCODE SessAddVersion(IN pSess);
/* get session version */
CCLAPI RCODE SessGetVersion(IN pSess,OUT ULONG*);

/* get media from sess object */
CCLAPI RCODE SessGetMedia(IN pSess,IN INT32 pos,OUT pSessMedia*);
/* Add media from sess object */
CCLAPI RCODE SessAddMedia(IN pSess,IN pSessMedia );
/* Remove media from sess object */
CCLAPI RCODE SessRemoveMedia(IN pSess,IN INT32 pos);
/* get media count from sess object */
CCLAPI RCODE SessGetMediaSize(IN pSess,OUT INT32*);

/*
 *	extended functions
 */
/* check connection and status of first audio media */
CCLAPI RCODE SessCheckHold(IN pSess,OUT HoldStyle*);
/* check connection and status of first audio media */
CCLAPI RCODE SessSetHold(IN pSess,IN HoldStyle);
CCLAPI RCODE SessSetUnHold(IN pSess,IN const char*);

#ifdef  __cplusplus
}
#endif

#endif

