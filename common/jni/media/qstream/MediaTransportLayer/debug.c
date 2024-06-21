/*
 * FILE:    debug.c 
 * AUTHORS: Isidor Kouvelas 
 *          Colin Perkins 
 *          Mark Handley 
 *          Orion Hodson 
 *          Jerry Isdale
 * 
 * $Revision: 1.7 $ 
 * $Date: 2005/05/03 10:59:49 $
 * 
 * Copyright (c) 1995-2000 University College London All rights reserved.
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

#include "config.h"
#include "config_unix.h"
#include "debug.h"


void debug_msg(const char *fmt, ...)
{
#ifdef WIN32
#ifdef _DEBUG
    char printf_buf[4092];
    va_list args;
    int printed;
   
    memset(printf_buf, 0, 4092);
    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);
    OutputDebugStringA(printf_buf);
#endif
#else
    char printf_buf[1024];
    va_list args;
    int printed;
   
    memset(printf_buf, 0, 1024);
    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);
    //printf(printf_buf);
#endif
}


void 
_dprintf(const char *format,...)
{
#ifdef DEBUG
	va_list		ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
#else
			UNUSED        (format);
#endif				/* DEBUG */
}

/**
 * debug_dump:
 * @lp: pointer to memory region.
 * @len: length of memory region in bytes.
 *
 * Writes a dump of a memory region to stdout.  The dump contains a
 * hexadecimal and an ascii representation of the memory region.
 *
 **/
void 
debug_dump(void *lp, long len)
{
	char           *p;
	long		i        , j, start;
	char		Buff      [81];
	char		stuffBuff [10];
	char		tmpBuf    [10];

	_dprintf("Dump of %ld=%lx bytes\n", len, len);
	start = 0L;
	while (start < len) {
		/* start line with pointer position key */
		p = (char *)lp + start;
		sprintf(Buff, "%p: ", p);

		/* display each character as hex value */
		for (i = start, j = 0; j < 16; p++, i++, j++) {
			if (i < len) {
				sprintf(tmpBuf, "%X", ((int)(*p) & 0xFF));

				if (strlen((char *)tmpBuf) < 2) {
					stuffBuff[0] = '0';
					stuffBuff[1] = tmpBuf[0];
					stuffBuff[2] = ' ';
					stuffBuff[3] = '\0';
				} else {
					stuffBuff[0] = tmpBuf[0];
					stuffBuff[1] = tmpBuf[1];
					stuffBuff[2] = ' ';
					stuffBuff[3] = '\0';
				}
				strcat(Buff, stuffBuff);
			} else {
				strcat(Buff, "   ");
			}
			if (j == 7)	/* space between groups of 8 */
				strcat(Buff, " ");
		}

		strcat(Buff, "  ");

		/* display each character as character value */
		for (i = start, j = 0, p = (char *)lp + start;
		     (i < len && j < 16); p++, i++, j++) {
			if (((*p) >= ' ') && ((*p) <= '~'))	/* test displayable */
				sprintf(tmpBuf, "%c", *p);
			else
				sprintf(tmpBuf, "%c", '.');
			strcat(Buff, tmpBuf);
			if (j == 7)	/* space between groups of 8 */
				strcat(Buff, " ");
		}
		_dprintf("%s\n", Buff);
		start = i;	/* next line starting byte */
	}
}

