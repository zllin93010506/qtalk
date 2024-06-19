/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * dx_str.h
 *
 * $Id: dx_str.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_str.h
* @brief Declare dx string related functions
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/
#ifndef DX_STR_H
#define DX_STR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>

typedef struct dxStrObj		*DxStr;

/**
* @fn CCLAPI DxStr	dxStrNew(void)
* @brief allocate a new String
* @retval DxHash - a pointer to dx string which have 256 bytes of char
*/
CCLAPI DxStr	dxStrNew(void);

/**
* @fn CCLAPI int	dxStrLen(IN DxStr _this)
* @brief length of dx string
* @param _this - a pointer to dx string
* @retval int - the length of this dx string, -1 for NULL
*/
CCLAPI int	dxStrLen(IN DxStr _this);

/**
* @fn CCLAPI RCODE	dxStrClr(IN OUT DxStr _this)
* @brief set dx string to '\0'
* @param _this - a pointer to dx string
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
CCLAPI RCODE	dxStrClr(IN OUT DxStr _this);

/**
* @fn CCLAPI RCODE	dxStrFree(IN OUT DxStr _this)
* @brief De-allocate dx string
* @param _this - a pointer to dx string
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
CCLAPI RCODE	dxStrFree(IN OUT DxStr _this);

/**
* @fn CCLAPI RCODE	dxStrCat(IN OUT DxStr _this, IN const char* s)
* @brief Append s to dx string
* @param _this - a pointer to dx string
* @param s - a pointer to data which will appended after dx string
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
CCLAPI RCODE	dxStrCat(IN OUT DxStr _this, IN const char* s);

CCLAPI RCODE dxStrCatB(DxStr _this, const char* s,unsigned int len);

/**
* @fn CCLAPI RCODE	dxStrCatN(IN OUT DxStr _this, IN const char* s, IN  int n)
* @brief Append n characters from s to dx string
* @param _this - a pointer to dx string
* @param s - a pointer to data which will appended after dx string
* @param n - a indicated length to be appended, if n > strlen(s), then strlen(s) will be appended
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
CCLAPI RCODE	dxStrCatN(IN OUT DxStr _this, IN const char* s, IN  int n);

/**
* @fn CCLAPI char*	dxStrAsCStr(IN DxStr _this)
* @brief return char* version of dx string; WARNING: dxStrAsCStr does NOT return a copy.  That is the callers responsibility
* @param _this - a pointer to dx string
* @retval char* - a pointer to data of dx string
*/
CCLAPI char*	dxStrAsCStr(IN DxStr _this);

#ifdef  __cplusplus
}
#endif

#define MINALLOC    256

#endif /* DX_STR_H */
