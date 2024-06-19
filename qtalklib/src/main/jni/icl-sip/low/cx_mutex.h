/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_mutex.h
 *
 * $Id: cx_mutex.h,v 1.2 2008/09/24 03:21:29 tyhuang Exp $
 */

#ifndef CX_MUTEX_H
#define CX_MUTEX_H

#ifdef UNIX
#include <pthread.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef UNIX
/* Necessary to avoid "implicit declaration" warning add by tyhuang */
int pthread_mutexattr_setkind_np(pthread_mutexattr_t *, int );
#endif

typedef enum {
	CX_MUTEX_INTERTHREAD,
	CX_MUTEX_INTERPROCESS
} CxMutexType;

typedef struct cxMutexObj* CxMutex;

/*
 * cxMutexNew(CxMutexType, int key)
 *	when type==MUTEX_INTERTHREAD, the key value is ignored.
 *	when type==MUTEX_INTERPROCESS, the key value is used for other process
 *	to obtain this mutex.
 */
CxMutex	cxMutexNew(CxMutexType, int key);

/* This will block current thread, if the mutex is owned by other threads now. */
int	cxMutexLock(CxMutex);
int     cxMutexTryLock(CxMutex);
int	cxMutexUnlock(CxMutex);
void	cxMutexFree(CxMutex);

#ifdef __cplusplus
}
#endif

#endif /* CX_MUTEX_H */

