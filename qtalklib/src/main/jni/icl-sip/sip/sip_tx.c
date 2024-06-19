/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_tx.c
 *
 * $Id: sip_tx.c,v 1.8 2009/01/16 09:05:05 tyhuang Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

#ifdef _WIN32_WCE_EVC3
#include <low_vc3/cx_sock.h>
#else
#include <low/cx_sock.h>
#endif

#include <low/cx_event.h>
#include <low/cx_timer.h>
#include <common/cm_trace.h>
#include <common/cm_utl.h>
#include <low/cx_misc.h>

#include "sip_tx.h"
#include "rfc822.h"
#include "sip_int.h"

#ifdef ICL_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif /* ICL_TLS */

typedef struct  
{
	SipTCP			tcp;
	SipUDP			udp;
	SipTLS			tls;
	int			tcptype;/* 0=client, 1=server */

	char			*msg;
	int			msgLen; /* current size of msg */
	int			leftBodyLen; /* -1, non-completed header ;
						>=0,  the left size of body to receive */
	SipMsgType		msgtype;
} sipEvtCBParam;

struct sipTCPObj
{
	CxSock			sock_;
	sipEvtCBParam*		ptrTCPPara;
};

struct sipUDPObj
{
	CxSock			sock_;
	sipEvtCBParam*		ptrUDPPara;
	unsigned short		refcount;
};

CxEvent				g_sipEvt=NULL; 
CxSock				g_sipTCPSrv=NULL; 
CxSock				g_sipTLSSrv=NULL;

SipTransType		g_sipAppType = SIP_TRANS_UNKN;
SipTCPEvtCB			g_sipTCPEvtCB = NULL; 
SipUDPEvtCB			g_sipUDPEvtCB = NULL;
SipTLSEvtCB			g_sipTLSEvtCB = NULL;

char	*g_TrustList = NULL;

#ifdef ICL_TLS
struct sipTLSObj
{
	CxSock				sock_;
	SSL*				ssl;
	sipEvtCBParam*		ptrTLSPara;
};

SSL_CTX*			g_SERVERTLSCTX = NULL;
SSL_CTX*			g_CLIENTTLSCTX = NULL;

#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
SipTLS sipTLSServerNew(CxSock sock);

#endif /* ICL_TLS */

#define SIP_TX_MAXBUFLEN		65536 /*Acer Modified SipIt 10th*/ /*original 4096*/
#define SIP_INFINITE_CONT_LEN		100000000
/*#define EOF -1 */

SipTCP sipTCPServerNew(CxSock sock);

int getc_socket(CxSock f) 
{
	char cstr[512];
	char c;
	int ret;
/* Fun: cxSockRecvEx
 * Ret: Return number of byets actually received when success, 
 *      0, connection closed.
 *      -1, error, 
 *      -2, timeout.
 */
	ret=cxSockRecvEx(f, cstr, 1,100);
	if (ret >0 ) {
		memcpy(&c, cstr, 1);
		return c;
	}else if(ret == -2){ /* Time out */
		return SIP_SOCKET_TIMEOUT;
	}else{		   /* socket error & connection closed */
		return EOF;
	}
}


static void initCBParamMsginfo(sipEvtCBParam* param)
{

	param->msg = NULL;
	param->msgLen = 0;
	param->leftBodyLen = -1;
	param->msgtype = SIP_MSG_UNKNOWN;
}

static void resetCBParamMsginfo(sipEvtCBParam* param)
{
	if( param->msg )
		free(param->msg);
	initCBParamMsginfo(param);
}


#define PROTOCOT_TCP	1
#define PROTOCOT_UDP	2
#define PROTOCOT_TLS	3

static void procASipMsg(CxSock sock, sipEvtCBParam* param, int ptype, SipMsgType msgtype, char *sipmsg)
{
	void*	message;

	 if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_UNKNOWN){ 
		message = sipReqNewFromTxt(sipmsg);
		/* print request after parse.added by tyhuang 2004/7/15 */
		#ifdef _DEBUG
		TCRPrint (TRACE_LEVEL_API, "[sipRev request]\n%s\n",sipReqPrint(message));
		#endif
	 }else if (msgtype == SIP_MSG_RSP){ 
		message = sipRspNewFromTxt(sipmsg);
		/* print response after parse.added by tyhuang 2004/7/15 */
		#ifdef _DEBUG
		TCRPrint (TRACE_LEVEL_API, "[sipRev response]\n%s\n",sipRspPrint(message));
		#endif
	 }else
		message = NULL;		

	 /* set received and rport if socket address is not equal to sent-by */
	 if (message && (msgtype == SIP_MSG_REQ)){
		 /* add received and rport */
		 char* viasent = NULL;
		 char* viasentport = NULL;
		 char* colon=NULL; /* for ipv6 */
		 UINT16 sentport=5060;
		 CxSockAddr raddr=cxSockGetRaddr(sock);
		 char* socksource = cxSockAddrGetAddr(raddr);
		 UINT16 sockport = cxSockAddrGetPort(raddr);
		 SipVia *viahdr = (SipVia*)sipReqGetHdr((SipReq)message,Via);
		 
		 if (viahdr && viahdr->ParmList){ 
			 viasent=trimWS(strDup(viahdr->ParmList->sentby));
			 
			 colon = strChr((unsigned char*)viasent,']');
			 if(colon!=NULL)
				 viasentport=strChr((unsigned char*)colon,':');
			 else
				 viasentport = strChr((unsigned char*)viasent,':');
			 
			 if(viasentport){
				 *viasentport++='\0';
				 sentport=a2i(viasentport);
			 }
		 }
		 
								
		 if (strCmp(viasent,socksource)!=0){
			if (viahdr && viahdr->ParmList){
				if(viahdr->ParmList->received)
					free(viahdr->ParmList->received);
				
				viahdr->ParmList->received = strDup( socksource );
			}
		}
		
		
		if(sentport!=sockport){
			if(viahdr && viahdr->ParmList){
				viahdr->ParmList->rport = (int)sockport;
			}
		}
		 
		 if(viasent) free(viasent);
	}

	if(message){
		switch(ptype){
		    case PROTOCOT_TCP:
			g_sipTCPEvtCB(param->tcp,SIP_TCPEVT_DATA, msgtype,(void*)message);
			break;
		    case PROTOCOT_TLS:
			g_sipTLSEvtCB(param->tls,SIP_TLSEVT_DATA, msgtype,(void*)message);
			break;
		    default:
			g_sipUDPEvtCB(param->udp,SIP_UDPEVT_DATA, msgtype,(void*)message); 
			break;
		}
	} else {
		switch(ptype){
		    case PROTOCOT_TCP:
			g_sipTCPEvtCB(param->tcp,SIP_TCPEVT_DATA, msgtype, NULL);
			break;
		    case PROTOCOT_TLS:
			g_sipTLSEvtCB(param->tls,SIP_TLSEVT_DATA, msgtype, NULL);
			break;
		    default:
			g_sipUDPEvtCB(param->udp,SIP_UDPEVT_DATA, msgtype, NULL);
			break;
		}
		TCRPrint(TRACE_LEVEL_API, "[sipRev response]receive non-SIP message\n");
	}
}


