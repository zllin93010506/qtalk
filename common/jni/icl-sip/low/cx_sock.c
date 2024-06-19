/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.c
 *
 * $Id: cx_sock.c,v 1.3 2008/12/04 06:55:16 tyhuang Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#pragma comment(lib, "ws2_32")
#endif

#ifdef UNIX

#ifndef h_addr
#define h_addr h_addr_list[0] /* Address, for backward compatibility.*/
#endif

#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "cx_sock.h"
#include "cx_misc.h"
#include <common/cm_trace.h>

#ifdef _WIN32_WCE
#define errno GetLastError()
#endif


#ifndef EINVAL
#define EINVAL WSAEINVAL
#endif

#ifdef _WIN32
static int	need_winsock_cleanup = 0;
#endif
static int	sock_refcnt = 0;

static CxSockAddr	freeList = NULL;

struct cxSockAddrObj {
	char			addr_[32];
	UINT16			port_;
	struct sockaddr_in	sockaddr_;
	CxSockAddr		next_;
};

struct cxSockObj {
	SOCKET			sockfd_;
	CxSockType		type_;
	CxSockAddr		laddr_;
	CxSockAddr		raddr_;
};

void cxSockInit_(void) {} /* just for DLL forcelink */

/** @brief  CxSockInit -  initail socket 
 *
 *  This Routine checks socket availability and initiates some global variables
 *
 *  @return On success, return RC_OK;
 *          Otherwise, return RC_ERROR;
 */
RCODE cxSockInit()
{
	RCODE rc = RC_OK;
#ifdef _WIN32
	WSADATA	wsadata;
	SOCKET sock;
#endif
	if(++sock_refcnt>1)
		return RC_OK;

#ifdef _WIN32
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		if (WSAStartup(WINSOCK_VERSION , &wsadata) == SOCKET_ERROR) {
			rc = RC_ERROR;
		} else
			need_winsock_cleanup = 1;
	} else
		closesocket(sock);
#endif
	freeList = NULL;

	return rc;
}


/** @brief  CxSockClean - release resource 
 *
 *  This Routine releases the resource used in cx_sock module
 *
 *  @return always return RC_OK (SUCCESS).
 */
RCODE cxSockClean()
{
	CxSockAddr _this;
	RCODE rc = RC_OK;

	if(--sock_refcnt>0)
		return RC_OK;

#ifdef _WIN32
	if (need_winsock_cleanup)
		if (WSACleanup() == SOCKET_ERROR) {
			rc = RC_ERROR;
		}
#endif
	while (freeList != NULL) {
		_this = freeList;
		freeList = freeList->next_;
		free(_this);
	}
	freeList = NULL;
	sock_refcnt = 0;

	return rc;
}

/** @brief  CxSockAddrAlloc - allocate memory for CxSockAddr
 *
 *  This Routine allocates memory for CxSockAddr which is used to describe
 *  an Internet socket address.
 *
 *  @return On success, returns a pointer which data type is CxSockAddr.
 *          Otherwise, NULL is returned.
 */
static CxSockAddr cxSockAddrAlloc(void)
{
	CxSockAddr _this;

	if( freeList != NULL ) {
		_this        = freeList;
		freeList     = freeList->next_;
		_this->next_ = NULL;
	} else {
		//_this = malloc(sizeof *_this);
		_this = malloc(sizeof(struct cxSockAddrObj));
		if (_this == NULL) 
			return NULL;
	}

	return _this;
}

/** @brief  CxSockAddrNew - allocate an CxSockAddr and fill all fields in the structure.
 *
 *  This Routine allocate the memory for CxSockAddr and fill all fields in the structure
 *   according to the input parameters.
 *
 *  @param addr the IP address or domain name. If addr is NULL, INADDR_ANY is used. 
 *              (INNADDR_ANY: Address to accept any incoming messages, usually include 
 *                            lookback address, and other addresses. )
 *  @param port the port number.
 *  @return On success, CxSockAddrNew returns a pointer which data type is CxSockAddr.
 *          Otherwise, NULL is returned.
 */
/* Fun: cxSockAddrNew
 *
 * Desc: addr - IP address or domain name, auto-fill local IP addr if NULL.
 *
 * Ret: NULL if error.
 */
CCLAPI
CxSockAddr cxSockAddrNew(const char* addr, UINT16 port)
{
	struct hostent *he;
	CxSockAddr _this = cxSockAddrAlloc();

	if (_this == NULL) return NULL;

	memset((char *)&_this->sockaddr_, 0, sizeof(_this->sockaddr_));
	_this->sockaddr_.sin_family = AF_INET;
	_this->sockaddr_.sin_port = htons(port);

	if(addr == NULL) {
		_this->sockaddr_.sin_addr.s_addr = htonl(INADDR_ANY); /* auto-fill with local IP */		
	/*} else if (!isalpha((int)addr[0])) {*/
	} else if ( inet_addr(addr)!=-1 ) {
		_this->sockaddr_.sin_addr.s_addr = inet_addr(addr);	
	} else if ((he=gethostbyname(addr)) != NULL) {
		memcpy(&_this->sockaddr_.sin_addr, he->h_addr, he->h_length);
	} else {
		TCRPrint(1, "<cxSockAddrNew>: Invalid addr.");
		cxSockAddrFree(_this);
		return NULL;
	}

	strcpy(_this->addr_, inet_ntoa(_this->sockaddr_.sin_addr));
	_this->port_ = port;
	_this->next_ = NULL;

	return _this;
}

