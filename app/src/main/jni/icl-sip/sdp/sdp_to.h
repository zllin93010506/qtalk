/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_to.h
 *
 * $Id: sdp_to.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#ifndef SDP_TO_H
#define SDP_TO_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Origin, "o="
*/

#define SDP_MAX_TO_USERNAME_SIZE	128
#define SDP_MAX_TO_ADDRESS_SIZE		128

typedef struct sdpTo {
	char		pUsername_[SDP_MAX_TO_USERNAME_SIZE];
	unsigned long	sessionID_;     /* should be a NTP timestamp */
	UINT32		version_;
	UINT16		networkType_;	/* see sdp_sess.h SDP_NETTYPE_XXX */
	UINT16		addressType_;	/* see sdp_sess.h SDP_ADDRTYPE_XXX */
	char		pAddress_[SDP_MAX_TO_ADDRESS_SIZE];
} SdpTo;

CCLAPI RCODE		sdpToParse(const char* originText, SdpTo*);
CCLAPI RCODE		sdpTo2Str(SdpTo*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TO_H */
