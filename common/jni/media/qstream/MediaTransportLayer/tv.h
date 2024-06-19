/*
 * FILE:     tv.h
 * AUTHOR:   Colin Perkins <csp@csperkins.org>
 * MODIFIED: Ladan Gharai <ladan@isi.edu>
 *
 * Copyright (c) 2001-2003 University of Southern California
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *      This product includes software developed by the University of Southern
 *      California Information Sciences Institute.
 * 
 * 4. Neither the name of the University nor of the Institute may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Revision: 1.16 $
 * $Date: 2005/06/22 00:50:28 $
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
/*
//#if defined(WIN32)
//#include "..\global.h"
//#include <winsock.h>
//#endif
*/
#ifndef __TV_H__
#define __TV_H__

/*
 * Time granularity for this platform (in usecs)
 */
#ifdef HAVE_DARWIN
#define TIME_GRANULARITY  100
#else
#define TIME_GRANULARITY  10000
#endif

/*
 * Type types used.
 */
typedef double    t_sec;   /* Seconds type */
typedef uint32_t  t_usec;  /* Microseconds type */


t_usec   get_local_time(void);
t_usec   get_local_mediatime(void);
t_usec   timeval_to_usec (struct timeval curr_time);


t_sec    tv_diff(struct timeval curr_time, struct timeval prev_time);
t_usec   tv_diff_usec(struct timeval curr_time, struct timeval prev_time);
void     tv_add(struct timeval *ts, t_sec offset_secs);
void	 tv_add_usec(struct timeval *ts, t_usec usecs);
int      tv_gt(struct timeval a, struct timeval b);
int      tv_eq(struct timeval a, struct timeval b);

//void     sleep_until(struct timeval a);

#endif

#ifdef __cplusplus
}
#endif