/** @brief  CxSockAddrNewCxSock - allocate an CxSockAddr and fill all fields in the structure.
 *
 *  This Routine allocate the memory for CxSockAddr and fill all fields in the structure
 *   according to the input parameters.
 *
 *  [Same as cxSockAddrNew() in IPv4 environment. ]
 *
 *  @param addr the IP address or domain name. If addr is NULL, INADDR_ANY is used. 
 *              (INNADDR_ANY: Address to accept any incoming messages, usually include 
 *                            lookback address, and other addresses. )
 *  @param port the port number.
 *  @param local_socket invalid parameter in IPv4 environment.
 *  @return On success, CxSockAddrNew returns a pointer which data type is CxSockAddr.
 *          Otherwise, NULL is returned.
 */
/* Fun: cxSockAddrNewCxSock
 *
 * Desc: addr - IP address or domain name, auto-fill local IP addr if NULL.
 * Ret: NULL if error.
 */
CCLAPI
CxSockAddr cxSockAddrNewCxSock (const char* addr, UINT16 port, CxSock local_socket)
{
/*
 * This function is the same as cxSockAddrNew in IPv4 environment
 */
	return cxSockAddrNew(addr, port);
}


/** @brief  CxSockAddrFree - release the CxSockAddr 
 *
 *  This Routine release the CxSockAddr ( put the CxSockAddr to freelist ).
 *
 *  @param _this  the pointer with data type CxSockAddr that want to be freed.
 *  @return On success, RC_OK is returned.
 *          Otherwise, RC_ERROR is returned.
 */
CCLAPI
RCODE cxSockAddrFree(CxSockAddr _this)
{
	CxSockAddr n = freeList;
	if( _this == NULL )
		return RC_ERROR;
	
	while( n ) {
		if( n==_this )
			return RC_ERROR;
		n = n->next_;
	}
	_this->next_ = freeList;
	freeList = _this;

	return RC_OK;
}

/** @brief  CxSockAddrDup - duplicate a CxSockAddr
 *
 *  This Routine creates a copy of the CxSockaddr _this.
 *
 *  @param _this  the pointer with data type CxSockAddr that want to be duplicated.
 *  @return On success, CxSockAddrDup returns a new CxSockAddr pointer.
 *          Otherwise, NULL is returned.
 */
CCLAPI
CxSockAddr cxSockAddrDup(CxSockAddr _this)
{
	CxSockAddr _copy;

	if (_this == NULL) return NULL;

	_copy = cxSockAddrAlloc();
	if (_copy == NULL) return NULL;

	strcpy(_copy->addr_, _this->addr_);
	_copy->port_ = _this->port_;
	_copy->sockaddr_ = _this->sockaddr_;

	return _copy;
}

/** @brief  CxSockAddrGetAddr - get the IP address of CxSockAddr
 *
 *  This Routine return the Ip address in the input CxSockAddr parameter.
 *
 *  @param _this  the pointer of the IP address
 *  @return On success, CxSockAddrGetAddr returns the character pointer for the IP address
 *          Otherwise, NULL is returned.
 */
CCLAPI
char* cxSockAddrGetAddr(CxSockAddr _this)
{
	return (_this)?_this->addr_:NULL;
}

/** @brief  CxSockAddrGetPort - get the port number of CxSockAddr
 *
 *  This Routine return the port number in the input CxSockAddr parameter.
 *
 *  @param _this  the pointer of the IP address
 *  @return On success, CxSockAddrGetPort returns the port number 
 *          Otherwise, 0 is returned.
 */
CCLAPI
UINT16 cxSockAddrGetPort(CxSockAddr _this)
{
	return (_this)?_this->port_:0;
}

/** @brief  CxSockAddrGetSockAddr - get the Internet socket address in CxSockAddr
 *
 *  This Routine return the pointer of struct sockaddr_in (Internet socket address) in the input CxSockAddr parameter.
 *
 *  @param _this  the pointer of the sockaddr_in
 *  @return On success, CxSockAddrGetSockAddr returns the struct sockaddr_in pointer
 *          Otherwise, NULL is returned.
 */
struct sockaddr_in* cxSockAddrGetSockAddr(CxSockAddr _this)
{
	return (_this)?&(_this->sockaddr_):NULL;
}

