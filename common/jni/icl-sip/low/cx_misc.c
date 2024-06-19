/** @file  sleeping()
 *  
 *  @author  yjliao
 */
/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_misc.c
 *
 * $Id: cx_misc.c,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#define _POSIX_C_SOURCE 199309L /* struct timespec */

#include "cx_misc.h"

#ifdef UNIX
#include <time.h>

/** @brief  sleeping - Sleep for the specified number of  milliseconds
 *
 *  sleeping delays the execution of the program for at least the number of milliseconds 
 *	specified by the msec parameter.
 *  
 *  [SYSTEM] UNIX only. 
 *
 *  @param msec Specifies the number of milliseconds to sleep
 */
void  sleeping(unsigned int msec)
{
	struct timespec rqtp;
	
	rqtp.tv_sec = msec/1000; 
	rqtp.tv_nsec = (msec%1000)*1000000;/* 10ms */
	nanosleep(&rqtp,NULL);
}
#endif
