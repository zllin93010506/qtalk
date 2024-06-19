/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_cfg.h
 *
 * $Id: sip_cfg.h,v 1.4 2008/10/28 03:48:08 yanghc Exp $
 */

#ifndef SIP_CFG_H
#define SIP_CFG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "sip_cm.h"
#include "sip_tx.h"
#include <common/cm_def.h>


typedef struct {
	UINT16		tcp_port;	/* TCP/UDP listen port number */
	LogFlag		log_flag;
	TraceLevel	trace_level;	
	char		fname[64];	/* log file name */
	char		raddr[128];	/* log server IP address */
	UINT16		rport;		/* log server port */
	char		laddr[128];	/* local IP address */
	int		limithdrset;	/* flag for limiting set of header fields */ 
	UINT16		tls_port;	/* TLS listem port number ;0 is off */

#ifdef ICL_TLS
	char		caCert[128];    /* TLS certificate file */
	char		caKey[128];     /* TLS priviate key file */
	char		caKeyPwd[64]; /* Password for TLS private-key */
	char		caPath[128];    /* TLS trust-ca's path */
	char		enableTLS;	/*if the value of enableTLS is 'n' or 'N', then disable TLS  */
#endif /* ICL_TLS */
	
	BOOL		UseShortFormHeader;
} SipConfigData;

			/* initialize protocol stack(transport type TCP or UDP, 
						     TCP callback function, 
						     UDP callback function, 
						     configuration data) */
CCLAPI RCODE		sipLibInit(IN SipTransType, 
				   IN SipTCPEvtCB, 
				   IN SipUDPEvtCB, 
				   IN SipConfigData);

#ifdef ICL_TLS
CCLAPI RCODE		sipLibInitWithTLS(IN SipTransType, 
				   IN SipTCPEvtCB, 
				   IN SipUDPEvtCB,
				   IN SipTLSEvtCB,
				   IN SipConfigData);
#endif
			/* shutdown SIP protocol stack */
CCLAPI RCODE		sipLibClean(void); 

			/* config protocol stack */
			/* This API not implement now */
CCLAPI RCODE		sipConfig(IN SipConfigData*); 

			/*Generate Call-ID header*/
                        /*host: input host name, ip: input ip address */
			/* return the char pointer to Call-ID string .it is not thread-safe */
CCLAPI unsigned char*	sipCallIDGen(IN char* host, IN char* ip); 

CCLAPI RCODE CallIDGenerator(IN char* host,IN char* ipaddr,OUT char* buffer,IN OUT int* length);


#ifdef  __cplusplus
}
#endif

#endif /* SIP_CFG_H */