static int s_nDefaultTos = 0;

/** @brief  CxSockSetDefaultTos - set default value for TOS.
 *
 *  This Routine sets the default value of TOS into the value of input parameter.
 *  See cxSockSetTos for more information.
 *
 *  @param tos the default value for TOS.
 */
/* Fun: cxSockSetDefaultToS
 *
 * Desc: Set the default TOS. UACom can set the tos value directly.
 *
 * Ret: none.
 */
void cxSockSetDefaultToS(int tos)
{
	s_nDefaultTos = tos;
}

/** @brief  CxSockSetToS - set TOS (Type Of Service) bit in IP header 
 *
 *  This Routine sets the TOS (Type Of Service) bit in IP header.
 *
 *  TOS: Type of Service controls the priority of the packet. The first 3 bits stand for routing priority, the next 4 bits for the type of service (delay, throughput, reliability and cost).
 *       Common Defaults: 0x00 (normal) 
 *       See RFC 1394 "Type of Service in the Internet Protocol Suite" for more detail.
 *
 *  [[WIN: CxSockSetTos is unable to mark the Internet Protocol type of service bits in Internet Protocol packet header on On Windows 2000, Windows XP, and Windows Server 2003.]]
 *
 *  @param _this  the pointer of CxSock to setup.
 *  @param tos  the TOS value.
 *  @return On success, CxSockSetTos returns 0.
 *          Otherwise, CxSockSetTos returns the error code from WSAGetLastError().
 */
/* Fun: cxSockSetToS
 *
 * Desc: Set the TOS.
 *
 * Ret: If successful, return 0. If fail, return the error code from WSAGetLastError()
 */
int cxSockSetToS(CxSock _this, int tos)
{
	SOCKET thisSocket;
	int tos_len;
	int nError = 0;

	thisSocket = _this->sockfd_;
	tos_len = sizeof(tos);
	if (setsockopt( thisSocket, IPPROTO_IP, IP_TOS, (char*)&tos, tos_len) == SOCKET_ERROR)
	{
		#ifdef _WIN32
		nError = WSAGetLastError();
		#endif
	}
	return nError;
}

/** @brief  CxSockNew - create a socket for communication
 *
 *  This Routine creates a socket for comunication and bind local address with laddr if input parameter laddr is not NULL. If the input parameter type is CX_SOCK_SERVER, CxSockNew will also listen for connections on the socket.
 *  For non-BLOCK mode, just define "ICL_NONBLOCK" when compiling. The default is BLOCK  mode if not defined.
 *
 *  @param type  socket type, defined in cx_sock.h
 *  @param laddr the CxSockaddr pointer, specifies the associate local address
 *  @return On success, CxSockAddrGetPort returns a CxSock pointer which describes the socket informations.
 *          Otherwise, NULL is returned.
 */
/* Fun: cxSockNew
 *
 * Desc: Bind address to socket if laddr != NULL.
 *       Listen if a TCP server socket.
 *
 * Ret: NULL if error.
 */
CxSock cxSockNew(CxSockType type, CxSockAddr laddr)
{
	CxSock _this;
	int    yes = 1;

	if ((type != CX_SOCK_DGRAM) && (type != CX_SOCK_SERVER)
		&& (type != CX_SOCK_STREAM_C) && (type != CX_SOCK_STREAM_S))
		return NULL;

	_this = (CxSock)malloc(sizeof *_this);
	if (_this == NULL) return NULL;

	_this->sockfd_ = INVALID_SOCKET;
	_this->type_ = type;

	if (laddr == NULL)
		_this->laddr_ = NULL;
	else
		_this->laddr_ = cxSockAddrDup(laddr);

	_this->raddr_ = NULL;

	if (type == CX_SOCK_STREAM_S)
		return _this;

	if (type == CX_SOCK_DGRAM)
		_this->sockfd_ = socket(PF_INET, SOCK_DGRAM, 0);
	else if ((type == CX_SOCK_SERVER) || (type == CX_SOCK_STREAM_C))
		_this->sockfd_ = socket(PF_INET, SOCK_STREAM, 0);

	if (_this->sockfd_ == INVALID_SOCKET) {
		TCRPrint(1, "<cxSockNew>: socket() error.\n");
		cxSockFree(_this);
		return NULL;
	} else
		TCRPrint(100, "<cxSockNew>: socket = %d\n", _this->sockfd_);

	/* set socket in non-blocking mode */

#ifdef ICL_NONBLOCK
	{
#if defined(_WIN32)
	unsigned long nonblock;
	nonblock = 1;
	ioctlsocket(_this->sockfd_, FIONBIO, &nonblock);
#else
	int flags;
	if ((flags = fcntl(_this->sockfd_, F_GETFL, 0)) < 0) return NULL;
	flags |= O_NONBLOCK;
	if (fcntl(_this->sockfd_, F_SETFL, flags) < 0) return NULL;
#endif
	}
#endif

	if (type == CX_SOCK_DGRAM && laddr != NULL) {
		if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
			TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
			cxSockFree(_this);
			return NULL;
		}

			
		if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
			== -1) {
			TCRPrint(1, "<cxSockNew>: bind() error., %d\n", errno);
			cxSockFree(_this);
			return NULL;
		}
	}
	
	if (type == CX_SOCK_STREAM_C && laddr != NULL) {
		if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
			TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
			cxSockFree(_this);
			return NULL;
		}		
		if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
			== -1) {
			TCRPrint(1, "<cxSockNew>: bind() error., errno=%d\n", errno);
			cxSockFree(_this);
			return NULL;
		}
	}
	
	if (type == CX_SOCK_SERVER) {
		if (laddr != NULL) {
			if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
				TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
				cxSockFree(_this);
				return NULL;
			}		
			if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
				== -1) {
				TCRPrint(1, "<cxSockNew>: bind() error, %d.\n", errno);
				cxSockFree(_this);
				return NULL;
			}
		}
		else {
			TCRPrint(1, "<cxSockNew>: CX_SOCK_SERVER needs $laddr.\n");
			cxSockFree(_this);
			return NULL;
		}

		if (listen(_this->sockfd_, 5) == -1) {
			TCRPrint(1, "<cxSockNew>: listen() error.\n");
			cxSockFree(_this);
			return NULL;
		}
	}

	cxSockSetToS( _this, s_nDefaultTos);

	return _this;
}
 
