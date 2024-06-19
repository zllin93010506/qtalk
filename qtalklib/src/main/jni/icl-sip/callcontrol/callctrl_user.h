/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*****************************************************************************
	created:	2005/01/11
	created:	11:1:2005   20:23
	filename: 	callctrl_user
	file ext:	h
	author:		tyhuang
	
	purpose:	user setting of call object,like username,displayname
				,contact and authentication information    
	lastmodify: $Id: callctrl_user.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
*****************************************************************************/

/** \file callctrl_user.h
 *  \brief Declaration ccUser struct access functions
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */


#ifndef CALLCTRL_USER_H
#define CALLCTRL_USER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include "callctrl_cm.h"

/* create/deplicate/destory ccUser object */
/** \fn CCALPI RCODE ccUserNew(OUT pCCUser *pUser, IN const char *username)
 *  \brief Function to new a user object
 *  \param pUser - a ccUser object pointer
 *  \param username - user name for this ccUser
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserNew(OUT pCCUser *pUser, IN const char *username);

/** \fn CCALPI RCODE ccUserFree(IN OUT pCCUser *pUser)
 *  \brief Function to free a user object
 *  \param pUser - a ccUser object pointer
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserFree(IN OUT pCCUser *pUser);

/** \fn CCALPI RCODE ccUserDup(IN pCCUser *const pUser,OUT pCCUser* pNewUser)
 *  \brief Function to duplicate a new user object
 *  \param pUser - a ccUser object will be duplicated
 *  \param pNewUser - new ccUser object after successful duplication
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserDup(IN pCCUser *const pUser,OUT pCCUser* pNewUser);

/* set scheme to ccUser object */
/** \fn CCLAPI RCODE ccUserSetScheme(IN pCCUser *const pUser,IN const char* scheme)
 *  \brief Function to set scheme to a user object like sip or tel
 *  \param pUser - a ccUser object
 *  \param scheme - username will be set in user and used for from-url's scheme part
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccUserSetScheme(IN pCCUser *const pUser,IN const char* scheme);

/** \fn CCALPI RCODE ccUserGetScheme(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
 *  \brief Function to get scheme from a user object
 *  \param pUser - a ccUser object
 *  \param buffer - buffer for returned scheme
 *	\param length - size of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserGetScheme(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length);

/* get username from ccUser object */
/** \fn CCALPI RCODE ccUserGetUserName(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
 *  \brief Function to get username from a user object
 *  \param pUser - a ccUser object
 *  \param buffer - buffer for returned username
 *	\param length - size of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserGetUserName(IN pCCUser *const pUser,IN char* buffer,IN UINT32* length);


/* set username to ccUser object */
/** \fn CCLAPI RCODE ccUserSetUserName(IN pCCUser *const pUser,IN const char* username)
 *  \brief Function to set username to a user object
 *  \param pUser - a ccUser object
 *  \param username - username will be set in user and used for from-url's user part
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserSetUserName(IN pCCUser *const pUser,IN const char* username);


/* get displayname from ccUser object */
/** \fn CCALPI RCODE ccUserGetAddr(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
 *  \brief Function to get address from a user object
 *  \param pUser - a ccUser object
 *  \param buffer - buffer for returned address
 *	\param length - size of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccUserGetAddr(IN pCCUser *const pUser,IN char* buffer,IN UINT32* length);

/* set displayname from ccUser object */
/** \fn CCALPI RCODE ccUserSetAddr(IN pCCUser *const pUser,IN const char* addr)
 *  \brief Function to set address to a user object
 *  \param pUser - a ccUser object
 *  \param addr - addr will be set in user and used for from-url
 *  \return - RC_OK for success; RC_ERROR for failed
 */		
CCLAPI RCODE ccUserSetAddr(IN pCCUser *const pUser,IN const char* addr);

/* get displayname from ccUser object */
/** \fn CCALPI RCODE ccUserGetDisplayName(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
 *  \brief Function to get displayname from a user object
 *  \param pUser - a ccUser object
 *  \param buffer - buffer for returned displayname
 *	\param length - size of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */
CCLAPI RCODE ccUserGetDisplayName(IN pCCUser *const pUser,IN char* buffer,IN UINT32* length);

/* set displayname from ccUser object */
/** \fn CCALPI RCODE ccUserSetDisplayname(IN pCCUser *const pUser,IN const char* displayname)
 *  \brief Function to set display name to a user object
 *  \param pUser - a ccUser object
 *  \param displayname - display name will be set in user
 *  \return - RC_OK for success; RC_ERROR for failed
 */		
CCLAPI RCODE ccUserSetDisplayname(IN pCCUser *const pUser,IN const char* displayname);

/* get contact from ccUser object */
/** \fn CCALPI RCODE ccUserGetContact(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
 *  \brief Function to get contact from a user object
 *  \param pUser - a ccUser object
 *  \param buffer - buffer for returned contact address
 *	\param length - size of buffer
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserGetContact(IN pCCUser *const pUser,IN char* buffer,IN UINT32* length);

/* set contact of ccUser */
/** \fn CCALPI RCODE ccUserSetContact(IN pCCUser *const pUser,IN const char*contact)
 *  \brief Function to set contact to a user object
 *  \param pUser - a ccUser object
 *  \param contact - contact url will be set in user
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserSetContact(IN pCCUser *const pUser,IN const char*contact);

/* set cridential input(realm,username,password)*/
/** \fn CCALPI RCODE ccUserSetCredential(IN pCCUser *const pUser,IN const char*realm,IN const char*name,IN const char*passwd)
 *  \brief Function to set username/password for authentication to a user object
 *  \param pUser - a ccUser object
 *  \param realm - realm for the new credential
 *	\param name - username for the new credential
 *	\param passwd - password fot the new credential
 *  \return - RC_OK for success; RC_ERROR for failed
 */	
CCLAPI RCODE ccUserSetCredential(IN pCCUser *const pUser,IN const char*realm,IN const char*name,IN const char*passwd);

#ifdef  __cplusplus
}
#endif

#endif
