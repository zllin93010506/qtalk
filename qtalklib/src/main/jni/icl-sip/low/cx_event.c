/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_event.c
 *
 * $Id: cx_event.c,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#include <string.h>
#include <stdlib.h>
#ifdef UNIX
#include <sys/time.h>
#include <sys/select.h>
#endif
#include "cx_event.h"
#include <common/cm_trace.h>

typedef struct {
	int		valid; /* 0=invalid; 1=valid */
	int		event;
	CxEventCB	cb;
	CxSock		sock;
	void*		context;
} EventNode;

struct cxEventObj
{
	EventNode	nodes[FD_SETSIZE];
	int		maxi; /* max index of nodes */
	SOCKET		maxfd; /* max file descriptor value in nodes */
	fd_set		selectRd;
	fd_set		selectWr;
	fd_set		selectEx;
};

/** @brief  cxEventNew - allocate memory for CxEvent and initialize it.
 *
 *  The cxEventNew function allocate memory for CxEvent and initialize it.
 *
 *  @param _this the pointer of CxSock specifies the socket.
 *  @param buf points to a buffer where the message should be placed.
 *  @param len the size of the buffer pointed to by the buffer parameter.
 *  @return On success, cxEventNew function returns a event-handler object descriptor.
 *          Otherwise, NULL is returned.
 */
CxEvent cxEventNew(void)
{
	CxEvent _this;
	int i;

	_this = (CxEvent)malloc(sizeof *_this);
	if (_this == NULL) return NULL;

	for (i=0; i<FD_SETSIZE; i++) {
		_this->nodes[i].valid = 0;
		_this->nodes[i].event = CX_EVENT_UNKNOWN;
		_this->nodes[i].cb = NULL;
		_this->nodes[i].sock = NULL;
		_this->nodes[i].context = NULL;
	}

	_this->maxi = -1;
	_this->maxfd = INVALID_SOCKET;

	FD_ZERO(&_this->selectRd);
	FD_ZERO(&_this->selectWr);
	FD_ZERO(&_this->selectEx);

	return _this;
}

/** @brief  cxEventFree - free the resrouce used in event-handler object.
 *
 *  The CxEventFree function free the resrouce that _this parameter associated with.
 *
 *  @return Always returns RC_OK.
 */
RCODE cxEventFree(CxEvent _this)
{
	if (_this!=NULL) free(_this);

	return RC_OK;
}

/** @brief  CxEventRegister -  set interesting events for a socket descriptor to event-handler object.
 *
 *  The CxEventRegister function sets the inseresting events associate with the socket descriptor sock to the event-handler object. 
 *
 *  @param _this the event-handler object.
 *  @param sock  the socket descriptor.
 *  @param event the events that want to be reported. The event value is formed by logically ORing the following values, defined in the cx_event.h file:
 *                CX_EVENT_RD  there is data to read.
 *                CX_EVENT_WR  ready for write.
 *                CX_EVENT_EX  exception occur.
 *		If event is set to CX_EVENT_UNKNOWN, all events that have been register will be cleared for re-register case.
 *  @param cb the user-defined callback function, called when the events specified in event parameter occur.
 *		void (*CxEventCB)(CxSock sock, CxEventType event, int err, void* context);
 *  @param context the argument value for cb.
 *  @return On success, RC_OK is returned.
 *          Otherwise, RC_ERROR is returned.
 */
RCODE cxEventRegister(CxEvent _this, CxSock sock, CxEventType event, CxEventCB cb, void* context)
{
	SOCKET fd;
	int i, j;

	if (_this == NULL || sock == NULL || !event || !cb)
		return RC_ERROR;

	fd = cxSockGetSock(sock);
	j = _this->maxi + 1;

	for (i=0; i<=_this->maxi; i++)
		if (_this->nodes[i].valid)
			if (cxSockGetSock(_this->nodes[i].sock) == fd) {
				/* re-register */
				j = i;
				break;
			} else
				continue;
		else
			if (i < j) j = i;

	if (j >= FD_SETSIZE) 
		return RC_ERROR;
	
	/* registration */
	if (event & CX_EVENT_RD) {
		FD_SET(fd, &_this->selectRd);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectRd);

	if (event & CX_EVENT_WR) {
		FD_SET(fd, &_this->selectWr);
 		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectWr);

	if (event & CX_EVENT_EX) {
		FD_SET(fd, &_this->selectEx);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectEx);

	if (j > _this->maxi) _this->maxi = j;
	_this->nodes[j].valid=1;
	_this->nodes[j].event = event;
	_this->nodes[j].cb = cb;
	_this->nodes[j].sock = sock;
	_this->nodes[j].context = context;
    
	return RC_OK;
}