/** @brief  cxSockFree - close socket connection and release the resource used in the CxSock 
 *
 *  This Routine closes the socket connection and release the resource used in CxSock parameter. Also the memory that the parameter _this pointed to will been released.
 *
 *  @param _this  the pointer of the CxSock
 *  @return On success, RC_OK is returned.
 *          Otherwise, RC_ERROR is returned.
 */
RCODE cxSockFree(CxSock _this)
{
	if( !_this )
		return RC_ERROR;

	if (_this->sockfd_ != INVALID_SOCKET) {
		closesocket(_this->sockfd_);
	}

	if (_this->laddr_ != NULL) {
		cxSockAddrFree(_this->laddr_);
	}

	if (_this->raddr_ != NULL) {
		cxSockAddrFree(_this->raddr_);
	}

	free(_this);

	return RC_OK;
}

/** @brief  cxSockGetSock - get the socket descriptor in CxSock
 *
 *  This Routine returns the socket descriptor in the input parameter _this
 *
 *  @param _this  the pointer of the CxSock
 *  @return On success, socket descriptor is returned.
 *          Otherwise, -1 is returned.
 */
SOCKET cxSockGetSock(CxSock _this)
{
	return (_this)?_this->sockfd_:-1;
}

/** @brief  cxSockSetSock - set the socket descriptor into CxSock
 *
 *  This Routine sets the socket descriptor s into CxSock _this
 *
 *  @param _this  the pointer of the CxSock
 *  @param s  the socket descriptor
 *  @return On success, RC_OK is returned.
 *          Otherwise, -1 is returned.
 */
RCODE cxSockSetSock(CxSock _this, SOCKET s)
{
	if( !_this )
		return RC_ERROR;
	_this->sockfd_ = s;

	return RC_OK;
}

/** @brief  CxSockGetRaddr - get the remote socket address information in CxSock
 *
 *  This Routine returns the remote socket address information in the input CxSock parameter.
 *
 *  @param _this the pointer of CxSock 
 *  @return On success, CxSockAddrGetRaddr returns a CxSockAddr pointer which describes the informations about remote socket address.
 *          Otherwise, NULL is returned.
 */
CxSockAddr cxSockGetRaddr(CxSock _this)
{
	return (_this)?_this->raddr_:NULL;
}

/** @brief  CxSockSetRaddr - set the remote socket address information into CxSock
 *
 *  This Routine sets the remote socket address information raddr into the input CxSock parameter _this. If the input parameter raddr is NULL, the remote address in _this will be set to NULL.
 *
 *  @param _this the pointer of CxSock 
 *  @param raddr the pointer of CxSockAddr, specifies the remote socket address
 *  @return Always return RC_OK.
 */
RCODE cxSockSetRaddr(CxSock _this, CxSockAddr raddr)
{
	if (!_this)
		return RC_ERROR;
	if (_this->raddr_ != NULL)
		cxSockAddrFree(_this->raddr_);

	if ( !raddr ) 
		_this->raddr_ = NULL;
	else
		_this->raddr_ = cxSockAddrDup(raddr);

	return RC_OK;
}

/** @brief  CxSockGetLaddr - get the local socket address information in CxSock
 *
 *  This Routine returns the local socket address information in the input CxSock parameter.
 *
 *  @param _this the pointer of CxSock 
 *  @return On success, CxSockAddrGetLaddr returns a CxSockAddr pointer which describes the informations about local socket address.
 *          Otherwise, NULL is returned.
 */
