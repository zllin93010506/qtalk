/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_trace.h
 *
 * $Id: cm_trace.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#ifndef CM_TRACE_H
#define CM_TRACE_H

#include "cm_def.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define	TCRMAXBUFLEN	10000
#define	TRACE_ON	1
#define	TRACE_OFF	0

typedef int		TraceFlag;

/**
 * @brief Turn on and initialize the tracer 
 */
/* TCRBegin()
	By default, all loggers are OFF. trace_level = 0
*/
void		TCRBegin(void);

/**
 * @brief Turn off the tracer
 */
void		TCREnd(void);

/**
 * @brief Set the trace level of the tracer
 */
RCODE		TCRSetTraceLevel(TraceLevel trace_level);

/**
 * @brief Get the setting of the trace level of the tracer
 */
TraceLevel	TCRGetTraceLevel(void);	

/**
 * @brief Set the message callback function of the tracer 
 */	
/* TCRSetMsgCB()
	TCR will callback #msgCB when it gets any message.
	Set #msgCB=NULL to disable callback function.
*/
RCODE		TCRSetMsgCB(void(*msgCB)(const char*));

/**
 * @brief Turn on the socket logger 
 */
/* TCRSockLoggerON()
	Log to remote via UDP.
*/
RCODE		TCRSockLoggerON(const char* raddr, UINT16 rport);

/**
 * @brief Turn off the socket logger 
 */
RCODE		TCRSockLoggerOFF(void);

/**
 * @brief Turn on the socket SockServer to allow tracer to receive message
 *        from UDP packetand log to any active logger
 */
/* TCRSockServerON()
	when SockServer is ON, tracer will receive UDP packet from
	#port, and log it to any logger that is active. 
*/
RCODE		TCRSockServerON(UINT16 port);

/**
 * @brief Turn off the SockSever
 */
RCODE		TCRSockServerOFF(void);

/**
 * @brief Turn on the console logger 
 */
RCODE		TCRConsoleLoggerON(void);

/**
 * @brief Turn off the console logger
 */
RCODE		TCRConsoleLoggerOFF(void);

/**
 * @brief Turn on the file logger 
 */
RCODE		TCRFileLoggerON(const char* fname);

/**
 * @brief Turn off the file logger
 */
RCODE		TCRFileLoggerOFF(void);

/**
 * @brief Print trace messages
 */
/* TCRPrint()
	this function acts just like printf().
*/
int		TCRPrint(int level, const char* format, ...);

#ifdef  __cplusplus
}
#endif

#endif /* CM_TRACE_H */
