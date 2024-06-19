/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_mutex.c
 *
 * $Id: cx_mutex.c,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */

#define _POSIX_C_SOURCE 199309L /* struct timespec */

#include <stdio.h>

#ifdef UNIX
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#elif defined(_WIN32_WCE)
#define assert(cond)		ASSERT(cond)
#elif defined(_WIN32)
#define _WIN32_WINNT            0x0400   /* enable TryEnterCriticalSection */
#include <windows.h>
#include <process.h>
#include <assert.h>
#endif 

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#include <stdlib.h> 

#include <common/cm_def.h>
#include "cx_mutex.h"
#include "cx_misc.h"

struct cxMutexObj {
	CxMutexType		myType_;
	int			key_;
	int			refCont_;
	BOOL			lock_;

#ifdef UNIX
	pthread_mutex_t		*pCS_;
	pthread_mutexattr_t	CSAtt_;
#elif defined(_WIN32)
	LPCRITICAL_SECTION	pCS_;
	HANDLE			hmutex_;
#endif		
};


/** @brief  cxMutexNew - create a mutex object and initializes it 
 *
 *  The cxMutexNew function creates a mutex object and initializes it to recursive mode when input parameter mtype is MUTEX_INTERTHREAD. 
 *
 *  [UNIX] 
 *	For UNIX systems, only MUTEX_INTERTHREAD mutex-type is useful.
 *
 *  [WINDOWS]
 *	For WINDOWS systems, the key parameter is ignored when the parameter mtype is MUTEX_INTERTHREAD. When mtype parameter is MUTEX_INTERPROCESS, the key value is used for other process to obtain this mutex.
 *
 *  @param mtype mutex type, define in the cx_mutex.h file.
 *  @param key the key value is used for other process to obtain this mutex.
 *  @return On success, cxMutexNew function returns a CxMutex pointer. 
 *          Otherwise, NULL is returned.
 */
CxMutex	cxMutexNew(CxMutexType mtype, int key)
{
	CxMutex	this_ = NULL;

	if( mtype!=CX_MUTEX_INTERPROCESS && mtype!=CX_MUTEX_INTERTHREAD )
		return NULL;

	this_ = (CxMutex)malloc(sizeof(struct cxMutexObj));
	if(!this_)
		return NULL;

	this_->myType_ = mtype;
	this_->key_ = key;
	this_->refCont_ = 0;
	this_->lock_ = FALSE;

	if( mtype==CX_MUTEX_INTERTHREAD ) {
#if defined(UNIX)
		this_->pCS_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
		if( !this_->pCS_ ) {
			free(this_);
			return NULL;
		}
		pthread_mutexattr_init(&this_->CSAtt_);
	#if defined(LINUX)
		pthread_mutexattr_setkind_np(&this_->CSAtt_,PTHREAD_MUTEX_RECURSIVE_NP);	
	#elif defined(SOLARIS)		
		pthread_mutexattr_settype(&this_->CSAtt_,PTHREAD_MUTEX_RECURSIVE);
	#endif
		if( pthread_mutex_init(this_->pCS_,&this_->CSAtt_)!=0 ) {
			free(this_->pCS_);
			free(this_);
			return NULL;
		}
#elif defined(_WIN32)
		this_->hmutex_ = NULL; /* unused, set to NULL */
		this_->pCS_ = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
		if( !this_->pCS_ ) {
			free(this_);
			return NULL;
		}
		InitializeCriticalSection(this_->pCS_);
#endif
	}
	else if ( mtype==CX_MUTEX_INTERPROCESS ) {
#if defined(UNIX)
		/* to be implemented */
#elif defined(_WIN32)
		char	key_temp[5] = {0,0,0,0,0};
		
		this_->pCS_ = NULL;
		*((int*)key_temp) = this_->key_;
   #ifdef _WIN32_WCE
		this_->hmutex_ = CreateMutex(NULL,FALSE, (unsigned short*)key_temp);
   #else
		this_->hmutex_ = CreateMutex(NULL,FALSE,key_temp);
   #endif
		if( this_->hmutex_==NULL ) {
			free(this_);
			return NULL;
		}
#endif
	}

	return this_;
}