/* check if the input string is a SIP request line, if it is, return some information if need*/
int isSIPReqLine(char *p_str, SipMethodType *r_method, char **r_ver, char **r_reqUri)
{
	int		i;
	char		*sp, *cp, ch, *reqUri=NULL, *version=NULL;
	SipMethodType	iMethod;
	char		buff[5];

	if( p_str == NULL )
		return 0; /*FALSE*/

	sp = p_str;

	/* 1. check method */
	if( (cp=strChr((unsigned char*)sp, ' ')) == NULL )
		return 0;
	*cp = '\0';
	if( (iMethod=sipMethodNameToType(sp)) == UNKNOWN_METHOD ){
		*cp = ' ';
		return 0; /*FALSE*/
	}
	if( r_method != NULL )
		*r_method = iMethod;
	*cp = ' ';
	sp = cp + 1;

	/* 2. check request-uri */
	if( (cp=strChr((unsigned char*)sp, ' ')) == NULL )
		return 0; /*FALSE*/
	*cp = '\0';
	if( *sp == '\0' ){
		*cp = ' ';
		return 0; /*FALSE*/
	}
	if( r_reqUri != NULL )
		reqUri = strDup(sp);
	*cp = ' ';
	sp = cp + 1;

	/* 3. check sip-version: SIP-Version string is case-insensitive */
	memset(buff,0,5);
	i=0;
	while(i<4 && sp[i] ){
		buff[i] = toupper(sp[i]);
		i++;
	}
	if( strncmp(buff, "SIP/", 4 * sizeof(char) ) != 0 ){
		if( reqUri != NULL )
			free(reqUri);
		return 0; /*FALSE*/
	}
	sp += 4;
	cp = sp;
	while( *cp && (isdigit(*cp) || (*cp == '.')) )
		cp ++;
	ch = *cp;
	*cp = '\0';
	if( *sp == '\0' ){
		*cp = ch;
		if( reqUri != NULL )
			free(reqUri);
		return 0; /*FALSE*/
	}
	if( r_ver != NULL )
		version= strDup(sp);
	*cp = ch;

	/* check if there exists garbage data */
	while( *cp && isspace(*cp) )
		cp ++;
	if(  strlen(cp) != 0 ){
		if( reqUri != NULL )
			free(reqUri);
		if( version != NULL )
			free(version);
		return 0; /*FALSE*/
	}

	if( r_reqUri != NULL )
		*r_reqUri = reqUri;
	if( r_ver != NULL )
		*r_ver = version;
	return 1; /*TRUE*/
}



/* check if the input string is a SIP response line, if it is, return some information if need*/
int isSIPRespLine(char *p_str, char **r_ver, int *r_status, char **r_reason)
{
	int	i;
	char	*sp, *cp, *dp, ch, *reason=NULL, *version=NULL;
	char	buff[5];

	if( p_str == NULL )
		return 0; /*FALSE*/

	sp = p_str;

	/* 1. check sip-version: SIP-Version string is case-insensitive */
	if( (cp=strChr((unsigned char*)sp, ' ')) == NULL )
		return 0; /*FALSE*/
	*cp = '\0';

	memset(buff,0,5);
	i=0;
	while(i<4 && sp[i] ){
		buff[i] = toupper(sp[i]);
		i++;
	}
	if( strncmp(buff, "SIP/", 4 ) != 0 ){
		*cp = ' ';
		return 0; /*FALSE*/
	}

	sp += 4;
	if( *sp == '\0' ){
		*cp = ' ';
		return 0; /*FALSE*/
	}
	dp = sp;
	while( *dp ){
		if( !isdigit(*dp) && (*dp != '.') ){
			*cp = ' ';
			return 0; /*FALSE*/
		}
		dp ++;
	}
	if( r_ver != NULL )
		version = strDup(sp);
	*cp = ' ';
	sp = cp + 1;

	/* 2. check status-code */
	if( (cp=strChr((unsigned char*)sp, ' ')) == NULL )
		return 0; /*FALSE*/
	*cp = '\0';
	if( strlen(sp) != 3 ){
		if( version != NULL )
			free(version);
		*cp = ' ';
		return 0; /*FALSE*/
	}
	dp = sp;
	while( *dp ){
		if( !isdigit(*dp) ){
			if( version != NULL )
				free(version);
			*cp = ' ';
			return 0; /*FALSE*/
		}
		dp ++;
	}

	if( r_status!= NULL )
		*r_status = atoi(sp);
	*cp = ' ';
	sp = cp + 1;

	/* 3. get reason-phrase */
	cp = &sp[strlen(sp)-1];

	while( (cp >= sp) && isspace(*cp) )
		cp --;

	if( cp < sp ){
		if( version != NULL )
			free(version);
		return 0; /*FALSE*/
	}
	if( r_reason != NULL ){
		cp++;
		ch = *cp;
		*cp = '\0';
		reason = strDup(sp);
		*cp = ch;
	}

	if( r_ver != NULL )
		*r_ver = version;
	if( r_reason != NULL )
		*r_reason = reason;
	return 1; /*TRUE*/
}