CxSockAddr cxSockGetLaddr(CxSock _this)
{
	return (_this)?_this->laddr_:NULL;
}

/** @brief  CxSockSetLaddr - set the local socket address information into CxSock
 *
 *  This Routine sets the local socket address information laddr into the input CxSock parameter _this. If the input parameter laddr is NULL, the local  address in _this will be set to NULL.
 *
 *  @param _this the pointer of CxSock 
 *  @param laddr the pointer of CxSockAddr, specifies the local socket address
 *  @return Always return RC_OK.
 */
RCODE cxSockSetLaddr(CxSock _this, CxSockAddr laddr)
{
	if (_this->laddr_ != NULL)
		cxSockAddrFree(_this->laddr_);

	_this->laddr_ = cxSockAddrDup(laddr);

	return RC_OK;
}

/** @brief  CxSockAccept - access a connection on a socket
 *
 *  This function is used with CX_SOCK_SERVER socket type.  It extracts the first connection request on the queue of pending connections, creates a new connected socket with mostly the same properties as _this, and allocates a new socket descriptor for the socket, which is returned. 
 *
 *  @param _this the pointer of CxSock 
 *  @return On success, CxSockAccept returns a CxSock pointer that is a descriptor for the accept socket.
 *          Otherwise, NULL is returned.
 */
CxSock cxSockAccept(CxSock _this)
{
	CxSock stream;
	CxSockAddr raddr;
	fd_set rset;
	int n;
    	struct sockaddr_in  addr;
	UINT32 addrlen;
	SOCKET s;
	UINT16 port;
	char* ipaddr;
	/*u_long nonblock;*/

	if (!_this)
		return NULL; 
	if (_this->type_ != CX_SOCK_SERVER)
		return NULL;

	FD_ZERO(&rset);
	FD_SET(_this->sockfd_, &rset);

	n = select(_this->sockfd_ + 1, &rset, NULL, NULL, NULL/*blocking*/);

	addrlen = sizeof(addr);
	if (FD_ISSET(_this->sockfd_, &rset)) {
		s = accept(_this->sockfd_, (struct sockaddr *)&addr, &addrlen);
		if (s == INVALID_SOCKET) {
			TCRPrint(1, "<cxSockAccept>: accept() error.\n");
			return NULL;
		}
/*
#if defined(_WIN32)
		nonblock = 1;
		ioctlsocket(s, FIONBIO, &nonblock);
#else
		int flags;
		if ((flags = fcntl(s, F_GETFL, 0)) < 0) return NULL;
		flags |= O_NONBLOCK;
		if (fcntl(s, F_SETFL, flags) < 0) return NULL;
#endif
*/
	}

	/*stream = cxSockNew(CX_SOCK_STREAM_S, NULL); modified by tyhuang */
	stream = cxSockNew(CX_SOCK_STREAM_S, cxSockGetLaddr(_this));
	cxSockSetSock(stream, s);

	port = ntohs((UINT16)addr.sin_port);
	ipaddr = inet_ntoa(addr.sin_addr);
	raddr = cxSockAddrNew(ipaddr, port);
	cxSockSetRaddr(stream, raddr);
	cxSockAddrFree(raddr);

	return stream;
}
				
/** @brief  CxSockConnect - initiate a connection on a socket
 *
 *  This function is used with CX_SOCK_STREAM_C socket type. The CxSockConnect function attempts to make a connection to the socket specified by the raddr parameter.
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param raddr the pointer of CxSockAddr specifies the remote address.
 *  @return On success, RC_OK is returned.
 *          Otherwise, RC_ERROR is returned.
 */

