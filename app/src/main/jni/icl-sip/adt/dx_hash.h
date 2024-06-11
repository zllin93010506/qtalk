/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * dx_hash.h
 *
 * $Id: dx_hash.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_hash.h
* @brief Declare dx hash related functions
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/
#ifndef DX_HASH_H
#define DX_HASH_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>

typedef struct dxHashObj	*DxHash;
typedef struct entryObj		*Entry;

typedef enum {
	DX_HASH_POINTER, 
	DX_HASH_CSTRING, 
	DX_HASH_INT
} DxHashType;

/**
* @fn DxHash	dxHashNew(IN int initSize, IN int keysUpper, IN DxHashType strings)
* @brief create new hashtable
* @param initSize - number of entries in table (may grow)
* @param keysUpper - bool should keys be mapped to upper case
* @param strings - dx HashType   should val be copied (cstrings and ints) by dxHashAdd, keys are always strings and always copied
* @retval DxHash - a pointer to dx hash
*/
DxHash	dxHashNew(IN int initSize, IN int keysUpper, IN DxHashType strings);

/**
* @fn void	dxHashFree(IN OUT DxHash, IN void (*freeVal)(void*))
* @brief destroy hashtable, call freeVal on each val
* @param _this - a pointer to dx hash
* @param freeVal - function to free val in Entry
*/
void	dxHashFree(IN OUT DxHash _this, IN void (*freeVal)(void*));

/**
* @fn DxHash  dxHashDup(IN DxHash _this, IN void*(*elementDupCB)(void*))
* @brief Duplicate dx hash
* @param _this - a pointer to dx hash which will be duplicated
* @param elementDupCB - function to duplicate val in Entry
* @retval DxHash - a pointer to the duplicate
*/
DxHash  dxHashDup(IN DxHash _this, IN void*(*elementDupCB)(void*));

/**
* @fn void*	dxHashAdd(IN OUT DxHash _this, IN const char* key, IN void* val)
* @brief add pair key/val to hash, return old value
* @param _this - a pointer to dx hash
* @param key - a pointer to key
* @param val - a pointer to val
* @retval void* - a pointer to old val
*/
void*	dxHashAdd(IN OUT DxHash _this, IN const char* key, IN void* val);

/**
* @fn void*	dxHashDel(IN OUT DxHash _this, IN const char* key)
* @brief del key from hash, return old value
* @param _this - a pointer to dx hash
* @param key - a pointer to key which is used for searching
* @retval void* - a pointer to old val
*/
void*	dxHashDel(IN OUT DxHash _this, IN const char* key);

/**
* @fn void*	dxHashItem(IN DxHash _this, IN const char* key)
* @brief return val for key in hash.  NULL if not found
* @param _this - a pointer to dx hash
* @param key - a pointer to key which is used for searching
* @retval void* - a pointer to the val of the matched key
*/
void*	dxHashItem(IN DxHash _this, IN const char* key);

/**
* @fn void	StartKeys(IN OUT DxHash _this)
* @brief iterator over keys in hash.
* @param _this - a pointer to dx hash
*/
void	StartKeys(IN OUT DxHash _this);

/**
* @fn char*	NextKeys(IN DxHash _this)
* @brief return the next key in dx hash
* @param _this - a pointer to dx hash
* @retval char* - a pointer to next key, returns NULL when no keys left
*/
char*	NextKeys(IN DxHash _this);

void    dxHashLock(DxHash); /*NO implementation*/
void    dxHashUnlock(DxHash);/*NO implementation*/

/**
* @fn int     dxHashSize(IN DxHash _this)
* @brief return the amount of elements in dx hash
* @param _this - a pointer to dx hash
* @retval int - the amount of elements in dx hash
*/
int     dxHashSize(IN DxHash _this);

/**
* @fn void	dxHashSetTraceFlag(IN OUT DxHash _this, IN OUT TraceFlag traceflag)
* @brief set this dx hash's trace flag ON or OFF
* @param _this - a pointer to dx hash
* @param traceflag - a int to indicate trace flag of dx hash is ON or OFF
*/
void	dxHashSetTraceFlag(IN OUT DxHash _this, IN OUT TraceFlag traceflag);

/**
* @fn Entry	dxHashItemLst(IN DxHash _this, IN const char* key)
* @brief get a list from table after hashing
* @param _this - a pointer to dx hash
* @param key - a pointer to key which is used for searching
* @retval Entry - a pointer to head of the list which key equals to key
*/
Entry	dxHashItemLst(IN DxHash _this, IN const char* key);

/**
* @fn RCODE	dxHashAddEx(IN OUT DxHash _this, IN const char* key, IN void* val)
* @brief add item ,allow use the same key
* @param _this - a pointer to dx hash
* @param key - a pointer to key
* @param val - a pointer to val
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE	dxHashAddEx(IN OUT DxHash _this, IN const char* key, IN void* val);

/**
* @fn RCODE dxHashDelEx(IN OUT DxHash _this, IN const char* key, IN void* val)
* @brief del item by key and value
* @param _this - a pointer to dx hash
* @param key - a pointer to key which is used for searching
* @param val - a pointer to val which is used for searching
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE dxHashDelEx(IN OUT DxHash _this, IN const char* key, IN void* val);

/**
* @fn char*	dxEntryGetKey(IN Entry _this)
* @brief get the key in this dx hash element
* @param _this - a pointer to dx hash element
* @retval char* - a pointer to the key
*/
char*	dxEntryGetKey(IN Entry _this);

/**
* @fn void*	dxEntryGetValue(IN Entry _this)
* @brief get the val in this dx hash element
* @param _this - a pointer to dx hash element
* @retval void* - a pointer to the val
*/
void*	dxEntryGetValue(IN Entry _this);

/**
* @fn Entry	dxEntryGetNext(IN Entry _this)
* @brief get a pointer to the next element of this one
* @param _this - a pointer to dx hash element
* @retval Entry - a pointer to the next element
*/
Entry	dxEntryGetNext(IN Entry _this);

#ifdef  __cplusplus
}
#endif

#endif /* DX_HASH_H */