static RCODE processInData(CxSock sock, char *p_str, sipEvtCBParam *p_param, int p_ptype, char **r_leftptr )
{
        int             n, k, totn, r1, r2;
        char		*sp, *cp, *bufp, *endp;
	char		ch;
        MsgHdr          hdr, tmp;

	bufp = p_str;
	*r_leftptr = NULL;

	while( *bufp ){
		if( p_param->msgLen>0 && p_param->leftBodyLen >= 0 ){
			if( p_param->leftBodyLen > 0 ){
				/* There exist msg header and partial msg of body that 
				 * have received last. Now, just need to receive left 
				 * messages of body.
			 	 */ 
				k = strlen(bufp);
				n = k > p_param->leftBodyLen ? p_param->leftBodyLen:k;
				totn = p_param->msgLen + n; 
				p_param->msg=realloc(p_param->msg, totn+1);
				if( p_param->msg == NULL ){
					/* log error here */
					resetCBParamMsginfo(p_param);
					return RC_ERROR;
				}
				sp = p_param->msg + p_param->msgLen;
				memcpy(sp, bufp, n);
				sp[n] = 0;
				bufp += n;
				p_param->msgLen += n;
				p_param->leftBodyLen -= n;
			}
			if( p_param->leftBodyLen == 0 ){ 
				/* a whole msg has been received */
				#ifdef _DEBUG
				/* print buffer from cx.added by tyhuang 2004/7/15 */
				TCRPrint (TRACE_LEVEL_API, "[Rev from TCP]\n%s\n",p_param->msg);
				#endif
				procASipMsg(sock, p_param, p_ptype, p_param->msgtype, p_param->msg);
				resetCBParamMsginfo(p_param);
			}
			/* continue to wait for receive other messages of body */
			
		} else { /* p_param->msgLen == 0 || p_param->leftBodyLen == -1  */

			/* Get header part of SIP message.
		 	 * If msgLen is 0, that means a new SIP message is expected.
			 * So, need to check if the input data is the first line of a SIP
 			 * msg. If not, skip those data.
			 */ 
			if( p_param->msgLen == 0 ){
				/* check the first line of SIP message */

				/* 1. search the first CRLF, 
				 *    if not found, return to receive other data in 
				 *    socket buffer. (If no more data in socket buffer, 
				 *    then discard these data.)
				 */ 
				cp = strChr((unsigned char*)bufp,'\n');		
				if( cp == NULL ){
					if( strlen(bufp) > 0 )
						*r_leftptr = bufp;
					return RC_OK; /* continue to read from socket */
				}

				/* 2. find the start byte of first line  */
				ch = *cp;
				*cp = '\0';
				r1=r2=0;
				while( *bufp ){
					if((r1=isSIPReqLine(bufp,NULL,NULL,NULL)) ||
				           (r2=isSIPRespLine(bufp,NULL,NULL,NULL)) ){
						break;
					}
					bufp++;
				}
				*cp = ch;
				if( !r1 && !r2 ){
					bufp=cp+1;
					continue;
				} 
				if( r1 )
					p_param->msgtype = SIP_MSG_REQ;
				else 
					p_param->msgtype = SIP_MSG_RSP;

				/* now, *bufp is the first byte of the first line */
				p_param->msg = NULL;
			}
			/* search the end of the SIP header */
			endp = NULL;
			if( p_param->msgLen > 0 && p_param->msg != NULL ){
				if( p_param->msg[p_param->msgLen-1] == '\n' ){
					switch(*bufp){
					   case '\n':
						r1 = 1;
						endp = bufp;
						break;
					   case '\r':
						if( *(bufp+1) == '\n' )
							endp = bufp + 1;
						break;
					   default: 
						break;
					}
				} else if( p_param->msg[p_param->msgLen-2] == '\n' &&
				           p_param->msg[p_param->msgLen-1] == '\r' ){
					if( *bufp == '\n' )
						endp = bufp;
				}
			}
			if( !endp ){ 
				sp = bufp;
				while( !endp && (cp=strChr((unsigned char*)sp,'\n')) != NULL ){
					cp++;
					switch(*cp){
					   case '\n':
						r1 = 1;
						endp = cp;
						break;
					   case '\r':
						if( *(cp+1) == '\n' ){
							endp = cp + 1;
						} else {
							sp = cp;
						}
						break;
					   default: 
						sp = cp;
						break;
					}
				}
			}

			if( endp ){
				/* a whole SIP header has received now */
				n = endp - bufp + 1;
				totn = p_param->msgLen + n; 
				p_param->msg=realloc(p_param->msg, totn+1);
				if( p_param->msg == NULL ){
					/* log error here */
					resetCBParamMsginfo(p_param);
					return RC_ERROR;
				}
				cp = p_param->msg + p_param->msgLen;
				memcpy(cp, bufp, n);
				cp[n] = 0;
				bufp += n;
				p_param->msgLen += n;
	
				/* Get content message length from header.
				 * If error occurs, just discard the data that has been
				 * received. (Is there other suggestion? )
				 */
	
				if( (hdr=msgHdrNew()) == NULL ){
					resetCBParamMsginfo(p_param);
					return RC_ERROR;
				}

				/* find and drop start line */
				if( (cp=strChr((unsigned char*)p_param->msg,'\n')) == NULL ){
					resetCBParamMsginfo(p_param);
					msgHdrFree(hdr);
					return RC_ERROR;
				}
				cp++;
				if( (sp = strDup(cp)) == NULL ){
					resetCBParamMsginfo(p_param);
					msgHdrFree(hdr);
					return RC_ERROR;
				}
				rfc822_parse((unsigned char*)sp, &n, hdr);
				tmp = hdr;
				p_param->leftBodyLen = 0;
				while( tmp != NULL ){
					if( CheckHeaderName(tmp, Content_Length) ||
					    CheckHeaderName(tmp, Content_Length_Short)){
						cp = NULL;
						ContentLengthParse(tmp, (void**) &cp);
						if( !cp ){
							resetCBParamMsginfo(p_param);
							msgHdrFree(hdr);
							free(sp);
							return RC_ERROR;
						}
						p_param->leftBodyLen= *(int *)cp;
						free(cp);
						break;
					}
					tmp = tmp->next;	
				}
				msgHdrFree(hdr);
				free(sp);
				if( p_param->leftBodyLen < 0 )
					p_param->leftBodyLen = 0;
	
				if( p_param->leftBodyLen == 0){
					/* a whole msg has been received */
					#ifdef _DEBUG
					/* print buffer from cx.added by tyhuang 2004/7/15 */
					TCRPrint(TRACE_LEVEL_API,"[Rev from TCP]\n%s\n",p_param->msg);
					#endif
					procASipMsg(sock, p_param,p_ptype,p_param->msgtype,p_param->msg);
					resetCBParamMsginfo(p_param);
				}
				/* continue to processs the body part */
			} else {
				n = strlen(bufp);
				totn = p_param->msgLen + n; 
				p_param->msg=realloc(p_param->msg, totn+1);
				if( p_param->msg == NULL ){
					/* log error here */
					resetCBParamMsginfo(p_param);
					return RC_ERROR;
				}
				cp = p_param->msg + p_param->msgLen;
				memcpy(cp, bufp, n);
				cp[n] = 0;
				bufp += n;
				p_param->msgLen += n;
				/* continue to read data from socket for other headers */
			}
		}
	} /* end of while( *bufp) */

	return RC_OK;
}