RCODE cxSockConnect(CxSock _this, CxSockAddr raddr)
{
	int result;

	int n;
	int error;
	UINT32 len;
	fd_set rset, wset;

	if (!_this)
		return RC_ERROR; 
	if (_this->type_ != CX_SOCK_STREAM_C)
		return RC_ERROR;

	result = connect(_this->sockfd_, (struct sockaddr *)&raddr->sockaddr_, sizeof(struct sockaddr));
	
	if (result == SOCKET_ERROR) {
		error = getError();
		/* have to take care of reentrant conditions */
		if( error != EINPROGRESS )
			switch ( error ){
				case EWOULDBLOCK:
				case EALREADY:
				case EINVAL:
					break;
				default:
					return RC_ERROR;
			}
		else {
			/* connect still in progress */
			FD_ZERO(&rset);
			FD_SET(_this->sockfd_, &rset);
			wset = rset;

			n = select(_this->sockfd_ + 1, &rset, &wset, NULL, NULL/*blocking*/);
			
			if (n == 0) {
				/* timeout */
				closesocket(_this->sockfd_);
				return RC_ERROR;
			} 

			if (FD_ISSET(_this->sockfd_, &rset) || FD_ISSET(_this->sockfd_, &wset)) {
				len = sizeof(error);
				if (getsockopt(_this->sockfd_, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
					/* Solaris pending error */
					return RC_ERROR;
			} else
				/* other errors */
				return RC_ERROR;
		}

	}
	cxSockSetRaddr(_this, raddr);

	/* connected */
	return RC_OK;
}


/* This is the original version for cxSockConnect(), and had been modify in above 
 * Never use cxSockConnect_bad_version_mark_960518()
*/
/*
static RCODE cxSockConnect_bad_version_mark_960518(CxSock _this, CxSockAddr raddr)
{
	int result;

#ifndef ICL_NONBLOCK
	int n;
	int error;
	UINT32 len;
	fd_set rset, wset;
#endif

	if (!_this)
		return RC_ERROR; 
	if (_this->type_ != CX_SOCK_STREAM_C)
		return RC_ERROR;

	result = connect(_this->sockfd_, (struct sockaddr *)&raddr->sockaddr_, sizeof(struct sockaddr));
	
	if (result == SOCKET_ERROR) {
#ifdef ICL_NONBLOCK
		switch ( getError() ) {
			case EWOULDBLOCK:
			case EALREADY:
			case EINVAL:
				break;
			default:
				return RC_ERROR;
		}
#else
		/// * have to take care of reentrant conditions * /
		error = getError();

		if (error != EINPROGRESS)
			return RC_ERROR;
		else {
			/// * connect still in progress 
			FD_ZERO(&rset);
			FD_SET(_this->sockfd_, &rset);
			wset = rset;

			n = select(_this->sockfd_ + 1, &rset, &wset, NULL, NULL); /// *blocking * /
			
			if (n == 0) {
				/// * timeout * /
				closesocket(_this->sockfd_);
				return RC_ERROR;
			} 

			if (FD_ISSET(_this->sockfd_, &rset) || FD_ISSET(_this->sockfd_, &wset)) {
				len = sizeof(error);
				if (getsockopt(_this->sockfd_, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
					/// * Solaris pending error * /
					return RC_ERROR;
			} else
				/// * other errors * /
				return RC_ERROR;
		}
#endif

	}
	cxSockSetRaddr(_this, raddr);

	/// * connected * /
	return RC_OK;
}
*/

/** @brief  CxSockSend - send messages on a socket
 *
 *  This function is used with CX_SOCK_STREAM_C and CX_SOCK_STREAM_S socket types. The CxSockSend function send message that buf parameter pointed to on the socket described in _this parameter. The length of the message is given by len parameter.
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to the buffer containing the message to send.
 *  @param len the length of the message in bytes.
 *  @return On success, CxSockSend function returns the number of characters sent.
 *          Otherwise, -1 is returned.
 */
/* Fun: cxSockSend
 *
 * Desc: Send 'len' of bytes from buffer.
 *
 * Ret: Return number of bytes sent, -1 on error.
 */
int cxSockSend(CxSock _this, const char* buf, int len)
{
	int n, nsent = 0;
	int error = 0;
	int result;
	fd_set wset;
	fd_set xset;
	struct timeval tv;
	int err;

	if (!_this || !buf)
		return -1;
	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ == INVALID_SOCKET)
		return -1;

	for (nsent = 0; nsent < len; nsent += n) {
		FD_ZERO(&wset);
		FD_SET(_this->sockfd_, &wset);

		FD_ZERO(&xset);
		FD_SET(_this->sockfd_, &xset);

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		result = select(_this->sockfd_ + 1, NULL, &wset, &xset, NULL); /*blocking*/

		if (result == SOCKET_ERROR) {
			error = 1;
			break;
		}

		n = send(_this->sockfd_, buf + nsent, len - nsent, 0/*no flags*/);

		err = getError();
      
		/* note that errno cannot be EWOULDBLOCK since select()
                 * just told us that data can be written.
		 */
		if (n == SOCKET_ERROR || n == 0) {
			error = 1;
			break;
		}
	}

	if (error)
		return -1;
	else
		return nsent;
}

/** @brief  CxSockRecv - receive messages from a socket
 *
 *  This function is used with CX_SOCK_STREAM_C and CX_SOCK_STREAM_S socket types. The CxSockRecv function receives message from the socket described in _this parameter. 
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to a buffer where the message should be placed.
 *  @param len the size of the buffer pointed to by the buffer parameter.
 *  @param timeout specifies the maximun time in milliseconds (1/10^3) to wait for messages. When the timeout parameter is -1, CxSockRecv blocks util one message arrived.
 *  @return On success, CxSockRecv function returns the length of the message in bytes.
 *          Otherwise, returns -1 if error occur, returns 0 if connection closed by peer, returns -2 if timeout.
 */
/* Fun: cxSockRecvEx
 *
 * Desc: Receive 'len' of bytes into buffer. Wait for $timeout msec.
 *	 Set $timeout==-1 for blocking mode.
 *
 * Ret: Return number of byets actually received when success, 
 *      0, connection closed.
 *      -1, error, 
 *      -2, timeout.
 */
int cxSockRecvEx(CxSock _this, char* buf, int len, long timeout)
{
	int n, nrecv = 0;
	int ret = 0;
	int result;
	fd_set rset;
	struct timeval t;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ == INVALID_SOCKET)
		return -1;

	for (nrecv = 0; nrecv < len; nrecv += n) {
		FD_ZERO(&rset);
		FD_SET(_this->sockfd_, &rset);

		if( timeout<0 ) /* blocking mode */
			result = select(_this->sockfd_ + 1, &rset, NULL, NULL, NULL);
		else {
			t.tv_sec = timeout/1000;
			t.tv_usec = (timeout%1000)*1000;
			result = select(_this->sockfd_ + 1, &rset, NULL, NULL, &t);
		}
		/* Acer Modify SipIt 10 */
		/* should check if result ==0 Timeout */
		if ((result == SOCKET_ERROR)) {
			ret = -1;
			break;
		}
		else if ( result == 0 )  {
			ret = -2;
			break;
		}

		n = recv(_this->sockfd_, buf + nrecv, len - nrecv, 0/*no flags*/);

		if (n == SOCKET_ERROR) {
			ret = -1;
			break;
		}

		if (n == 0) { /* connection closed */
			ret = 0;

			/* received data should be delivered to caller,
			   so, nrecv should remain its value. */
			/* nrecv = 0; */
			break;
		}
	}

	/* modified by tkhung 2004-10-19
	if (ret<0)
		return ret;
	else
		return nrecv;
	*/
		
	if ( nrecv > 0)
                return nrecv;
        else
                return ret;
}

