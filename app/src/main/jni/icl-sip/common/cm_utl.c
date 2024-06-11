/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_utl.c
 *
 * $Id: cm_utl.c,v 1.8 2009/02/11 06:28:11 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cm_utl.h"


/**
 * @brief Convert integer to string format
 * 
 * @param x The integer to be converted 
 * @param buf The string buffer to store the converted result
 * @param buflen Length of the string buffer
 * @return Return the buffer which contains the converted result
 * @retval NULL When error occur
 */
/* return string rep of integer i
 * string is in static memory, so it must be copied before another is
 * converted
 */
char* i2a(int x, char* buf, int buflen)
{
	char temp[12];

	if(!buf||buflen<0)
		return NULL;

	sprintf(temp,"%d",x);
	if( strlen(temp)<(unsigned int)buflen )
		strcpy(buf,temp);
	else
		return NULL;

	return buf;
}

/**
 * @brief Copy string and map it to uppercase
 *
 * @param src String to be copied
 * @param dst String to store the mapped results
 * @param dstlen The length of the string dst
 * @return Return the pointer pointing to the mapped string
 * @retval NULL When error occurs 
 */
/* copy string s, mapping to uppercase */
char* copyUp(const char* src, char* dst, int dstlen)
{
	/*char* copy;*/
	char* c;

	if( src==NULL || dst==NULL || dstlen<=(int)strlen(src) )
		return NULL;

	/*copy = malloc(strlen(s)+1);*/
	c = dst;

	while( *src != 0 ) {
		(*c)=toupper(*src);
		c++;
		src++;
	}
	(*c) = 0;

	return dst;
}

/**
 * @brief Check if one certain pattern appears in the string 
 * 
 * @param s The string in which we will check
 * @param pattern The string representing the pattern to be checked 
 * @retval true The pattern appears in the string s 
 * @retval false The string s is NULL or the pattern does not appear in the string
 */
/* return true if pattern appears in s */
/* ??? check and see if strstr is reasonably efficient
 * probably will be replaced by simple data structure
 */
int strCon(const char* s, const char* pattern)
{
	return s!=NULL && strstr(s,pattern) != NULL;
}

/**
 * @brief The case-insensitive version of strCon
 *
 * @param s The string in which we will check
 * @param pattern The string representing the pattern to be checked 
 * @retval true The pattern appears in the string s 
 * @retval false Error occurs when checking or the pattern does not appear in the string
 */
int strICon(const char* s, const char* pattern)
{
	char	*aa,*bb;
	int	ret;

	if( !s||!pattern ) return 0;

	aa = copyUp(s,strDup(s),strlen(s)+1);
	if ( !aa )
		return 0;

	bb = copyUp(pattern,strDup(pattern),strlen(pattern)+1);
	if ( !bb ) {
		free(aa);
		return 0;
	}

	ret = ( strstr(aa,bb)!=NULL );
	free(aa);	free(bb);

	return ret;
}

/** */
char *strncpyOverlap(char *dest, const char *src, size_t n)
{
    char* stringToCopy = (char*) malloc(n*sizeof(char)+1);
    strncpy(stringToCopy, src, n);
    char* returnPointer = strncpy(dest, stringToCopy, n);
    free(stringToCopy);
    
    return returnPointer;
}

char *strcpyOverlap(char *dest, const char *src)
{
    size_t len = strlen(src)+1;
    return strncpyOverlap(dest, src, len);
}


/**
 * @brief Remove whitespace from both beginning and end of a string
 *
 * @param s The string on which we will operate
 * @return Return the handled string
 */
/* remove whitespace from beginning and end of s, moving characters forward
 * returns s
 */
char* trimWS(char*s)
{
	char* pre;
	char* t;
	size_t len;

	if( s==NULL ) return s;

	pre = s;
	while( isspace((int)(*pre)) ) pre++;	/* NB !isspace('\0') */

	len=strlen(pre);
	if( pre>s )  /*strcpy(s,pre);*/
		strncpyOverlap(s,(const char*)pre,len+1);

	t = s+len;				/* start t at EOS */
	while( s<t && isspace((int)*(t-1)) )	/* if preceding is space, trim */
		t--;

	*t='\0';
	return s;
}