void sipTCPCallback(CxSock sock, CxEventType event, int err, void* context)
{
	RCODE		ret;
	int		n, recvleft;
	sipEvtCBParam*	param = (sipEvtCBParam*)context;
	char		buff[SIP_TX_MAXBUFLEN+1];
	char		*sp, *recvp;


	if ( event & CX_EVENT_EX ) {
		g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_END, SIP_MSG_UNKNOWN, NULL);
		return;
	}

	if ( event & CX_EVENT_WR) {
		g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_CONNECTED, SIP_MSG_UNKNOWN, NULL);
		cxEventRegister(g_sipEvt, sock, CX_EVENT_RD, sipTCPCallback, (void*)param);
		return;
	}

	/* else : event & CX_EVENT_RD */
	memset(buff, 0, SIP_TX_MAXBUFLEN*sizeof(char));
	recvleft = SIP_TX_MAXBUFLEN;
	recvp = buff;
	while( (n=cxSockRecvEx(sock, recvp, recvleft, 100)) > 0 ) {

		recvp[n] = 0;

		if((g_TrustList!=NULL)&&(strstr(g_TrustList,cxSockAddrGetAddr(cxSockGetRaddr(sock)))==NULL)){
			TCRPrint (TRACE_LEVEL_API, "[drop packet from cx - src:%s:%d]\n%s\n",
					cxSockAddrGetAddr(cxSockGetRaddr(sock)),
					cxSockAddrGetPort(cxSockGetRaddr(sock)),
					buff);
			cxSockFree(sock);
			return;
		}

		sp = NULL;
		ret = processInData(sock, buff, param, PROTOCOT_TCP, &sp);
		if( ret == RC_ERROR ){
			/* log error here */
			return;
		}
		if( sp != NULL ){
			n = strlen(sp);
			if( (SIP_TX_MAXBUFLEN - n) < 256 ){
				/* discard first 256 bytes,
				 * p_maxLen(SIP_TX_MAXBUFLEN) must > 300
				 */
				sp += 256-(SIP_TX_MAXBUFLEN-n);
				n = strlen(sp);
				memcpy(buff, sp, n );
				sp[n] = 0;
			}
			recvp = &buff[n];
			recvleft = SIP_TX_MAXBUFLEN - n;
		} else {
			recvp = buff;
			recvleft = SIP_TX_MAXBUFLEN;
		}
	} /* end of while cxSockRecvEx() */

	return;
}

void sipUDPCallback(CxSock sock, CxEventType event, int err, void* context)
{
	sipEvtCBParam* param = (sipEvtCBParam*)context;
	int nrecv;
	SipMsgType msgtype = SIP_MSG_UNKNOWN; /* ljchuang modified 0607 */
	void* message = NULL; /* ljchuang modified 0607 */
	
	/*static char buf[SIP_TX_MAXBUFLEN];*/
	char buf[SIP_TX_MAXBUFLEN];
	
	/* added by mac, since cxSockRecvfrom may return -1 ! Feb. 16, 2002 */
	buf[0] = '\0';

	/* memset(buf,0,sizeof(char)*SIP_TX_MAXBUFLEN);*/
	/* One UDP datagram may contain several SIP messages!! */
	nrecv = cxSockRecvfrom(sock, buf, SIP_TX_MAXBUFLEN);

	/* check added by mac, since cxSockRecvfrom may return -1 ! Feb. 16, 2002 */
	if ( nrecv >= 0 ) {
		buf[nrecv] = '\0';
	}

	if((g_TrustList!=NULL)&&(strstr(g_TrustList,cxSockAddrGetAddr(cxSockGetRaddr(sock)))==NULL)){
		TCRPrint (TRACE_LEVEL_API, "[drop packet from cx - src:%s:%d]\n%s\n",
				cxSockAddrGetAddr(cxSockGetRaddr(sock)),
				cxSockAddrGetPort(cxSockGetRaddr(sock)),
				buf);
		return;
	}

	if (nrecv > 0) {
#ifdef _DEBUG
 		TCRPrint (TRACE_LEVEL_API, "[Rev from cx - src:%s:%d]\n%s\n",
				cxSockAddrGetAddr(cxSockGetRaddr(sock)),
				cxSockAddrGetPort(cxSockGetRaddr(sock)),
				buf);
#endif
		msgtype = sipMsgIdentify((unsigned char*)buf);

		if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_UNKNOWN){ 
			/* message = sipReqNewFromTxt(buf); for received binary data */
			message = sipReqNewFromTxtEx(buf,nrecv);
#ifdef _DEBUG
			TCRPrint (TRACE_LEVEL_API, "[sipRev request]\n%s\n",sipReqPrint(message));
#endif
		}else if (msgtype == SIP_MSG_RSP){ 

			message = sipRspNewFromTxt(buf);

#ifdef _DEBUG
			TCRPrint (TRACE_LEVEL_API, "[sipRev response]\n%s\n",sipRspPrint(message));
#endif
		}else
			message = NULL;
	}
	
	/* set received and rport if socket address is not equal to sent-by */
	if (message && (msgtype == SIP_MSG_REQ)){
		/* add received and rport */
		char* viasent = NULL;
		char* viasentport = NULL;
		char* colon=NULL; /* for ipv6 */
		UINT16 sentport=5060;
		CxSockAddr raddr=cxSockGetRaddr(sock);
		char* socksource = cxSockAddrGetAddr(raddr);
		UINT16 sockport = cxSockAddrGetPort(raddr);
		SipVia *viahdr = (SipVia*)sipReqGetHdr((SipReq)message,Via);
		
		if (viahdr && viahdr->ParmList){ 
			viasent=trimWS(strDup(viahdr->ParmList->sentby));
			
			colon = strChr((unsigned char*)viasent,']');
			if(colon!=NULL)
				viasentport=strChr((unsigned char*)colon,':');
			else
				viasentport = strChr((unsigned char*)viasent,':');
			
			if(viasentport){
				*viasentport++='\0';
				sentport=a2i(viasentport);
			}
		}
		
					
		if (strCmp(viasent,socksource)!=0){
			if (viahdr && viahdr->ParmList){
				if(viahdr->ParmList->received)
					free(viahdr->ParmList->received);
				
				viahdr->ParmList->received = strDup( socksource );
			}
		}
		
		
		if(sentport!=sockport){
			if(viahdr && viahdr->ParmList){
				viahdr->ParmList->rport = (int)sockport;
			}
		}
		
		if(viasent) free(viasent);
	}

	/* ljchuang modified 0607 */
	/* check message if parse failed or NULL */
	if((SIP_MSG_PING==msgtype)||(SIP_MSG_PONG==msgtype)){
		g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, msgtype, NULL); 
	}else{
		if(message != NULL){
			g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, msgtype, (void*)message); 
		}else{
			g_sipUDPEvtCB(param->udp, SIP_UDPEVT_UNKN /* SIP_UDPEVT_DATA */, msgtype, NULL); 
			TCRPrint(TRACE_LEVEL_ERROR, "[sipRev]receive non-SIP message\n");
		}
	}
	/*g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, ((msgtype==SIP_MSG_UNKNOWN)?SIP_MSG_REQ:msgtype), (void*)message); */
}

void sipTCPServerAccept(CxSock sock, CxEventType event, int err, void* context)
{
	SipTCP newtcp;

	newtcp = sipTCPServerNew(sock);
	if (newtcp != NULL)
		g_sipTCPEvtCB(newtcp, SIP_TCPEVT_CLIENT_CONN, SIP_MSG_UNKNOWN, NULL);
}

