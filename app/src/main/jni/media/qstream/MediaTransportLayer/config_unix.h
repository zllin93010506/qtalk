/*
 * config-unix.h
 *
 * Unix specific definitions and includes for RAT.
 *
 * Copyright (c) 2001-2003 University of Southern California
 * Copyright (c) 1995-2001 University College London
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
 * $Revision: 1.1 $
 * $Date: 2005/06/09 13:53:08 $
 */

#ifndef _CONFIG_UNIX_H
#define _CONFIG_UNIX_H

/* A lot of includes here that should all probably be in files where they   */
/* are used.  If anyone ever has the time to reverse the includes into      */
/* the files where they are actually used, there would be a couple of pints */
/* in it.                                                                   */
#ifdef WIN32
#include <winsock2.h> //for timeval
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // <stat.h>
#include <windows.h> 
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include "pthread.h"
#else
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef WIN32
#include <limits.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>  
#include <string.h>
#else
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <ctype.h>

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>  
#include <string.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>  
#endif 

#ifdef HAVE_SYS_SOCK_IO_H
#include <sys/sockio.h>
#endif

#ifdef WIN32
#include <time.h>   // <sys/time.h>
#include <io.h>    // <sys/uio.h> <sys/ioctl.h>
#include <fcntl.h> // <sys/fcntl.h>
#include <process.h>
#include "stdint.h"
#include <sys/types.h>     
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/param.h>
#include <fcntl.h> // <sys/fcntl.h> // Frank modified for PX2
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#endif
#ifdef HAVE_IPv6

#ifdef HAVE_NETINET6_IN6_H
/* Expect IPV6_{JOIN,LEAVE}_GROUP in in6.h, otherwise expect */
/* IPV_{ADD,DROP}_MEMBERSHIP in in.h                         */
#include <netinet6/in6.h>
#else
#ifdef WIN32
#else
#include <netinet/in.h>
#endif
#endif /* HAVE_NETINET_IN6_H */

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#else
#error  No definition of IPV6_ADD_MEMBERSHIP
#endif /* IPV6_JOIN_GROUP     */
#endif /* IPV6_ADD_MEMBERSHIP */

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef  IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#else
#error  No definition of IPV6_LEAVE_GROUP
#endif  /* IPV6_LEAVE_GROUP     */
#endif  /* IPV6_DROP_MEMBERSHIP */

#endif /* HAVE_IPv6 */

#ifdef GETTOD_NOT_DECLARED
int gettimeofday(struct timeval *tp, void * );
#endif

#ifdef KILL_NOT_DECLARED
int kill(pid_t pid, int sig);
#endif
#define MAX_PATH 256
typedef unsigned char  ttl_t;
typedef int     fd_t;

#ifndef TRUE
#define FALSE	0
#define	TRUE	1
#endif /* TRUE */

#define USERNAMELEN	8
#ifndef WIN32
#define max(a, b)	(((a) > (b))? (a): (b))
#define min(a, b)	(((a) < (b))? (a): (b))
#endif
#endif /* _CONFIG_UNIX_H */

