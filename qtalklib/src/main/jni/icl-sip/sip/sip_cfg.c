/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_cfg.c
 *
 * $Id: sip_cfg.c,v 1.10 2009/03/11 07:53:49 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32_WCE /*WinCE didn't support time.h*/
#elif defined(_WIN32)
#include <time.h>
#include <sys/timeb.h>
#elif defined(UNIX)
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <low/cx_mutex.h>

#ifdef _WIN32_WCE_EVC3
#include <low_vc3/cx_sock.h>
#else
#include <low/cx_sock.h>
#endif

#include <low/cx_event.h>
#include <low/cx_timer.h>

#include <common/cm_trace.h>
#include "sip_cfg.h"
#include "sip_tx.h"
#include "sip_hdr.h"
#include "sip_int.h"
#include "rfc822.h"

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif

CxMutex CallIDGENMutex=NULL;
BOOL	g_sipUseShortForm;

extern CxEvent		g_sipEvt;
extern CxSock		g_sipTCPSrv;

#ifdef ICL_TLS

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CERTF   "root.pem"
#define KEYF    "root.pem"

extern CxSock		g_sipTLSSrv;
extern SipTLSEvtCB	g_sipTLSEvtCB;
extern SSL_CTX*		g_SERVERTLSCTX;
extern SSL_CTX*		g_CLIENTTLSCTX;

#endif /* ICL_TLS */

extern SipTransType	g_sipAppType;
extern SipTCPEvtCB	g_sipTCPEvtCB;
extern SipUDPEvtCB	g_sipUDPEvtCB;

extern char* g_TrustList;


void sipTCPServerAccept(CxSock sock, CxEventType event, int err, void* context);

#ifdef ICL_TLS
void sipTLSServerAccept(CxSock sock, CxEventType event, int err, void* context);
#endif /* ICL_TLS */

void sipInit(void) {} /* just for DLL forcelink */

void sipConfigInit(SipConfigData c)
{
	if( c.log_flag == LOG_FLAG_NONE )
		return;

	TCRBegin();

	TCRSetTraceLevel(c.trace_level);

	if(c.log_flag & LOG_FLAG_CONSOLE) 
		TCRConsoleLoggerON();

	if(c.log_flag & LOG_FLAG_FILE)
		TCRFileLoggerON(c.fname);

	if(c.log_flag & LOG_FLAG_SOCK) 
		TCRSockLoggerON(c.raddr, c.rport);
}

void sipConfigClean(void)
{
	TCREnd();
}

CCLAPI 
RCODE sipLibInit(SipTransType type, SipTCPEvtCB tcpEvtCB, SipUDPEvtCB udpEvtCB, SipConfigData config)
{
	int port;
	RCODE rc;


	if(config.tcp_port)
		port=config.tcp_port;
	else{
		port=SIP_TCP_DEFAULT_PORT; /*default TCP port number */
	}

	g_sipAppType = type;

	if (!tcpEvtCB || !udpEvtCB){
		printf("<sipLibInit>:TCP or UDP Callback function NULL! \n");
		return RC_ERROR;
	}else {
		g_sipTCPEvtCB = tcpEvtCB;
		g_sipUDPEvtCB = udpEvtCB;
	}

	if(1==config.UseShortFormHeader)
		g_sipUseShortForm = TRUE;
	else
		g_sipUseShortForm = FALSE;

	sipConfigInit(config);

	rc=cxSockInit();
	if(rc ==RC_ERROR){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:cxSockInit() error");
		return rc;
	}

	if(RC_ERROR==cxTimerInit(SIP_MAX_TIMER_COUNT)){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:cxTimerInit() error");
		return RC_ERROR;
	}

	g_sipEvt = cxEventNew();

	
	/* sipConfigInit(config); */

	config.laddr[15]='\0';
	TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:using local ip=%s\n",config.laddr);

	if (type & SIP_TRANS_TCP_SERVER) {
		CxSockAddr laddr=NULL;

		/* get server ipaddr & port from config file */
		if(config.laddr&&(strlen(config.laddr) == 0))
			laddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
		else {
			if (inet_addr((const char*)config.laddr)!=INADDR_NONE)
				laddr=cxSockAddrNew(config.laddr,(UINT16)port);
		}
		if(!laddr)
			laddr = cxSockAddrNew(NULL,(UINT16)port);

		g_sipTCPSrv = cxSockNew(CX_SOCK_SERVER, laddr);

		if(g_sipTCPSrv != NULL)
			rc=cxEventRegister(g_sipEvt, g_sipTCPSrv, CX_EVENT_RD, sipTCPServerAccept, NULL);
		else {
			TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>: csSockNew for server laddr error\n");
			rc=RC_ERROR;
		}

		cxSockAddrFree(laddr);
	}
	
	/*HdrSetFlag=(config.limithdrset!=1)?0:1;*/

	if( rc == RC_ERROR){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:SIP stack init failed!\n");
		sipLibClean();
		return RC_ERROR;
	}else{
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:SIP stack 1.7.2 is running!\n"); /* update by tyhuang 2009/3/6 */
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TCP port=%d\n",port);
		return RC_OK;
	}
}