/* add sipTCPNew() and rewrite sipTCPNEW(char*laddr,UINT16 lport) */
CCLAPI 
SipTCP sipTCPNew(const char* laddr,UINT16 lport)
{
	SipTCP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	//_this = malloc(sizeof *_this);
	_this = malloc(sizeof(struct sipTCPObj));
	/* error check */
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPNew]TCP object is NULL!\n");
		return NULL;
	}

	if(laddr){
		if(strlen(laddr) == 0)
			lsockaddr = cxSockAddrNew(NULL, lport); /* auto-fill my ip */
		else 
			if (inet_addr(laddr)!=-1)
				lsockaddr = cxSockAddrNew(laddr,lport);
		if(!lsockaddr)
			lsockaddr = cxSockAddrNew(NULL,lport);
	}else
		lsockaddr = cxSockAddrNew(NULL,lport);
	
	_this->ptrTCPPara=NULL;
	/* lsockaddr = NULL; */
	/* error check */

	sock = cxSockNew(CX_SOCK_STREAM_C, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxSockAddrFree(lsockaddr);

	return _this;
}

CCLAPI 
SipTCP sipTCPSrvNew(const char* laddr, UINT16 port)
{
	SipTCP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	_this = (SipTCP) malloc(sizeof *_this);
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPSrvNew]TCP object is NULL!\n");
		return NULL;
	}
	_this->ptrTCPPara = NULL;

	/* get server ipaddr & port from configfile */
	if ( laddr && (strlen(laddr) == 0 ))
		lsockaddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
	else {
		if ( inet_addr(laddr)!=-1 ) 
			lsockaddr = cxSockAddrNew(laddr,(UINT16)port);
	} 
	
	if(!lsockaddr){
		lsockaddr = cxSockAddrNew(NULL,(UINT16)port);
	}

	sock = cxSockNew(CX_SOCK_SERVER, lsockaddr);
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTCPServerAccept, NULL);
	cxSockAddrFree(lsockaddr);

	return _this;
}

CCLAPI RCODE sipTCPConnect(SipTCP _this,const char* raddr, UINT16 rport)
{
	CxSockAddr	rsockaddr;
	sipEvtCBParam*	param;

	if ((raddr == NULL) || (rport == 0)||(_this==NULL)) 
		return RC_ERROR;

	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL)
		return RC_ERROR;
	/* error check*/

	if (cxSockConnect(_this->sock_, rsockaddr) < 0) {
		cxSockAddrFree(rsockaddr);
		return RC_ERROR;
	}
	cxSockAddrFree(rsockaddr);

	/* register this new sipTCP for incoming event if callback is set*/
	if (g_sipTCPEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = _this;
		param->udp = NULL;
		param->tcptype = 0; /* TCP client */
		initCBParamMsginfo(param);
		_this->ptrTCPPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_WR|CX_EVENT_EX, sipTCPCallback, (void*)param);
	}

	return RC_OK;
}

CCLAPI 
const char*	sipTCPGetRaddr(SipTCP _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPGetRaddr]TCP object is NULL!\n");
		return NULL;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	if(raddr !=NULL)
		address=cxSockAddrGetAddr(raddr);
	else
		address=NULL;
	return address;
}

CCLAPI 
RCODE	sipTCPGetRport(SipTCP _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPGetRport]TCP object is NULL!\n");
		return RC_ERROR;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	(raddr != NULL)?(*port=cxSockAddrGetPort(raddr)):(rc=RC_ERROR);

	return rc;

}

CCLAPI
const char*	sipTCPGetLaddr(SipTCP _this)
{
	CxSockAddr laddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPGetLaddr]TCP object is NULL!\n");
		return NULL;
	}
	laddr=cxSockGetLaddr(_this->sock_);
	if(laddr !=NULL)
		address=cxSockAddrGetAddr(laddr);
	else
		address=NULL;
	return address;
}


CCLAPI 
void sipTCPFree(SipTCP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPFree]TCP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	/*Since tcp parameter have been free when callback*/
	if(_this->ptrTCPPara != NULL){
		free(_this->ptrTCPPara);
		_this->ptrTCPPara = NULL;
	}

	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_ = NULL;
	}

	/* Context has been freed alreay!
	if( context )
		free(context);
	*/

	free(_this);
}


SipTCP sipTCPServerNew(CxSock sock)
{
	SipTCP _this;
	sipEvtCBParam* param;
	CxSock	newsock;

	newsock = cxSockAccept(sock);
	if (newsock == NULL) return NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPServerNew]TCP object is NULL!\n");
		return NULL;
	}

	_this->sock_ = newsock;
	_this->ptrTCPPara=NULL;

	/* memory leak while active close!! */
	param = calloc(1,sizeof *param);
	param->tcp = _this;
	param->udp = NULL;
	param->tcptype = 1; /* TCP server */
	initCBParamMsginfo(param);
	_this->ptrTCPPara=param;

	cxEventRegister(g_sipEvt, newsock, CX_EVENT_RD, sipTCPCallback, (void*)param);

	return _this;
}

CCLAPI
void sipTCPServerFree(SipTCP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPServerFree]TCP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	/*Since tcp parameter have been free when callback*/
	if(_this->ptrTCPPara != NULL){
		free(_this->ptrTCPPara);
		_this->ptrTCPPara = NULL;
	}
	
	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_ = NULL;
	}
	/*
	Modified by Mac on May 22, 2003
	no idea why this code is here, which causes double free (context == _this->ptrTCPPara)
	This code seems was added by Acer on July 17, 2002
	if( context )
		free(context);
	*/
	free(_this);
}

CCLAPI 
int sipTCPSendTxt(SipTCP _this, char* msgtext)
{
	int nsent = 0;
	int	len;
	
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPSendTxt]TCP object is NULL!\n");
		return RC_ERROR;
	}

	if(!msgtext)
		return RC_ERROR;

	len = strlen(msgtext);
	while ( nsent < len  ) {
		nsent += cxSockSend(_this->sock_, &msgtext[nsent], len-nsent);
	}
	return nsent;
}

CCLAPI 
int sipTCPSendReq(SipTCP _this, SipReq request)
{
	int nsent = 0;
	char* msgtext;
	int	len;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPSendReq]TCP object is NULL!\n");
		return RC_ERROR;
	}
	/* SIP message body could contain null character??? */
	msgtext = sipReqPrint(request);
	if(_this->sock_ == NULL)
		nsent =0;
	else {
		len = strlen(msgtext);
		while ( nsent < len ) {
			nsent += cxSockSend(_this->sock_, &msgtext[nsent], len-nsent);
		}
	}
	return nsent;
}

CCLAPI 
int sipTCPSendRsp(SipTCP _this, SipRsp response)
{
	unsigned int nsent = 0,len;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTCPSendRsp]TCP object is NULL!\n");
		return RC_ERROR;
	}
	msgtext = sipRspPrint(response);

	TCRPrint(TRACE_LEVEL_ERROR,"[TCP sende]\n%s\n",msgtext);

	len=strlen(msgtext);
	if(_this->sock_ == NULL)
		nsent = 0 ;
	else {
		/*while (nsent < strlen(msgtext) ) {
			nsent += cxSockSend(_this->sock_, msgtext, strlen(msgtext));
		}*/
		while (nsent < len ) 
			nsent += cxSockSend(_this->sock_, &msgtext[nsent], len-nsent);
	}
	return nsent;
}

