/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tb.h
 *
 * $Id: sdp_tb.h,v 1.2 2008/08/12 02:53:07 tyhuang Exp $
 */

#ifndef SDP_TB_H
#define SDP_TB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Bandwidth, "b="
*/
#define SDP_TB_MODIFIER_NONE			0x0
#define SDP_TB_MODIFIER_CONFERENCE_TOTAL	0x1
#define SDP_TB_MODIFIER_APPLICATION_SPECIFIC	0x2		
#define SDP_TB_MODIFIER_EXTENSION		0x3

#define SDP_MAX_TB_EXTENTIONNAME_SIZE		32

#define SDP_TB_MAX_TOKEN_SIZE 8

typedef struct sdpTb {
	UINT16		modifier_;
	char		pExtentionName[SDP_MAX_TB_EXTENTIONNAME_SIZE];
	UINT32		bandwidth_;
} SdpTb;

CCLAPI RCODE		sdpTbParse(const char* bandwidthText, SdpTb*);
CCLAPI RCODE		sdpTb2Str(SdpTb*, char* strbuf, UINT32* buflen);
CCLAPI char*		sdpTbModifer2Str(UINT16 m);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TB_H */