/**
 * @brief Add quote ('"') to both beginning and end of the string 
 *
 * @param s The string to be add quote to
 * @return Return the quote-added string 
 */
char* quote(char* s)
{
	char	*quoted;

	if( !s )		return NULL;

	quoted = (char*)malloc(strlen(s)+3);
	if( !quoted )
		return NULL;
	quoted[0] = 0;
	strcat(quoted,"\"");
	strcat(quoted,s);
	strcat(quoted,"\"");
	strcpy(s,quoted);
	free( quoted );

	return s;
}

/**
 * @brief Remove the quote ('"') from both beginning and end of the string if exists
 *
 * @param s The string from which we will remove quote
 * @return Return the quote-removed string 
 */
char* unquote(char* s)
{
	if( !s )		return NULL;

	s = trimWS(s);
	if ( *s=='\"' && s[strlen(s)-1]=='\"' ) {
		s[strlen(s)-1] = '\0';
        strcpyOverlap( s, s+1 );
//		strcpy( s, s+1 );
	}

	return s;
}

/**
 * @brief Remove the anglequote ('<','>') from both the beginning and end of the string if exist
 *
 * @param s The string from which we will remove anglequote
 * @return Return the anglequote-removed string
 */
char* unanglequote(char* s)
{
	if( !s )		return NULL;

	s = trimWS(s);
	if ( *s=='<' && s[strlen(s)-1]=='>' ) {
		s[strlen(s)-1] = '\0';
        strcpyOverlap( s, s+1 );
//		strcpy( s, s+1 );
	}

	return s;
}

/**
 * @brief Case-sensitive string comparison
 *
 * @param s1 The string to be compared 
 * @param s2 The other string to be compared 
 * @retval -1 Error occurs during the comparison
 * @retval !=0 s1 is different from s2
 * @retval =0 s1 is identical to s2
 */
int strCmp(const char* s1,const char* s2)
{

	if( !s1 || !s2 )
		return -1;
	else
		return strcmp(s1,s2);
}

/**
 * @brief Case-insensitive string comparison
 *
 * @param s1 The string to be compared 
 * @param s2 The other string to be compared 
 * @retval -1 Error occurs during the comparison
 * @retval <0 s1 is less than s2
 * @retval =0 s1 is identical to s2
 * @retval >0 s1 is greater than s2
 */
int strICmp(const char* s1, const char* s2)
{

#ifdef _WIN32
	if( !s1 || !s2 )
		return -1;
	else
		return _stricmp(s1,s2);
#elif defined _GNU_SOURCE
	if( !s1 || !s2 )
		return -1;
	else
		return strcasecmp(s1,s2);
#else
	char*	aa;
	char*	bb;
	int	result;
	
	if( !s1 || !s2 )
		return -1;

	aa = copyUp((char*)s1,strDup(s1),strlen(s1)+1);
	if ( !aa )
		return -1;

	bb = copyUp((char*)s2,strDup(s2),strlen(s2)+1);
	if ( !bb ) {
		free(aa);
		return -1;
	}
	result = strcmp(aa,bb);

	free(aa);
	free(bb);
	return result;
#endif
}

/**
 * @brief Compare numbers of characters of two strings case-insensitively
 *
 * @param s1 String to compare
 * @param s2 String to compare
 * @param n The number of characters to be compared
 * retval -1 Error occurs during the comparison
 * retval <0 s1 substring is less than s2 substring 
 * retval =0 s1 substring is identical to s2 substring 
 * retval >0 s1 substring is greater than s2 substring 
 */