/** @brief  cxMutexTryLock - Attempts to lock the specified mutex, without waiting if already locked.
 *
 *  The cxMutexTryLock function attempts to lock the mutex specified in the mutex argument.  If the mutex is currently unlock (or is not lock by another thread), it becomes locked (or increasing the lock count) and owned by the the  calling  thread, and cxMutexTryLock returns success value immediately. Otherwise, cxMutexTryLock returns immediately with a negative value.
 *
 *  @param mutex points to a CxMutex structure that created by cxMutexNew function to lock.
 *  @return On success, 0 is returned.
 *          Otherwise, cxMutexTryLock function returns a negative value.
 */
int cxMutexTryLock(CxMutex mutex)
{
	if(!mutex||mutex->lock_)
		return -1;

	if( mutex->myType_==CX_MUTEX_INTERTHREAD ) {
		if(!mutex->pCS_)
			return -1;
#if defined(UNIX)
		if( pthread_mutex_trylock(mutex->pCS_)!=0 ) {
			if( errno==EBUSY )
				return -2;
			printf( "cxMutexLock(): pthread_mutex_lock() error!");
			return -1;
		}
#elif defined(_WIN32)
		if( TryEnterCriticalSection(mutex->pCS_)==0 )
			return -2;
#endif
	}
	else if ( mutex->myType_==CX_MUTEX_INTERPROCESS ) {
#if defined(UNIX)
		/* to be implemented */
#elif defined(_WIN32)
		DWORD ret;
		if(!mutex->hmutex_)
			return -1;
		ret = WaitForSingleObject(mutex->hmutex_,0);
		if( ret==WAIT_FAILED ) {
			printf( "cxMutexLock(): WaitForSingleObject() error!");		
			return -1;	
		}
		else if( ret==WAIT_TIMEOUT ) {
			return -2;	
		}
#endif
	}
	mutex->refCont_++;

	return 0;
}

/** @brief  cxMutexLock - lock an unlocked mutex
 *
 *  The cxMutexLock function attempts to lock the mutex specified in the mutex argument.  If the mutex is currently unlock (or is not lock by another thread), it becomes locked (or increasing the lock count) and owned by the the  calling  thread, and cxMutexLock returns success value immediately. Otherwise, cxMutexLock suspends the calling thread util the mutex is unlocked.
 *
 *  @param mutex points to a CxMutex structure that created by cxMutexNew function to lock.
 *  @return On success, 0 is returned.
 *          Otherwise, -1 is returned.
 */
int cxMutexLock(CxMutex mutex)
{
	if(!mutex||mutex->lock_)
		return -1;

	if( mutex->myType_==CX_MUTEX_INTERTHREAD ) {
		if(!mutex->pCS_)
			return -1;
#if defined(UNIX)
		if( pthread_mutex_lock(mutex->pCS_)!=0 ) {
			printf("cxMutexLock(): pthread_mutex_lock() error!");
			return -1;
		}
#elif defined(_WIN32)
		EnterCriticalSection(mutex->pCS_);
#endif
	}
	else if ( mutex->myType_==CX_MUTEX_INTERPROCESS ) {
#if defined(UNIX)
		/* to be implemented */
#elif defined(_WIN32)
		if(!mutex->hmutex_)
			return -1;
		if( WaitForSingleObject(mutex->hmutex_,INFINITE)==WAIT_FAILED ) {
			printf("cxMutexLock(): WaitForSingleObject() error!");		
			return -1;	
		}
#endif
	}
	mutex->refCont_++;

	return 0;
}


/** @brief  cxMutexUnlock - unlock the specified mutex
 *
 *  The cxMutexUnlock function unlocks the mutex specified in the mutex argument.  If the mutex is owned by the calling thread, the lock count is decremented. The mutex becomes unlocked when the lock count reaches 0.
 *
 *  @param mutex points to a CxMutex structure that created by cxMutexNew function to unlock.
 *  @return On success, 0 is returned.
 *          Otherwise, -1 is returned.
 */
