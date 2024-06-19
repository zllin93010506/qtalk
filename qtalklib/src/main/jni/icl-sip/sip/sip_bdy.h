/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 *
 * sip_bdy.h
 *
 * $Id: sip_bdy.h,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#ifndef SIP_BDY_H
#define SIP_BDY_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
	int			length;
	unsigned char*		content;
} SipBody;

#ifdef  __cplusplus
}
#endif

#endif /* SIP_BDY_H */