#ifdef ICL_TLS

int passwd_cb( char *buf, int size, int flag, void *password)
{
        strncpy(buf, (char *)(password), size);
        buf[size - 1] = '\0';    
 
        return(strlen(buf));
}

#ifdef _DEBUG
void msg_cb(	int write_p, 
		int version, 
		int content_type, 
		const void *buf, 
		size_t len, 
		SSL *ssl, 
		void *arg )
/* arg: the value assigned in SSL_CTX_set_msg_callback_arg(), 
 *      usually the output file pointer. But, since we use TCRPrint(),
 *      we don't need to get output FILE pointer. 
 */
{
	FILE		*outFp = (FILE*)arg;
	const char 	*str_write_p, 
			*str_version, 
			*str_content_type = "", 
			*str_details1 = "", 
			*str_details2= "";
	
	str_write_p = write_p ? ">>>" : "<<<";

	switch (version)
		{
	case SSL2_VERSION:
		str_version = "SSL 2.0";
		break;
	case SSL3_VERSION:
		str_version = "SSL 3.0 ";
		break;
	case TLS1_VERSION:
		str_version = "TLS 1.0 ";
		break;
	default:
		str_version = "???";
		}

	if (version == SSL2_VERSION)
		{
		str_details1 = "???";

		if (len > 0)
			{
			switch (((unsigned char*)buf)[0])
				{
				case 0:
					str_details1 = ", ERROR:";
					str_details2 = " ???";
					if (len >= 3)
						{
						unsigned err = (((unsigned char*)buf)[1]<<8) + ((unsigned char*)buf)[2];
						
						switch (err)
							{
						case 0x0001:
							str_details2 = " NO-CIPHER-ERROR";
							break;
						case 0x0002:
							str_details2 = " NO-CERTIFICATE-ERROR";
							break;
						case 0x0004:
							str_details2 = " BAD-CERTIFICATE-ERROR";
							break;
						case 0x0006:
							str_details2 = " UNSUPPORTED-CERTIFICATE-TYPE-ERROR";
							break;
							}
						}

					break;
				case 1:
					str_details1 = ", CLIENT-HELLO";
					break;
				case 2:
					str_details1 = ", CLIENT-MASTER-KEY";
					break;
				case 3:
					str_details1 = ", CLIENT-FINISHED";
					break;
				case 4:
					str_details1 = ", SERVER-HELLO";
					break;
				case 5:
					str_details1 = ", SERVER-VERIFY";
					break;
				case 6:
					str_details1 = ", SERVER-FINISHED";
					break;
				case 7:
					str_details1 = ", REQUEST-CERTIFICATE";
					break;
				case 8:
					str_details1 = ", CLIENT-CERTIFICATE";
					break;
				}
			}
		}

	if (version == SSL3_VERSION || version == TLS1_VERSION)
		{
		switch (content_type)
			{
		case 20:
			str_content_type = "ChangeCipherSpec";
			break;
		case 21:
			str_content_type = "Alert";
			break;
		case 22:
			str_content_type = "Handshake";
			break;
			}

		if (content_type == 21) /* Alert */
			{
			str_details1 = ", ???";
			
			if (len == 2)
				{
				switch (((unsigned char*)buf)[0])
					{
				case 1:
					str_details1 = ", warning";
					break;
				case 2:
					str_details1 = ", fatal";
					break;
					}

				str_details2 = " ???";
				switch (((unsigned char*)buf)[1])
					{
				case 0:
					str_details2 = " close_notify";
					break;
				case 10:
					str_details2 = " unexpected_message";
					break;
				case 20:
					str_details2 = " bad_record_mac";
					break;
				case 21:
					str_details2 = " decryption_failed";
					break;
				case 22:
					str_details2 = " record_overflow";
					break;
				case 30:
					str_details2 = " decompression_failure";
					break;
				case 40:
					str_details2 = " handshake_failure";
					break;
				case 42:
					str_details2 = " bad_certificate";
					break;
				case 43:
					str_details2 = " unsupported_certificate";
					break;
				case 44:
					str_details2 = " certificate_revoked";
					break;
				case 45:
					str_details2 = " certificate_expired";
					break;
				case 46:
					str_details2 = " certificate_unknown";
					break;
				case 47:
					str_details2 = " illegal_parameter";
					break;
				case 48:
					str_details2 = " unknown_ca";
					break;
				case 49:
					str_details2 = " access_denied";
					break;
				case 50:
					str_details2 = " decode_error";
					break;
				case 51:
					str_details2 = " decrypt_error";
					break;
				case 60:
					str_details2 = " export_restriction";
					break;
				case 70:
					str_details2 = " protocol_version";
					break;
				case 71:
					str_details2 = " insufficient_security";
					break;
				case 80:
					str_details2 = " internal_error";
					break;
				case 90:
					str_details2 = " user_canceled";
					break;
				case 100:
					str_details2 = " no_renegotiation";
					break;
					}
				}
			}
		
		if (content_type == 22) /* Handshake */
			{
			str_details1 = "???";

			if (len > 0)
				{
				switch (((unsigned char*)buf)[0])
					{
				case 0:
					str_details1 = ", HelloRequest";
					break;
				case 1:
					str_details1 = ", ClientHello";
					break;
				case 2:
					str_details1 = ", ServerHello";
					break;
				case 11:
					str_details1 = ", Certificate";
					break;
				case 12:
					str_details1 = ", ServerKeyExchange";
					break;
				case 13:
					str_details1 = ", CertificateRequest";
					break;
				case 14:
					str_details1 = ", ServerHelloDone";
					break;
				case 15:
					str_details1 = ", CertificateVerify";
					break;
				case 16:
					str_details1 = ", ClientKeyExchange";
					break;
				case 20:
					str_details1 = ", Finished";
					break;
					}
				}
			}
		}

	fprintf(outFp, "%s %s%s [length %04lx]%s%s\n", 
		str_write_p, str_version, str_content_type, 
		(unsigned long)len, str_details1, str_details2);
	/*
	TCRPrint( TRACE_LEVEL_API, "%s %s%s [length %04lx]%s%s\n", 
		str_write_p, str_version, str_content_type, 
		(unsigned long)len, str_details1, str_details2);
	*/


	if (len > 0) {
		size_t num, i;
		fprintf(outFp, "   ");
		/*
		TCRPrint( TRACE_LEVEL_API, "   ");
		*/
		num = len;
#if 0
		if (num > 16)
			num = 16;
#endif
		for (i = 0; i < num; i++)
			{
			if (i % 16 == 0 && i > 0){
				fprintf(outFp, "\n   ");
				/*
				TCRPrint( TRACE_LEVEL_API, "\n   ");
				*/
			}
			fprintf(outFp, " %02x", ((unsigned char*)buf)[i]);
			/*
			TCRPrint( TRACE_LEVEL_API, " %02x", ((unsigned char*)buf)[i]);
			*/
			}
		if (i < len){
			fprintf(outFp, " ...");
			/*
			TCRPrint( TRACE_LEVEL_API, " ...");
			*/
		}
		fprintf(outFp, "\n");
		/*
		TCRPrint( TRACE_LEVEL_API, "\n");
		*/
	}
}
#endif /*end of _DEBUG */