/*
 *  RFC: 2126 (ISO Transport Service on top of TCP (ITOT))
 * 
 *	A TPKT consists of two part:
 *	
 *  - a Packet Header
 *  - a TPDU.
 *
 *  The format of the Packet Header is constant regardless of the type of
 *  TPDU. The format of the Packet Header is as follows:
 *
 *  +--------+--------+----------------+-----------....---------------+
 *  |version |reserved| packet length  |             TPDU             |
 *  +----------------------------------------------....---------------+
 *  <8 bits> <8 bits> <   16 bits    > <       variable length       >
 *
 *  where:
 *
 *  - Protocol Version Number
 *    length: 8 bits
 *    Value:  3
 *
 *  - Reserved
 *    length: 8 bits
 *    Value:  0
 *
 *  - Packet Length
 *    length: 16 bits
 *    Value:  Length of the entire TPKT in octets, including Packet
 *            Header
 */

/** @brief  CxSockTpktSend - send messages with TPKT header on a socket
 *
 *  This function is used with CX_SOCK_STREAM_C and CX_SOCK_STREAM_S socket types. The CxSockTpktSend function send message that buf parameter pointed to on the socket described in _this parameter. The length of the message is given by len parameter. Before send the message, cxSockTpktSend send 4 bytes TPKT header on the sokcet.
 *
 *  The data format is:
 *    +--------+--------+----------------+-----------....---------------+
 *    |version |reserved| packet length  |             TPDU(buf)        |
 *    +----------------------------------------------....---------------+
 *    <8 bits> <8 bits> <   16 bits    > <       variable length (len)  >
 *
 *    - Protocol Version Number
 *      length: 8 bits
 *      Value:  3
 *
 *    - Reserved
 *      length: 8 bits
 *      Value:  0
 *
 *    - Packet Length
 *      length: 16 bits
 *      Value:  Length of the entire TPKT in octets, including Packet
 *
 *  [[ TPKT: see RFC2126 for more information. ]]
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to the buffer containing the message to send.
 *  @param len the length of the message in bytes.
 *  @return On success, CxSockTpktSend function returns the total number of characters sent(include 4 bytes TPKT header).
 *          Otherwise, -1 is returned.
 */

/* Fun: cxSockTpktSend
 *
 * Desc: Send data with TPKT header.
 *
 * Ret: Return length of len + 4, if success. Otherwise, return -1.
 */
int cxSockTpktSend(CxSock _this, const char* buf, int len)
{
	int nSent, nTpktSent;
	unsigned char tpktHeader[4];

	if (!_this || !buf)
		return -1;

	tpktHeader[0] = 3;		/* Protocol Version Number, always 3 */
	tpktHeader[1] = 0;		/* Reserved, 0 */
	tpktHeader[2] = (len + 4) >> 8;	/* high byte of length */
	tpktHeader[3] = (len + 4);	/* low byte of length */

	if ((nTpktSent = cxSockSend(_this, (char *)tpktHeader, 4)) != -1) {
		if (nTpktSent == 4) {
			if ((nSent = cxSockSend(_this, buf, len)) != -1)
				nTpktSent = (nSent == len) ? (nTpktSent + nSent) : -1;
			else 
				nTpktSent = -1;
		} else 
			nTpktSent = -1;
	}

	return nTpktSent;
}