CCLAPI 
SipUDP sipUDPNew(const char* laddr, UINT16 lport)
{
	SipUDP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr;
	sipEvtCBParam* param;

	_this = calloc(1,sizeof *_this);
	if(_this == NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"alloc memory for UDP object is failed!\n");
		return NULL;
	}
	/* error check*/

	_this->ptrUDPPara=NULL;
	lsockaddr = cxSockAddrNew(laddr, lport);
	if(lsockaddr == NULL)
		return NULL;
	/* error check*/

	sock = cxSockNew(CX_SOCK_DGRAM, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}
	cxSockAddrFree(lsockaddr);

	_this->sock_ = sock;

	/* register this new sipUDP for incoming event if callback is set*/
	if (g_sipUDPEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->udp = _this;
		initCBParamMsginfo(param);
		_this->ptrUDPPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipUDPCallback, (void*)param);
	}

	_this->refcount = 1;

	return _this;
}


CCLAPI 
void sipUDPFree(SipUDP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrUDPPara != NULL){
		free(_this->ptrUDPPara);
		_this->ptrUDPPara = NULL;
	}
	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_=NULL;
	}
	/* context has been freed alreay!
	if( context )
		free(context);
	*/

	free(_this);

}

CCLAPI
unsigned short sipUDPGetRefCount(SipUDP _this)
{
	return 	_this->refcount;
}

CCLAPI
unsigned short sipUDPIncRefCount(SipUDP _this)
{
	return 	++(_this->refcount);
}

CCLAPI
unsigned short sipUDPDecRefCount(SipUDP _this)
{
	if (_this->refcount > 0)
		return --(_this->refcount);
	else
		return 0;
}

CCLAPI 
const char* sipUDPGetRaddr(SipUDP _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return NULL;
	}
	raddr = cxSockGetRaddr(_this->sock_);
	if(raddr!=NULL)
		address = cxSockAddrGetAddr(raddr);
	else
		address = NULL;

	return address;
}

CCLAPI 
RCODE sipUDPGetRport(SipUDP _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	raddr = cxSockGetRaddr(_this->sock_);
	if(raddr!=NULL)
		*port = cxSockAddrGetPort(raddr);
	else
		rc=RC_ERROR;

	return rc;
}

CCLAPI 
int sipUDPSendTxt(SipUDP _this, char* msgtext, char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;


	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	if(!msgtext)
		return RC_ERROR;

	/* modified by txyu, to solve the problem that one domain name mapping to
	   IPv4 and IPv6 address, 2004/5/20 */
	/* rsockaddr = cxSockAddrNew(raddr, rport); */

	/* Use cxSockAddrNewCxSock to force the application sends packets 
	   to the same IP family. In other words, send packets to remote machine's
	   IPv4 address if we bind at IPv4 address, or send packets to remote 
	   machine's IPv6 address if we bind at IPv6 address.
	*/
	rsockaddr = cxSockAddrNewCxSock(raddr, rport, _this->sock_);

	if(rsockaddr == NULL) 
		return RC_ERROR;

	if(_this->sock_ != NULL)
		nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);
	else
		nsent = RC_ERROR;

	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipUDPSendReq(SipUDP _this, SipReq request,const char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL)
		return RC_ERROR;
	msgtext = sipReqPrint(request);

	/*nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);*/
	nsent = cxSockSendto(_this->sock_, msgtext, sipReqLen(request), rsockaddr);

#ifdef _DEBUG
 		TCRPrint (TRACE_LEVEL_API, "[Send to cx - src:%s:%d]\n%s\n",
				raddr,
				rport,
				msgtext);
#endif

	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipUDPSendRsp(SipUDP _this, SipRsp response,const char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr == NULL)
		return RC_ERROR;
	msgtext = sipRspPrint(response);

	nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);
	
	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipEvtDispatch(int timeout)
{
	return cxEventDispatch(g_sipEvt, timeout);
}

CCLAPI 
int sipTimerSet(int delay, sipTimerCB cb, void* data)
{
	struct timeval d = {0, 0};
	int timerIdx;

	d.tv_sec = delay / 1000;
	d.tv_usec = (delay % 1000)*1000;
	timerIdx = cxTimerSet(d, cb, data);

	return timerIdx;
}

CCLAPI 
void* sipTimerDel(int e)
{
	return cxTimerDel(e);
}

CCLAPI 
int sipSetTrustList(IN const char *tl)
{
	if(g_TrustList!=NULL) free(g_TrustList);

	g_TrustList=strDup(tl);

	return 0;

}

CCLAPI 
char* sipGetTrustList(void)
{
	return g_TrustList;
}


#ifdef ICL_TLS

void sipTLSServerAccept(CxSock sock, CxEventType event, int err, void* context)
{
	SipTLS newtls;

	TCRPrint(TRACE_LEVEL_ERROR,"sipTLSServerAccept\n");

	newtls = sipTLSServerNew(sock);
	if (newtls != NULL)
		g_sipTLSEvtCB(newtls, SIP_TLSEVT_CLIENT_CONN, SIP_MSG_UNKNOWN, NULL);
}

CCLAPI
void sipTLSChkPeerCert(SSL *ssl );

static int sslRecv(SSL *ssl, char *buff, int buffsize)
{

#ifndef ICL_NONBLOCK
	int	fd;
#if defined(_WIN32)
	int	res;
	fd_set	rset;
	struct	timeval t;
#else
	int	n, orgFlag, newFlag;
#endif

	if ( (fd=SSL_get_fd(ssl)) == -1 )
		return -1;

#if defined(_WIN32)
	/* using select to check if there are datas in the socket of ssl for win32, since I have no idea about how to get original socket mode in win32 */

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	t.tv_sec = 0;
	t.tv_usec = 2;

	res = select(fd+1, &rset, NULL, NULL, &t);
	if( res != 1 )
		return 0;
	else
		return SSL_read(ssl,buff,buffsize);
#else

	orgFlag = fcntl(fd, F_GETFL , NULL);
	newFlag = orgFlag | O_NONBLOCK;
	fcntl(fd, F_SETFL , newFlag );

	n = SSL_read(ssl,buff,buffsize);

	fcntl(fd, F_SETFL , orgFlag );
	return n;

#endif /*end of defined(_WIN32) */

#else
	return SSL_read(ssl,buff,buffsize);

#endif /* ICL_NONBLOCK */
}

