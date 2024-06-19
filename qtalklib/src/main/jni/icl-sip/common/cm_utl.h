/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_utl.h
 *
 * $Id: cm_utl.h,v 1.2 2008/09/23 08:22:46 tyhuang Exp $
 */

#ifndef CM_UTL_H
#define CM_UTL_H

#include <stdio.h>
#include <stdlib.h>

#define MAXFIELD 100

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

#define ISASCII(c)	(((c) & ~0x7f) == 0)
#define TOASCII(c)	((c) & 0x7f)

/**
 * @brief Convert integer to string format
 */
/* return string rep of integer i
 * string is in static memory, so it must be copied before another is
 * converted
 */
char*	i2a(int x, char* buf, int buflen);

#ifndef a2i
#define a2i(x)	((x)?atoi(x):0)
#endif

#ifndef a2ui
#define a2ui(x)	((x)?strtoul(x,NULL,10):0)
#endif

/**
 * @brief Remove the whitespace from both beginning and end of a string
 */
char*	trimWS(char * s);

/**
 * @brief Add quote (") to both beginning and end of a string
 */
/* Notice: quote() will not allocate new memory space to store the quoted string.
           User should make sure that memory space of $s is big enough to insert
           additional two bytes. */
char*	quote(char* s);

/**
 * @brief Remove quote (") from both beginning and end of a string if exists
 */
char*	unquote(char* s);

/**
 * @brief Remove anglequote ('<','>') from beginning and end of a string if exists
 */
/* remove '<' '>' */
char* unanglequote(char* s);

/**
 * @brief Check if one certain pattern appears in a string
 */
/* return true if pattern appears in s */
int	strCon(const char* s, const char* pattern);

/**
 * @brief Check case-insensitively if one certain pattern appears in a string
 */
/* case-insensitive version of strCon() */
int	strICon(const char* s, const char* pattern);

/**
 * @brief Case-sensitive string comparison
 */
int strCmp(const char* s1, const char* s2);

/**
 * @brief String case-insensitive compare
 */
/* string case-insensitive compare */
int	strICmp(const char* s1, const char* s2);

/**
 * @brief String case-insensitive n characters compare
 */
/* string case-insensitive n charactors compare */
int	strICmpN(const char* s1, const char* s2, int n);

/**
 * @brief Duplicate a string s
 */
/* strDup, use malloc() to duplicate string s.
   User should use free() to deallocate memory. */
char*	strDup(const char* s);

/**
 * @brief Attach one string to the end of the other string
 */
/* attach string src to string dst */
char*	strAtt(char*dst, const char*src);

/**
 * @brief Copy characters from source string to the destination string
 *        until the first character of the source string which is not 
 *        in the character set good
 */
/* Copy characters from src to dst, return number copied, dst[n]
 * until the first not in good
 */
int	strCpySpn(char* dst, const char* src, const char* good, int n);

/**
 * @brief Copy characters from the source string to the destination string
 *        until the first character of the source string which is in the 
 *        character set bad
 */
/* Copy characters from src to dst, return number copied, dst[n]
 * until the first in bad
 */
int	strCpyCSpn(char* dst, const char* src, const char* bad, int n);

/**
 * @brief Find a character in a string
 */
char *strChr(unsigned char*s,int c);

/**
 * @brief Allocate an array in memory with element initialized to 0
 */
void* Calloc(size_t num, size_t size);

#ifdef  __cplusplus
}
#endif

#endif /* CM_UTL_H */

