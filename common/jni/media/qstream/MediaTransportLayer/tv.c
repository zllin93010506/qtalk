/*
 * FILE:     tv.c
 * AUTHOR:   Colin Perkins <csp@csperkins.org>
 * MODIFIED: Ladan Gharai  <ladan@isi.edu>
 *           Alvaro Saurin <saurin@dcs.gla.ac.uk>
 *
 * Copyright (c) 2001-2005 University of Southern California
 * Copyright (c) 2004-2005 University of Glasgow
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
 * $Revision: 1.37 $
 * $Date: 2005/06/29 00:58:53 $
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
//#include "config.h"
//#include <assert.h>
#include <stdlib.h>
//#include <stdint.h>
//#include <sys/time.h>
*/
#ifdef WIN32
#include "config_unix.h"
#endif

//#include "config.h"
//#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef WIN32
#include <sys/time.h> // Frank enable for PX2
#endif
	
#include "tv.h"

static struct timeval   start_time;		/* The start time of the application           */
static int              start_time_valid = 0;	/* Non-zero if start_time has been initialised */
//static t_usec           random_offset;		/* For the RTP media clock only                */

/*
 * Return a timestamp in micro seconds 
 */
t_usec
get_local_time(void)
{
        struct timeval          curr_time;

        if (!start_time_valid) {
                gettimeofday(&start_time, NULL);
                //random_offset = lrand48(); Mark by Ian Tuan
				//srand(GetTickCount());
				//random_offset = (unsigned int)rand();
				start_time_valid = 1;
			//Eric, test the TFRC wrap-around
			//start_time.tv_sec-=4260;
        }

        gettimeofday(&curr_time, NULL);
	return tv_diff_usec(curr_time, start_time);
}

/*
 * convert currtime to micro seconds 
 */
t_usec
timeval_to_usec (struct timeval curr_time) 
{
        if (!start_time_valid) {
                gettimeofday(&start_time, NULL);
                //random_offset = lrand48(); Mark by Ian Tuan
				//srand(GetTickCount());
				//random_offset = (unsigned int)rand();
                start_time_valid = 1;
			//Eric, test the TFRC wrap-around
			//start_time.tv_sec-=4260;
            //Add by Eric, initialization 
            return 0;
        }
	
	return tv_diff_usec(curr_time, start_time);
}


t_usec
get_local_mediatime(void)
{
        struct timeval          curr_time;

        if (!start_time_valid) {
                gettimeofday(&start_time, NULL);
                //random_offset = lrand48();
                start_time_valid = 1;
			//Eric, test the TFRC wrap-around
			//start_time.tv_sec-=4260;
        }

        gettimeofday(&curr_time, NULL);
        //return ((tv_diff(curr_time, start_time) * 90000) + random_offset);
		//Modified by Ian Tuan because we don't have larand48() function
		return ((tv_diff(curr_time, start_time) * 90000));
}


double 
tv_diff(struct timeval curr_time, struct timeval prev_time)
{
    /* Return (curr_time - prev_time) in seconds */
    t_sec      ct, pt;

    ct = (double) curr_time.tv_sec + (((double) curr_time.tv_usec) / 1000000.0);
    pt = (double) prev_time.tv_sec + (((double) prev_time.tv_usec) / 1000000.0);
    return (ct - pt);
}

t_usec
tv_diff_usec (struct timeval curr_time, struct timeval prev_time)
{	
	/* Return curr_time - prev_time in usec - i wonder if these numbers will be too big?*/
	t_usec  tmp, tmp1, tmp2;

	/* We return an unsigned, so fail is prev_time is later than curr_time */
	//assert(curr_time.tv_sec >= prev_time.tv_sec);
	if (curr_time.tv_sec == prev_time.tv_sec) {
	//	assert(curr_time.tv_usec >= prev_time.tv_usec);
	}

	tmp1 = (curr_time.tv_sec - prev_time.tv_sec)*((uint32_t)1000000);
        tmp2 = curr_time.tv_usec - prev_time.tv_usec;
        tmp  = tmp1+tmp2;

	return tmp;
}

void 
tv_add(struct timeval *ts, t_sec offset_secs)
{
	unsigned long offset = (unsigned long) (offset_secs * 1000000.0);

        ts->tv_usec += offset;
        while (ts->tv_usec >= 1000000) {
                ts->tv_sec++;
                ts->tv_usec -= 1000000;
        }
}

void
tv_add_usec(struct timeval *ts, t_usec usecs)
{
        unsigned long offset = (unsigned long) (usecs);

        ts->tv_usec += offset;
        while (ts->tv_usec >= 1000000) {
                ts->tv_sec++;
                ts->tv_usec -= 1000000;
        }
}


int 
tv_gt(struct timeval a, struct timeval b)
{
        /* Returns (a>b) */
        if (a.tv_sec > b.tv_sec) {
                return 1;
        }
        if (a.tv_sec < b.tv_sec) {
                return 0;
        }
//        assert(a.tv_sec == b.tv_sec);
        return a.tv_usec > b.tv_usec;
}

int
tv_eq(struct timeval a, struct timeval b)
{
        return (a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec ? 1 : 0);
}

#ifdef HAVE_DARWIN
#define BUSY_WAIT_LIMIT 100	/* Maximum time to busy wait, in microseconds */
#else
#define BUSY_WAIT_LIMIT 1000
#endif



#ifdef __cplusplus
}
#endif