void sipTLSCallback(CxSock sock, CxEventType event, int err, void* context)
{
	RCODE		ret;
	int		n, recvleft;
	sipEvtCBParam*	param = (sipEvtCBParam*)context;
	char		buff[SIP_TX_MAXBUFLEN+1];
	char		*sp, *recvp;
	SSL		*ssl = param->tls->ssl;
	int		err0 = 0,err1 = 0;

	if ( event & CX_EVENT_EX ) {
		g_sipTLSEvtCB(param->tls, SIP_TLSEVT_END, SIP_MSG_UNKNOWN, NULL);
		return;
	}

	if ( event & CX_EVENT_WR) {
		TCRPrint (TRACE_LEVEL_API, "[sipTLSCallback] try SSL_connect\n");

		err0 = SSL_set_fd (ssl, cxSockGetSock(sock) );
		/*CHK_SSL(err0);*/
		if( err0 != 1 ){
			err1 = SSL_get_error(ssl, err0);
			TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSCallBack] SSL_set_fd err,%d\n",err1);
			return;
		}

		while( (err0 = SSL_connect(ssl)) < 0) {
			err1 = SSL_get_error(ssl, err0);
			if ( (err1 != SSL_ERROR_WANT_READ)&&(err1 != SSL_ERROR_WANT_WRITE)){
				TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSCallBack]: SSL_connect() err, %d\n", err1);
				return;
			}
			TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSCallBack]: SSL_connect() SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE\n");
			sleeping(100);
		}
		TCRPrint (TRACE_LEVEL_API, "[sipTLSCallback] SSL_connect successful\n");
		
		g_sipTLSEvtCB(param->tls, SIP_TLSEVT_CONNECTED, SIP_MSG_UNKNOWN, NULL);
		cxEventRegister(g_sipEvt, sock, CX_EVENT_RD, sipTLSCallback, (void*)param);
		return;
	}

	/* else : event & CX_EVENT_RD */

	memset(buff, 0, SIP_TX_MAXBUFLEN*sizeof(char));
	recvleft = SIP_TX_MAXBUFLEN;
	recvp = buff;
	/*
	while( (n=SSL_read(ssl,recvp,recvleft)) >0 ) {
		MARK_NOTE: since SSL_read will wait for at least one byte data in,
			   so, change the flag of the file descriptor to nonblock.
	*/
	while( (n=sslRecv(ssl,recvp,recvleft)) >0 ) {
		recvp[n] = 0;
		sp = NULL;
		ret = processInData(sock, buff, param, PROTOCOT_TLS, &sp);
		if( ret == RC_ERROR ){
			/* log error here */
			return;
		}
		if( sp != NULL ){
			n = strlen(sp);
			if( (SIP_TX_MAXBUFLEN - n) < 256 ){
				/* discard first 256 bytes,
				 * p_maxLen(SIP_TX_MAXBUFLEN) must > 300
				 */
				sp += 256-(SIP_TX_MAXBUFLEN-n);
				n = strlen(sp);
				memcpy(buff, sp, n );
				sp[n] = 0;
			}
			recvp = &buff[n];
			recvleft = SIP_TX_MAXBUFLEN - n;
		} else {
			recvp = buff;
			recvleft = SIP_TX_MAXBUFLEN;
		}
	} /* end of while cxSockRecvEx() */

	return;
}

/* Create a new TLS object */
CCLAPI 
SipTLS sipTLSNew(const char* laddr, UINT16 lport)
{
	SipTLS		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	if(!g_CLIENTTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]Client TLS ctx is NULL!\n");
		return NULL;
	}

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]TLS object is NULL!\n");
		return NULL;
	}

	if(laddr){
		if(strlen(laddr) == 0)
			lsockaddr = cxSockAddrNew(NULL, lport); /* auto-fill my ip */
		else{ 
			if (inet_addr(laddr)!=-1)
				lsockaddr = cxSockAddrNew(laddr,lport);
		}
		if(!lsockaddr)
			lsockaddr = cxSockAddrNew(NULL,lport);
	}else
		lsockaddr = cxSockAddrNew(NULL,lport);

	_this->ptrTLSPara=NULL;

	/* error check*/
	sock = cxSockNew(CX_SOCK_STREAM_C, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxSockAddrFree(lsockaddr);	

	_this->ssl=SSL_new(g_CLIENTTLSCTX);

	if(_this->ssl==NULL){
		cxSockFree(_this->sock_);
		free(_this);
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]new ssl is failed!\n");
		return NULL;
	}

	return _this;
}

CCLAPI 
SipTLS sipTLSSrvNew(const char* laddr, UINT16 port)
{
	SipTLS		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TLS object is NULL!\n");
		return NULL;
	}

	_this->ptrTLSPara = NULL;
	_this->ssl = NULL;

	/* get server ipaddr & port from configfile */
	if ( laddr && (strlen(laddr) == 0 ))
		lsockaddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
	else{
		if ( inet_addr(laddr)!=-1 ) 
			lsockaddr = cxSockAddrNew(laddr,(UINT16)port);
	} 
	if(!lsockaddr){
		lsockaddr = cxSockAddrNew(NULL,(UINT16)port);
	}

	sock = cxSockNew(CX_SOCK_SERVER, lsockaddr);
	cxSockAddrFree(lsockaddr);
	if (!sock) {
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTLSServerAccept, NULL);

	return _this;
}

CCLAPI
void sipTLSServerFree(SipTLS _this)
{
	void* context = NULL;


	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TLS object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrTLSPara!=NULL){
		free(_this->ptrTLSPara);
		_this->ptrTLSPara=NULL;
	}
	
	if(_this->sock_ != NULL){
		cxSockFree(_this->sock_);
		_this->sock_ =NULL;
	}

	if( _this->ssl ){
		SSL_free(_this->ssl);
		_this->ssl=NULL;
	}
	
	free(_this);
}

/* Establish a TLS connection */
/* input a tcp object pointer and IP address/port for remote side */
SipTLS sipTLSServerNew(CxSock sock)
{
	SipTLS _this;
	sipEvtCBParam* param;
	CxSock	newsock;
	int err, err0;

	if(!g_SERVERTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew]TLS ctx is NULL!\n");
		return NULL;
	}
	
	newsock = cxSockAccept(sock);
	if (newsock == NULL) 
		return NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew]TLS object is NULL!\n");
		return NULL;
	}
	_this->sock_ = newsock;
	_this->ptrTLSPara=NULL;

	_this->ssl = SSL_new (g_SERVERTLSCTX);
	if( _this->ssl == NULL ){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew] SSL_new error!\n");
		free(_this);
		return NULL;
	}

	err = SSL_set_fd (_this->ssl, cxSockGetSock(newsock));
	/* check err: CHK_SSL(err);*/
	if( err <= 0 ){
		err0 = SSL_get_error(_this->ssl, err);
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew] SSL_set_fd error, %d(%s)\n",
					err0, ERR_reason_error_string(err0) );
		SSL_free(_this->ssl);
		free(_this);
		return NULL;
	}

	while ( (err=SSL_accept(_this->ssl))<=0 ) {
		err0 = SSL_get_error(_this->ssl, err);
		if( err==-1 && (err0==SSL_ERROR_WANT_READ || err0==SSL_ERROR_WANT_WRITE)){
			sleeping(100);
			continue;
		}
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew] SSL_accept error, %d(%s)\n",
					err0, ERR_reason_error_string(err0) );
		/* Do I need to release some resource ? */
		SSL_free(_this->ssl);
		free(_this);
		return NULL;
	}

	#ifdef _DEBUG
	sipTLSChkPeerCert(_this->ssl);
	#endif

	/* memory leak while active close!! */
	param = calloc(1,sizeof *param);
	param->tcp = NULL;
	param->udp = NULL;
	param->tls = _this;
	param->tcptype = 1; /* TLS server */
	initCBParamMsginfo(param);
	_this->ptrTLSPara=param;

	cxEventRegister(g_sipEvt, newsock, CX_EVENT_RD, sipTLSCallback, (void*)param);

	return _this;	
}

