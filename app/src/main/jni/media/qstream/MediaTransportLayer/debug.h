/*
 * FILE:    debug.h
 * PROGRAM: RAT
 * AUTHOR:  Isidor Kouvelas + Colin Perkins + Mark Handley + Orion Hodson
 * 
 * $Revision: 1.1 $
 * $Date: 2005/06/09 13:53:08 $
 *
 * Copyright (c)      2005 University of Glasgow
 * Copyright (c) 1995-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include "../GlobalDefine.h"

#define UNUSED(x)	(void) x

/*----------------------------------------------------------------------------------------------------*/
#ifndef WIN32
#define TL_DEBUG_MODE        1
#endif
/* -----------------------------
   Media Transport Layer Debug Macro
   -----------------------------      
 */
#define DBG_MASK1           0xFF        /* 1111 1111 */
#define DBG_MASK2           0xFE        /* 1111 1110 */
#define DBG_MASK3           0xFC        /* 1111 1100 */
#define DBG_MASK4           0xF8        /* 1111 1000 */
#define DBG_MASK5           0xF0        /* 1111 0000 */

/* Define debugging levels */
#define DBG_LEVEL0          0x00        /* 0000 0000  / None */
#define DBG_LEVEL1          0x01        /* 0000 0001  / Show MASK 1 */
#define DBG_LEVEL2          0x03        /* 0000 0011  / Show MASK 1, 2 */
#define DBG_LEVEL3          0x07        /* 0000 0111  / Show MASK 1, 2, 3 */
#define DBG_LEVEL4          0x0F        /* 0000 1111  / Show MASK 1, 2, 3, 4 */
#define DBG_LEVEL5          0x1F        /* 0001 1111  / Show MASK 1, 2, 3, 4, 5 */
#ifdef WIN32
#ifdef _DEBUG
#define TL_DEBUG_MSG(debug, mask, fmt ,...) debug_msg(fmt,__VA_ARGS__);
#else
#define TL_DEBUG_MSG(debug, mask, fmt ,...)
#endif
#else
#if ( (TL_DEBUG_MODE==1) && (LINUX_DEBUGMSG==1) )
#define TL_DEBUG_MSG(debug, mask, fmt ,arg...) if(debug & mask){ printf(fmt,##arg); }
#else
#define TL_DEBUG_MSG(debug, mask, fmt ,arg...)
#endif
#endif

#define TL_MAIN			    		DBG_LEVEL0
#define TL_TFRC_SNDR	            DBG_LEVEL0
#define TL_TFRC_RCVR	            DBG_LEVEL0
#define TL_SELFTEST		    		DBG_LEVEL0
#define TL_TRANSPORT          	    DBG_LEVEL1
#define TL_SENDING_BUF              DBG_LEVEL0
#define TL_PLAYOUT_BUF      	    DBG_LEVEL0
#define TL_RFC3984                  DBG_LEVEL0
#define TL_RTP                      DBG_LEVEL0
#define TL_RTCP	                    DBG_LEVEL0
#define TL_RFC4585                  DBG_LEVEL1
#define TL_NETUDP                   DBG_LEVEL0
/*----------------------------------------------------------------------------------------------------*/



/*
#define debug_msg	_dprintf("[pid/%d/%p +%d %s] ", getpid(), pthread_self(), __LINE__, __FILE__), _dprintf
*/

//#define debug_msg printf

#define DEBUG_LEVEL 1

void debug_msg(const char *format, ...);
void _dprintf(const char *format, ...);
void debug_dump(void*lp, long len);

#endif