/** @brief  CxEventUnregister -  unset a socket descriptor for events handling from event-handler object.
 *
 *  The CxEventRegister function clears the events handling information associated with socket descriptor sock from the event-handler object. This function place the context data back in the context parameter, where user input the context data when called CxEventRegister function.
 *
 *  @param _this the event-handler object.
 *  @param sock  the socket descriptor.
 *  @param context points to a pointer where the context data should be placed.
 *  @return On success, RC_OK is returned.
 *          Otherwise, RC_ERROR is returned.
 */
RCODE cxEventUnregister(CxEvent _this, CxSock sock, void** context)
{
	SOCKET fd, sockfd, maxfd;
	int i, index, maxi;
	int found=0;

	if (_this==NULL)
		return RC_ERROR;

	fd = cxSockGetSock(sock);
	maxi = -1;
	maxfd = INVALID_SOCKET;

	for (i=0; i<=_this->maxi; i++)
		if (_this->nodes[i].valid) {
			sockfd = cxSockGetSock(_this->nodes[i].sock);
			index = i;
			if (sockfd == fd) {
				_this->nodes[i].valid = 0;
				_this->nodes[i].event = CX_EVENT_UNKNOWN;
				_this->nodes[i].cb = NULL;
				_this->nodes[i].sock = NULL;
				/* free here ? */
				/*if (_this->nodes[i].context)
					free(_this->nodes[i].context);*/
				if( context )
					*context = _this->nodes[i].context;
				_this->nodes[i].context = NULL;

				FD_CLR(fd, &_this->selectRd);
				FD_CLR(fd, &_this->selectWr);
				FD_CLR(fd, &_this->selectEx);

				sockfd = INVALID_SOCKET;
				index = -1;
				found = 1;
			}
			if (index > maxi) 
				maxi = index;
			if ((maxfd == INVALID_SOCKET) || 
				((sockfd != INVALID_SOCKET) && (sockfd > maxfd)))
				maxfd = sockfd;
		}

	if (!found) 
		return RC_ERROR;

	_this->maxi = maxi;
	_this->maxfd = maxfd;

	return RC_OK;
}

/** @brief  cxEventDispatch - waits for a number of socket descriptors to change status.
 *
 *  The CxEventDispatch function  waits for a number of socket descriptors to change status until times out. For all socket descriptor that has registered in the _this parameter, call ecah callback function association with the socket descriptor if at least one of three event occur. 
 *
 *  @param _this the event-handler object.
 *  @param timeout specifies the maximun time in milliseconds (1/10^3) to wait.
 *  @return On success, the cxEventDispatch function returns the number of detected events.
 *          Otherwise, -1 is returned.
 */
int cxEventDispatch(CxEvent _this, int timeout)
{
	struct timeval tv;
	int numEvents=0;
	SOCKET fd;
	int event;
	int i;
	fd_set rset, wset, xset;
	int retval;

	if (!_this)
		return -1;

	rset = _this->selectRd;
	wset = _this->selectWr;
	xset = _this->selectEx;

	if ( timeout == -1)
		numEvents = select(_this->maxfd+1, &rset, &wset, &xset, NULL);
	else {
		tv.tv_sec = timeout/1000;
		tv.tv_usec = (timeout%1000)*1000;    
		numEvents = select(_this->maxfd+1, &rset, &wset, &xset, &tv);
	}
	if (numEvents == SOCKET_ERROR)
		retval  = -1;
	else
		retval = numEvents;

	/* invoke callbacks if event occurs*/
	event = CX_EVENT_UNKNOWN;
	if ( numEvents > 0 ) {
		for (i=0; i<=_this->maxi; i++) {
			if (_this->nodes[i].valid) {
				fd = cxSockGetSock(_this->nodes[i].sock);
				if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset) || FD_ISSET(fd, &xset)) {
					if ((_this->nodes[i].event & CX_EVENT_RD) && FD_ISSET(fd, &rset) )
						event |= CX_EVENT_RD;
 
					if ( (_this->nodes[i].event & CX_EVENT_WR) && FD_ISSET(fd, &wset) )
						event |= CX_EVENT_WR;

					if ((_this->nodes[i].event & CX_EVENT_EX) && FD_ISSET(fd, &xset) )
						event |= CX_EVENT_EX;
					/* type conversion from int to CxEventType ? */
					_this->nodes[i].cb(_this->nodes[i].sock, (CxEventType)event, 0, _this->nodes[i].context);

					numEvents--;
					if (numEvents == 0) break;
				}
			}
		}
	}

	return retval;
}