/** @brief  CxSockTpktRecv - receive messages with TPKT header from a socket
 *
 *  This function is used with CX_SOCK_STREAM_C and CX_SOCK_STREAM_S socket types. The CxSockTpktSend function receives TPKT message from the socket described in _this parameter and places the TPDU part of TPKT message into buf parameter. The length of TPDU message is defined in TPKT header.
 *
 *  The TPKP data format is:
 *    +--------+--------+----------------+-----------....---------------+
 *    |version |reserved| packet length  |             TPDU(buf)        |
 *    +----------------------------------------------....---------------+
 *    <8 bits> <8 bits> <   16 bits    > <       variable length (len)  >
 *
 *    - Protocol Version Number
 *      length: 8 bits
 *      Value:  3
 *
 *    - Reserved
 *      length: 8 bits
 *      Value:  0
 *
 *    - Packet Length
 *      length: 16 bits
 *      Value:  Length of the entire TPKT in octets, including Packet
 *
 *  [[ TPKT: see RFC2126 for more information. ]]
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to a buffer where the TPKT message should be placed.
 *  @param len invalid parameter.
 *  @return On success, CxSockTpktRecv function returns the length of the TPDU message in bytes.
 *          Otherwise, returns -1 if error occur, returns 0 if connection closed by peer.
 */
/* Fun: cxSockTpktRecv
 *
 * Desc: Receive data with TPKT header. Buffer will not include TPKT header.
 *
 * Ret: Return length of len + 4, if success. Otherwise, return -1.
 */
int cxSockTpktRecv(CxSock _this, char* buf, int len)
{
	int nRecv, nTPDU;
	unsigned char tpktHeader[4];

	if (!_this || !buf)
		return -1;

	if ((nRecv = cxSockRecv(_this, (char*)tpktHeader, 4)) != -1) {
		if (nRecv == 4) {
			nTPDU = (int) (( tpktHeader[2]) << 8)+tpktHeader[3];
			nTPDU -= 4;
			if ((nRecv = cxSockRecv(_this, buf, nTPDU)) != -1) {
				if (nRecv != nTPDU) nRecv = (nRecv > 0) ? -1 : 0;
			}
		} else 
			nRecv = (nRecv > 0) ? -1 : 0;
	} else 
		return 0;

	return nRecv;
}

/** @brief  CxSockSendto - send messages on a socket
 *
 *  This function is used with CX_SOCK_DGRAM socket types. The CxSockSendto function send message placed in put parameter to remote address raddr on the socket described in _this parameter. The length of the message is given by len parameter.
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to the buffer containing the message to send.
 *  @param len the length of the message in bytes.
 *  @param raddr specifies the destination address.
 *  @return On success, CxSockSendto function returns the number of characters sent.
 *          Otherwise, -1 is returned.
 */
int cxSockSendto(CxSock _this, const char* buf, int len, CxSockAddr raddr)
{
	int nsent;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;
 
	nsent = sendto( _this->sockfd_, 
			buf, 
			len, 
			0, /* No flags */
			(struct sockaddr*)&(raddr->sockaddr_),
			sizeof(raddr->sockaddr_));

	if(nsent == SOCKET_ERROR) 
		return -1;

	return nsent;
}

/** @brief  CxSockRecvfrom - receive messages from a socket
 *
 *  This function is used with CX_SOCK_DGRAM socket types. The CxSockRecfrom function receives message from the socket described in _this parameter. The source address of the message is filled into _this parameter.
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to a buffer where the message should be placed.
 *  @param len the size of the buffer pointed to by the buffer parameter.
 *  @return On success, CxSockRecvfrom function returns the length of the message in bytes.
 *          Otherwise, -1 is returned.
 */
/* Fun: cxSockRecvfrom
 *
 * Desc: Receive 'len' of bytes into buffer.
 *       Side effect - source address updated.
 *
 * Ret: Return number of byets actually received, -1 on error.
 */
int cxSockRecvfrom(CxSock _this, char* buf, int len)
{
	CxSockAddr raddr;
	int nrecv;
	UINT32 addrlen;
	struct sockaddr_in addr;
	UINT16 port;
	char* ipaddr;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;

	addrlen = sizeof(addr);
	nrecv = recvfrom( _this->sockfd_, 
			buf, 
			len, 
			0, /* no flags */
			(struct sockaddr*)&addr, 
			&addrlen);

	if (nrecv == SOCKET_ERROR) 
		return -1;

	port = ntohs((UINT16)addr.sin_port);
	ipaddr = inet_ntoa(addr.sin_addr);
	raddr = cxSockAddrNew(ipaddr, port);
	cxSockSetRaddr(_this, raddr);
	cxSockAddrFree(raddr);

	return nrecv;
}