CCLAPI 
RCODE sipLibInitWithTLS(IN SipTransType type, 
				   IN SipTCPEvtCB tcpEvtCB, 
				   IN SipUDPEvtCB udpEvtCB,
				   IN SipTLSEvtCB tlsEvtCB,
				   IN SipConfigData config)
{
	int port, tlsport=0;
	RCODE rc=RC_OK;
	char	enableTLS='N';

	sipConfigInit(config);

	if(config.tcp_port)
		port=config.tcp_port;
	else
		port=SIP_TCP_DEFAULT_PORT; /*default TCP port number */

	if(config.enableTLS != 'n' && config.enableTLS != 'N'){
		enableTLS='Y';
		if( config.tls_port )
			tlsport=config.tls_port;
		else
			tlsport=port+1;
		
		if(port==tlsport){
			TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:TCP and TLS listen port conflit! \n");
			return RC_ERROR;
		}
	}

	g_sipAppType = type;

	if (!tcpEvtCB || !udpEvtCB){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:TCP or UDP Callback function NULL! \n");
		return RC_ERROR;
	}else {
		g_sipTCPEvtCB = tcpEvtCB;
		g_sipUDPEvtCB = udpEvtCB;
	}

	rc=cxSockInit();
	if(rc ==RC_ERROR) 
		return rc;

	if(RC_ERROR==cxTimerInit(SIP_MAX_TIMER_COUNT))
		return RC_ERROR;

	g_sipEvt = cxEventNew();

	config.laddr[15]='\0';
	if (type & SIP_TRANS_TCP_SERVER) {
		CxSockAddr laddr=NULL;

		/* get server ipaddr & port from configfile */
		if(config.laddr&&(strlen(config.laddr) == 0))
			laddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
		else {
			if (inet_addr(config.laddr)!=-1)
				laddr=cxSockAddrNew(config.laddr,(UINT16)port);
		}
		if(!laddr){
			laddr = cxSockAddrNew(NULL,(UINT16)port);
		}

		g_sipTCPSrv = cxSockNew(CX_SOCK_SERVER, laddr);
		if( g_sipTCPSrv != NULL )
			rc=cxEventRegister(g_sipEvt, g_sipTCPSrv, CX_EVENT_RD, sipTCPServerAccept, NULL);
		else {
			TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>: csSockNew for server laddr error\n");
			rc=RC_ERROR;

		}
		cxSockAddrFree(laddr);
		if( rc == RC_ERROR){
			TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:SIP stack init failed!\n");
			sipLibClean();
			return RC_ERROR;
		}
	}
	
	/* if tlsEvtCB is not NULL */
	if( enableTLS != 'N' && tlsEvtCB ) {
		g_sipTLSEvtCB = tlsEvtCB;

		if (type & SIP_TRANS_TLS_SERVER) {
			
			CxSockAddr laddr=NULL;
			SSL_CTX         *ctx,*clctx;
			
			/* get server ipaddr & port from configfile */
			if(config.laddr&&(strlen(config.laddr) == 0))
				laddr = cxSockAddrNew(NULL, (UINT16)tlsport); /* auto-fill my ip */
			else{
				if ((!laddr)&&(inet_addr(config.laddr)!=INADDR_NONE))
					laddr=cxSockAddrNew(config.laddr,(UINT16)tlsport);
			}
			if(!laddr){
				laddr = cxSockAddrNew(NULL,(UINT16)tlsport);
			}

			SSL_library_init();
			SSL_load_error_strings();
			/*
			clctx = SSL_CTX_new (TLSv1_client_method());
			ctx = SSL_CTX_new (TLSv1_server_method());
			*/
			clctx = SSL_CTX_new (TLSv1_method());
			ctx = SSL_CTX_new (TLSv1_method());
			
			if(!ctx || !clctx ){
				TCRPrint(TRACE_LEVEL_ERROR,"new SSL_CTX is failed\n ");		
			}else{
				/* set client ctx*/
				/* SSL_CTX_set_cipher_list(clctx,"ALL:!ADH:!AES256:+RC4:@STRENGTH");*/
				SSL_CTX_set_default_passwd_cb_userdata(clctx, (void *)config.caKeyPwd); 
				SSL_CTX_set_default_passwd_cb(clctx, passwd_cb);                       
				if(SSL_CTX_use_certificate_file(clctx, config.caCert, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_certificate_file is failed\n ");
					SSL_CTX_free(clctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_use_PrivateKey_file(clctx, config.caKey, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_PrivateKey_file is failed\n ");
					SSL_CTX_free(clctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_check_private_key(clctx)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX_check_private_key is failed\n ");
					SSL_CTX_free(clctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_load_verify_locations(clctx,config.caPath,NULL)<1)
					TCRPrint(TRACE_LEVEL_ERROR,"set trusted certificate path failed\n ");
				SSL_CTX_set_verify(clctx, SSL_VERIFY_PEER,NULL);

                                #ifdef _DEBUG
                                SSL_CTX_set_msg_callback(clctx, msg_cb );
                                SSL_CTX_set_msg_callback_arg(clctx, stdout);
				#endif

				g_CLIENTTLSCTX=clctx;

				/* set server ctx parameter */
				SSL_CTX_set_default_passwd_cb_userdata(ctx, (void *)config.caKeyPwd); 
				SSL_CTX_set_default_passwd_cb(ctx, passwd_cb);                       
				
				if(SSL_CTX_use_certificate_file(ctx, config.caCert, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_certificate_file is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_use_PrivateKey_file(ctx, config.caKey, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_PrivateKey_file is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_check_private_key(ctx)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX_check_private_key is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_load_verify_locations(ctx,config.caPath,NULL)<1)
					TCRPrint(TRACE_LEVEL_ERROR,"set trusted certificate path failed\n ");
				SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE,NULL);
                                #ifdef _DEBUG
                                SSL_CTX_set_msg_callback(ctx, msg_cb );
                                SSL_CTX_set_msg_callback_arg(ctx, stdout);
				#endif


				g_SERVERTLSCTX = ctx;
				
				g_sipTLSSrv = cxSockNew(CX_SOCK_SERVER, laddr);
				if( g_sipTLSSrv != NULL )
					rc|=cxEventRegister(g_sipEvt, g_sipTLSSrv, CX_EVENT_RD, sipTLSServerAccept, NULL);
				else {
					TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>: csSockNew for server TLS-laddr error\n");
					rc=RC_ERROR;

				}

				cxSockAddrFree(laddr);
			}

		}
		if( rc == RC_ERROR){
			TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:SIP stack init failed!\n");
			sipLibClean();
			return RC_ERROR;
		}
	}/* end of tls */


	/*HdrSetFlag=(config.limithdrset!=1)?0:1;*/


	if (!CallIDGENMutex) 
		CallIDGENMutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);

	TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:SIP stack 1.7.0 is running!\n");
	TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TCP port=%d\n",port);
	if( enableTLS != 'N' ) 
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TLS port=%d\n",tlsport);

	return RC_OK;
}

#endif /* ICL_TLS */

CCLAPI 
RCODE sipLibClean()
{
	void* context = NULL;
	RCODE rcode=RC_OK;

	if (g_sipTCPSrv) {
		cxEventUnregister(g_sipEvt, g_sipTCPSrv, &context);
		cxSockFree(g_sipTCPSrv);
		if( context )
			free(context);

		g_sipTCPSrv = NULL;
	}

#ifdef ICL_TLS
	if(g_SERVERTLSCTX){ 
		cxEventUnregister(g_sipEvt, g_sipTLSSrv,  &context);
		cxSockFree(g_sipTLSSrv);
		SSL_CTX_free(g_SERVERTLSCTX);
		g_SERVERTLSCTX = NULL;
	}

	if( g_CLIENTTLSCTX ){
		SSL_CTX_free(g_CLIENTTLSCTX);
		g_CLIENTTLSCTX=NULL;
	}
#endif /* ICL_TLS */

	rcode|=cxEventFree(g_sipEvt);
	g_sipEvt = NULL;

	rcode|=cxSockClean();

	rcode|=cxTimerClean();

	sipConfigClean();

	if (CallIDGENMutex) {
		cxMutexFree(CallIDGENMutex);
		CallIDGENMutex = NULL;
	}

	if(g_TrustList!=NULL){
		free(g_TrustList);
		g_TrustList=NULL;
	}

	return rcode;
}

CCLAPI 
RCODE sipConfig(SipConfigData* c)
{

	return RC_OK;
}

int rchar(void)
{
	return (rand()%239+17);
}

int rint(int yy)
{
	return (yy%239+17);
}
int ipResol(char *ip,char hex[10])
{
	char seps[]= ".";
	char *token,buf[3];
	int count=0;
	
	hex[0]='\0';
	token=strtok(ip,seps);
	while(token!= NULL){
		count++;
		sprintf(buf,"%X",rint(atoi(token)));
		strcat(hex,buf);
		if(count%4 == 2) strcat(hex,"-");
		token=strtok(NULL,seps);
	}
	return RC_OK;
}

CCLAPI
unsigned char* sipCallIDGen(char* host,char* ipaddr)
{
	static unsigned char callidstr[128]={'\0'};
	static unsigned int  gencount;
	char ipbuf[12]={'\0'},buf[64]={'\0'},tmpip[20]={'\0'};
	int pos;

#ifdef _WIN32
	unsigned long ltime;
	ltime=(unsigned long) GetTickCount();

#elif defined(UNIX)
	unsigned long ltime;
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(unsigned long) (tv.tv_sec * 1000 + tv.tv_usec / 1000);

#else
	int ltime;
	ltime=rand();
#endif
	if(gencount==0)
		gencount++;

	if(ipaddr!=NULL){
		/* copy ip to buffer */
		memset(tmpip,0,sizeof tmpip);
		strncpy(tmpip,ipaddr,19);
		tmpip[19] = 0;

		ipResol(tmpip,ipbuf);
		
		pos=sprintf(buf,"%lu-",ltime);
/*	for mtu
		sprintf(buf+pos,"%X%X%X%X%X%X",rchar(),rchar(),rchar(),rchar(),rchar(),rchar());
		sprintf((char*)callidstr,"%s-%s-%04d",ipbuf,buf,gencount);

		sprintf((char*)callidstr,"%lu-%04d",ltime,gencount);
*/
		sprintf(buf+pos,"%X%X",rchar(),rchar());
		sprintf((char*)callidstr,"%s-%04d",buf,gencount++);
		/*gencount=(gencount+1)%60000;*/
		/* add 2003-03-06 by tyhuang */
		if(host){
			if(strlen(host)>0){
				strcat(callidstr,"@");
				strcat(callidstr,host);
			}
		}else{
			if(strlen(ipaddr)>0){
				strcat(callidstr,"@");
				strcat(callidstr,ipaddr);
			}
		}

		
	}else{
		callidstr[0]='\0';
	}
	return callidstr;
} /* sipCallIdGenerate */


CCLAPI
RCODE CallIDGenerator(IN char* host,IN char* ipaddr,OUT char* buffer,IN OUT int* length)
{
	char callidstr[256]={'\0'};
	static unsigned int  gencount;
	char buf[128]={'\0'};
	int pos;

#ifdef _WIN32
	unsigned long ltime;
	ltime=(unsigned long) GetTickCount();

#elif defined(UNIX)
	unsigned long ltime;
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(unsigned long) (tv.tv_sec * 1000 + tv.tv_usec / 1000);

#else
	int ltime;
	ltime=rand();
#endif
	if(gencount==0)
			gencount++;

	if ((NULL==buffer)||(NULL==length)||(*length<1)) return RC_ERROR;

	if(ipaddr||host){

		pos=sprintf(buf,"%lu-",ltime);
		sprintf(buf+pos,"%X%X",rchar(),rchar());
		
		cxMutexLock(CallIDGENMutex);
		
		sprintf((char*)callidstr,"%s-%04d",buf,gencount);
		gencount=(gencount+1)%60000;
		cxMutexUnlock(CallIDGENMutex);
		
		/* add 2003-03-06 by tyhuang */
		if(host){
			if(strlen(host)>0){
				strcat(callidstr,"@");
				strcat(callidstr,host);
			}
		}else{
			if(strlen(ipaddr)>0){
				strcat(callidstr,"@");
				strcat(callidstr,ipaddr);
			}
		}

	}else{
		callidstr[0]='\0';
	}
	
	strncpy(buffer,callidstr,*length);
	return RC_OK;
} /* sipCallIdGenerate */