int cxMutexUnlock(CxMutex mutex)
{
	if(!mutex)
		return -1;

	if( mutex->myType_==CX_MUTEX_INTERTHREAD ) {
		if(!mutex->pCS_)
			return -1;
		mutex->refCont_--;

		/* assert( mutex->refCont_>=0 ); mark by tyhuang 2004/8/5	*/
		if(mutex->refCont_< 0) {
			printf("cxMutexUnlock(): warning! refCont < 0!\n");
			mutex->refCont_=0;
			return -1;
		}
#if defined(UNIX)
		if( pthread_mutex_unlock(mutex->pCS_)!=0 ) {
			printf("cxMutexUnlock(): pthread_mutex_unlock() error!");		
			return -1;	
		}		
#elif defined(_WIN32)
		LeaveCriticalSection(mutex->pCS_);
#endif
	}
	else if ( mutex->myType_==CX_MUTEX_INTERPROCESS ) {
		mutex->refCont_--;
		assert( mutex->refCont_>=0 );
		/*
		if(mutex->refCont_< 0) {
			printf("cxMutexUnlock(): warning! refCont < 0!\n");
			mutex->refCont_=0;
			return -1;
		}mark by tyhuang 2005/1/7	*/
#if defined(UNIX)
		/* to be implemented */
#elif defined(_WIN32)
		if(!mutex->hmutex_)
			return -1;
		if( !ReleaseMutex(mutex->hmutex_) )
			return -1;
#endif
	}
	
	return 0;
}


/** @brief  cxMutexFree - destroy a mutex and release the resources it uesd
 *
 *  The cxMutexFree function destroys the specified mutex and release the resources it uesd. The function should be called when the mutex is no longer referenced. If the mutex is not unlock, the cxMutexFree function will do nothing.
 *
 *  @param mutex points to a CxMutex structure that created by cxMutexNew function to lock.
 *  @return On success, 0 is returned.
 *          Otherwise, -1 is returned.
 */
void cxMutexFree(CxMutex mutex)
{
	int i = 0;
	if(!mutex||mutex->lock_)
		return;
	mutex->lock_ = TRUE;

	while( mutex->refCont_!=0 ) {
		i++;
		sleeping(10);
		if( i>200 ) {
			printf( "cxMutexFree(): "
			        "Wait for other threads to release mutex!\n"
			        "Maybe deadlocked!!");
			return;
		}
	}

	if( mutex->myType_==CX_MUTEX_INTERTHREAD ) {
		if(!mutex->pCS_) 
			return;
#if defined(UNIX)
		if( pthread_mutex_lock(mutex->pCS_)!=0 ) {
			printf( "cxMutexFree(): pthread_mutex_lock() error!");		
			return;	
		}				
		if( pthread_mutex_unlock(mutex->pCS_)!=0 ) {
			printf( "cxMutexFree(): pthread_mutex_unlock() error!");		
			return;	
		}				
		if( pthread_mutex_destroy(mutex->pCS_)!=0 ) {
			printf( "cxMutexFree(): WaitForSingleObject() error!");		
			return;					
		}
                if( pthread_mutexattr_destroy(&mutex->CSAtt_)!=0 ) {
                        printf( "cxMutexFree(): WaitForSingleObject() error!");
                        return;
                }
#elif defined(_WIN32)
		EnterCriticalSection(mutex->pCS_);
		LeaveCriticalSection(mutex->pCS_);			
		DeleteCriticalSection(mutex->pCS_);
#endif
		free(mutex->pCS_);
		mutex->pCS_ = NULL;
	}
	else if ( mutex->myType_==CX_MUTEX_INTERPROCESS ) {
#if defined(UNIX)
		/* to be implemented */
#elif defined(_WIN32)
		if( WaitForSingleObject(mutex->hmutex_,INFINITE)==WAIT_FAILED )
			return;	/* memory leak */
		if( !ReleaseMutex(mutex->hmutex_) )
			return; /* memory leak */
		if(mutex->hmutex_) {
			CloseHandle(mutex->hmutex_);
			mutex->hmutex_ = NULL;
		}
#endif
	}

	free(mutex);
}