CCLAPI 
RCODE sipTLSConnect(IN SipTLS _this,IN const char* raddr,IN UINT16 rport)
{
	CxSockAddr	rsockaddr;
	sipEvtCBParam*	param;
	/*int err; */

	if(!g_CLIENTTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]TLS ctx is NULL!\n");
		return RC_ERROR;
	}

	if ((raddr == NULL) || (rport == 0)||(_this==NULL)){ 
		TCRPrint(TRACE_LEVEL_API,"[sipTLSConnect]input parameters is NULL!\n");
		return RC_ERROR;
	}

	if((_this->sock_==NULL)||(_this->ssl==NULL)){
		TCRPrint(TRACE_LEVEL_WARNING,"[sipTLSConnect]NULL pointer: sock_ or ssl\n");
		return RC_ERROR;
	}

	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]new sockaddr is NULL!\n");
		return RC_ERROR;
	}
	/* error check*/

	if (cxSockConnect(_this->sock_, rsockaddr) < 0) {
		cxSockAddrFree(rsockaddr);
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]connect failed!\n");
		return RC_ERROR;
	}
	cxSockAddrFree(rsockaddr);

	/* register this new sipTCP for incoming event if callback is set*/
	if (g_sipTLSEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->tls = _this;
		param->udp = NULL;
		param->tcptype = 0; /* TCP client */
		initCBParamMsginfo(param);
		_this->ptrTLSPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_WR|CX_EVENT_EX, sipTLSCallback, (void*)param);
	}

	/* tls connect */
	/*
	SSL_set_fd (_this->ssl, cxSockGetSock(_this->sock_));
    err=SSL_connect (_this->ssl);

    CHK_SSL(err);
*/
	/* register this new sipTCP for incoming event if callback is set*/
/*
	if (g_sipTLSEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->tls = _this;
		param->udp = NULL;
		param->tcptype = 0;
		initCBParamMsginfo(param);
		_this->ptrTLSPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTLSCallback, (void*)param);
	}
*/

	return RC_OK;
}

/* free a TLS object */
CCLAPI 
void sipTLSFree(IN SipTLS _this)
{

	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSFree]TLS object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrTLSPara != NULL){
		free(_this->ptrTLSPara);
		_this->ptrTLSPara = NULL;
	}

	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_=NULL;
	}
	
	if( _this->ssl ){
		SSL_free(_this->ssl);
		_this->ssl=NULL;
	}

	memset(_this,0,sizeof *_this);

	free(_this);
}

/* Send out a buffer of character string using a exist TLS connect*/
/* return value = how many bytes had been sent */
CCLAPI 
int	sipTLSSendTxt(IN SipTLS _this, IN char* msgtext)
{
	int nsent;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]TLS object is NULL!\n");
		return RC_ERROR;
	}

	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]ssl is NULL!\n");
		return RC_ERROR;
	}

	if(!msgtext){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]message is NULL!\n");
		return RC_ERROR;
	}
	nsent = SSL_write(_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Send out a sip request message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI 
int		sipTLSSendReq(IN SipTLS _this, IN SipReq req)
{
	int nsent;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]TCP object is NULL!\n");
		return RC_ERROR;
	}
	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]ssl is NULL!\n");
		return RC_ERROR;
	}
	if( !req ) {
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]request is NULL!\n");
		return RC_ERROR;
	}
	/* SIP message body could contain null character??? */
	msgtext = sipReqPrint(req);
	if( msgtext == NULL )
		nsent =0;
	else
		nsent = SSL_write (_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Send out a sip response message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI 
int	sipTLSSendRsp(IN SipTLS _this, IN SipRsp rsp)
{
	int nsent;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]TLS object is NULL!\n");
		return RC_ERROR;
	}
	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]ssl is NULL!\n");
		return RC_ERROR;
	}
	if( !rsp ) {
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]response is NULL!\n");
		return RC_ERROR;
	}

	/* SIP message body could contain null character??? */
	msgtext = sipRspPrint(rsp);
	if( msgtext == NULL )
		nsent =0;
	else
		nsent = SSL_write (_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Get remote side address from a TLS object */
			/* return value: remote side IP address */
CCLAPI 
const char*	sipTLSGetRaddr(IN SipTLS _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSGetRaddr]TLS object is NULL!\n");
		return NULL;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	if(raddr !=NULL)
		address=cxSockAddrGetAddr(raddr);
	else
		address=NULL;
	return address;	
}

			/* Get remote side port number from a TLS object */
			/* return value: remote side port number */
CCLAPI 
RCODE sipTLSGetRport(SipTLS _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSGetRport]TLS object is NULL!\n");
		return RC_ERROR;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	(raddr != NULL)?(*port=cxSockAddrGetPort(raddr)):(rc=RC_ERROR);

	return rc;
}


CCLAPI 
void sipTLSChkPeerCert(SSL *ssl )
{
	char	*sp;
	X509	*peer_cert;


	/* check certificate result */
	if( SSL_get_verify_result(ssl) != X509_V_OK){
		TCRPrint(TRACE_LEVEL_API, "Warning: SSL_get_verify_result not OK !!\n");
	}

	peer_cert = SSL_get_peer_certificate (ssl);
	if (peer_cert != NULL) {
		TCRPrint (TRACE_LEVEL_API, "Client certificate:\n");

		sp = X509_NAME_oneline (X509_get_subject_name (peer_cert), 0, 0);
		if( sp == NULL ){
			TCRPrint(TRACE_LEVEL_API,"ERROR: X509_NAME_oneline: X509_get_subject_name  error");
			return;
		}
		TCRPrint(TRACE_LEVEL_API,"\t subject: %s\n", sp);
		OPENSSL_free (sp); 
		sp = X509_NAME_oneline (X509_get_issuer_name  (peer_cert), 0, 0);
		if( sp == NULL ){
			TCRPrint(TRACE_LEVEL_API,"ERROR: X509_NAME_oneline:X509_get_issuer_name  error");
			return;
		}
		TCRPrint(TRACE_LEVEL_API,"\t issuer: %s\n", sp);
		OPENSSL_free (sp);

		/* We could do all sorts of certificate verification stuff here before
		deallocating the certificate. */

		X509_free (peer_cert);

	} else
		TCRPrint(TRACE_LEVEL_API,"Client does not have certificate.\n");


}

#endif