int strICmpN(const char* s1, const char* s2, int n)
{
	char*	aa;
	char*	bb;
	int	result;

	if( !s1 || !s2 )
		return -1;

	aa = copyUp((char*)s1,strDup(s1),strlen(s1)+1);
	if ( !aa )
		return -1;

	bb = copyUp((char*)s2,strDup(s2),strlen(s2)+1);
	if ( !bb ) {
		free(aa);
		return -1;
	}
	result = strncmp(aa,bb,n);

	free(aa);
	free(bb);

	return result;
}

/**
 * @brief Duplicate string
 *
 * @param s The string to be duplicated
 * @return Return a pointer to the storage location of the copied string 
 * @retval NULL If s is NULL or the storage can not be allocated
 */
char* strDup(const char* s)
{

#ifdef _WIN32
	if( !s )
		return NULL;
	else
		return _strdup(s);
#elif defined _GNU_SOURCE
	if( !s )
		return NULL;
	else
		return strdup(s);
#else
	char* d=NULL;

	if( !s )
		return NULL;

	d = (char*)calloc(1,strlen(s)+1);
	if( !d )
		return NULL;

	return strcpy(d,s);
#endif

}

/**
 * @brief Attach a string to the end of the other string
 *
 * @param dst The string which will be put in head
 * @param src The string which will be put in tail
 * @return Return a pointer to the storage location for the string 
 *         containing dst followed by src
 * @retval NULL If dst and src are all NULL, or the storage cannot be allocated 
 */
char* strAtt(char*dst, const char*src)
{
	char* buff = NULL;

	if (!dst && !src)
		return NULL;
	else if (!dst)
		return (char*)src;
	else if (!src)
		return dst;

	buff = (char*)malloc((strlen(dst)+strlen(src)+1) * sizeof(char));
	if( !buff )
		return NULL;

	sprintf(buff, "%s%s", dst, src);
	buff[(strlen(dst)+strlen(src))] = '\0';

	return buff;
}

/**
 * @brief Copy characters from source string to the destination string
 *        until the first character of the source string which is not 
 *        in the character set good
 *
 * @param dst The destination string 
 * @param src The source string 
 * @param good The character set which will be copied to the destination string
 * @param n The maximum number of character to copy
 * @return Return the number of characters copied
 */
/* Copy characters from src to dst, return number copied, dst[n]
 * until the first not in good
 */
int strCpySpn(char* dst, const char* src, const char* good, int n)
{
	int len = 0;

	if (!dst || !src || !good)
		return -1;

	len = strspn(src, good);

	if (len >= n) len = n-1;
	strncpy(dst, src, len);
	dst[len] = '\0';

	return len;
}

/**
 * @brief Copy characters from the source string to the destination string
 *        until the first character of the source string which is in the 
 *        character set bad
 *
 * @param dst The destination string
 * @param src The source string
 * @param bad The character set which will not be copied to the destination string
 * @param n The maximum number of characters to copy
 * @return Return the number of characters copied
 */
/* Copy characters from src to dst, return number copied, dst[n]
 * until the first in bad
 */
int strCpyCSpn(char* dst, const char* src, const char* bad, int n)
{
	int len = 0;

	if (!dst || !src || !bad)
		return -1;

	len = strcspn(src, bad);

	if (len >= n) len = n-1;
	strncpy(dst, src, len);
	dst[len] = '\0';

	return len;
}

/**
 * @brief Find a character in a string 
 *
 * @param s The string in which we will find the character
 * @param c The character to be found
 * @return Return the pointer to the first occurrence of c in s 
 * @retval NULL If c is not found
 */
char *strChr(unsigned char*s,int c)
{
	if(s)
		return strchr((const char*)s,c);
	else
		return NULL;
}

/**
 * @brief Allocate an array in memory with element initialized to 0
 * 
 * @param num Number of elements in the array
 * @param size The size in bytes of each element 
 */
void* Calloc(size_t num, size_t size)
{
#ifndef _WIN32_WCE
	return calloc(num,size);
#else
	size_t totalsize=num*size;
	void *mem=malloc(totalsize);
	if(NULL==mem)
		return NULL;
	else{
		memset(mem,0,totalsize);
		return mem;
	}
#endif
}


