/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*****************************************************************************
	created:	2005/01/11
	created:	11:1:2005   20:19
	filename: 	callctrl_user
	file ext:	c
	author:		tyhaung
	
	purpose:	user setting of call object,like username,displayname
				,contact and authentication information
	lastmodify: $Id: callctrl_user.c,v 1.2 2008/09/23 09:07:21 tyhuang Exp $
*****************************************************************************/

/** \file callctrl_user.c
 *  \brief Define ccUser struct access functions
 *  \author Huang, Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#include <adt/dx_lst.h>
#include <common/cm_utl.h>
#include "callctrl_user.h"

struct ccUserObj{
	char	*scheme;
	char	*username;
	char	*address;
	char	*displayname;
	char	*contact;
	
	DxLst	authdata;
};

typedef struct ccAuthzObj ccAuthz;
typedef struct ccAuthzObj* pccAuthz;

struct ccAuthzObj{
	char*	realm;		/*specify realm name*/
	char*	username;	/*specify user name for above realm*/
	char*	passwd;		/*specify user password for above realm*/
};

void ccAuthDataFree(pccAuthz);
pccAuthz ccAuthDataDup(pccAuthz);

CCLAPI 
RCODE 
ccUserNew(OUT pCCUser *pUser, IN const char *username)
{
	pCCUser user;

	/* check parameter */
	if( !pUser ) {
		CCERR ("[ccUserNew] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	if( !username ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserNew] Invalid argument : Null pointer to username\n" );
		return RC_ERROR;
	}

	user=calloc(1,sizeof(CCUser));
	if( !user ) {
		CCERR("[ccUserNew] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	user->username=strDup(username);

	if (!user->username) {
		ccUserFree(&user);
		CCERR("[ccUserNew] memory allocation of username failed!\n");
		return RC_ERROR;
	}

	user->scheme=strDup("sip");
	if (!user->scheme) {
		ccUserFree(&user);
		CCERR("[ccUserNew] memory allocation of url scheme failed!\n");
		return RC_ERROR;
	}
	/*
	user->address=NULL;
	user->displayname=NULL;
	user->contact=NULL;
	user->authdata=NULL;
	*/
	
	*pUser=user;
	return RC_OK;
}

CCLAPI 
RCODE 
ccUserFree(IN OUT pCCUser *pUser)
{
	pCCUser user;

	/* check parameter */
	if( !pUser ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserFree] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;

	if( !user ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserFree] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if(user->scheme)
		free(user->scheme);

	if( user->username )
		free(user->username);

	if( user->address )
		free(user->address);

	if( user->displayname )
		free(user->displayname);

	if( user->contact)
		free( user->contact);

	if( user->authdata )
		dxLstFree( user->authdata,(void (*) (void*))ccAuthDataFree);

	memset(user,0,sizeof(CCUser));

	free(user);
	
	*pUser=NULL;

	return RC_OK;
}

CCLAPI 
RCODE 
ccUserDup(IN pCCUser *const pUser,OUT pCCUser *pNewUser)
{
	pCCUser newuser,user;

	/* check parameter */
	if( !pUser ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserDup] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserDup] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	if( !pNewUser ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserDup] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	newuser=calloc(1,sizeof(CCUser));
	if( !newuser ) {
		CCERR("[ccUserDup] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	if (user->scheme) {
		newuser->scheme=strDup(user->scheme);
		if (!newuser->scheme) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}

	if (user->username) {
		newuser->username=strDup(user->username);
		if (!newuser->username) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}

	if (user->address) {
		newuser->address=strDup(user->address);
		if (!newuser->address) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}

	if(user->displayname){
		newuser->displayname=strDup(user->displayname);
		if (!newuser->displayname) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}

	if(user->contact){
		newuser->contact=strDup(user->contact);
		if (!newuser->contact) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}

	if(user->authdata){
		newuser->authdata=dxLstDup(user->authdata,(void *(*)(void *))ccAuthDataDup);
		if (!newuser->authdata) {
			ccUserFree(&newuser);
			CCERR("[ccUserDup] memory allocation failed!\n");
			return RC_ERROR;
		}
	}
	*pNewUser=newuser;
	return RC_OK;
}

	  
CCLAPI 
RCODE 
ccUserSetScheme(IN pCCUser *const pUser,IN const char* scheme)
{
	pCCUser user;
	
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetScheme] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetScheme] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	if ( !scheme ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetScheme] Invalid argument : Null pointer to scheme\n" );
		return RC_ERROR;
	}
	
	/* free old value */
	if(user->scheme)
		free(user->scheme);
	
	/* set new value */
	user->scheme=strDup(scheme);
	if (!user->scheme) {
		CCERR("[ccUserSetScheme] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	return RC_OK;	
}

CCLAPI 
RCODE 
ccUserGetScheme(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
{
	pCCUser user;
	UINT32 len;

	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserGetScheme] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserGetScheme] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	if (!user->scheme) {
		CCWARN ("[ccUserGetScheme] Invalid argument : Null pointer to username\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if(!length){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetScheme] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}

	len=strlen(user->scheme);
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < (unsigned int)*length ){
		/* clean buffer */
		memset(buffer,0,(unsigned int)*length);
		/* copy data */
		strncpy(buffer,user->scheme, len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetUserName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccUserGetUserName(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
{
	pCCUser user;
	UINT32 len;
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserGetUserName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserGetUserName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	if (!user->username) {
		CCWARN ("[ccUserGetUserName] Invalid argument : Null pointer to username\n" );
		return RC_ERROR;
	}

	len=strlen(user->username);
	/* check input buffer size */
	if(!length){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetUserName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	if(*length==0){
		*length=len+1;
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetUserName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	if( !buffer){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetUserName] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < (unsigned int)*length ){
		/* clean buffer */
		memset(buffer,0,(unsigned int)*length);
		/* copy data */
		strncpy(buffer,user->username, len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetUserName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccUserSetUserName(IN pCCUser *const pUser,IN const char* username)
{
	pCCUser user;
	
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetUserName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetUserName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	if ( !username ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetUserName] Invalid argument : Null pointer to username\n" );
		return RC_ERROR;
	}
	
	/* free old value */
	if(user->username)
		free(user->username);
	
	/* set new value */
	user->username=strDup(username);
	if (!user->username) {
		CCERR("[ccUserSetUserName] memory allocation failed!\n");
		return RC_ERROR;
	}
	
	return RC_OK;	
}

/* set displayname from ccUser object */
CCLAPI 
RCODE 
ccUserSetAddr(IN pCCUser *const pUser,IN const char* addr)
{
	pCCUser user;

	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetAddr] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetAddr] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if ( !addr ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetAddr] Invalid argument : Null pointer to addr\n" );
		return RC_ERROR;
	}

	/* free old value */
	if(user->address)
		free(user->address);

	/* set new value */
	user->address=strDup(addr);
	if (!user->address) {
		CCERR("[ccUserSetAddr] memory allocation failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

/* get displayname from ccUser object */
CCLAPI 
RCODE 
ccUserGetAddr(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
{
	pCCUser user;
	UINT32 len;
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserGetAddr] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserGetAddr] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if (!user->address) {
		return RC_ERROR;
	}

	/* check input buffer size */
	if(!length){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetAddr] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	len=strlen(user->address);
	if(*length==0){
		*length=len+1;
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetAddr] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	if( !buffer){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetAddr] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < (unsigned int)*length ){
		/* clean buffer */
		memset(buffer,0,(unsigned int)*length);
		/* copy data */
		strncpy(buffer,user->address, len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetAddr] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}	
}

CCLAPI 
RCODE 
ccUserSetDisplayname(IN pCCUser *const pUser,IN const char*displayname)
{
	pCCUser user;

	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetDisplayname] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetDisplayname] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if ( !displayname ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetDisplayname] Invalid argument : Null pointer to displayname\n" );
		return RC_ERROR;
	}

	/* free old value */
	if(user->displayname)
		free(user->displayname);

	/* set new value */
	user->displayname=strDup(displayname);
	if (!user->displayname) {
		CCERR("[ccUserSetDisplayname] memory allocation failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

CCLAPI 
RCODE 
ccUserGetDisplayName(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
{
	pCCUser user;
	UINT32 len;
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserGetDisplayName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserGetDisplayName] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if (!user->displayname) {
		return RC_ERROR;
	}

	/* check input buffer size */
	if(!length ){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetDisplayName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	len=strlen(user->displayname);
	if(*length==0){
		*length=len+1;
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetDisplayName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	if( !buffer){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetDisplayName] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < (unsigned int)*length ){
		/* clean buffer */
		memset(buffer,0,(unsigned int)*length);
		/* copy data */
		strncpy(buffer,user->displayname,len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetDisplayName] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccUserSetContact(IN pCCUser *const pUser,IN const char*contact)
{
	pCCUser user;

	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetContact] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetContact] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if ( !contact ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetContact] Invalid argument : Null pointer to contact\n" );
		return RC_ERROR;
	}

	/* free old value */
	if(user->contact)
		free(user->contact);

	/* set new value */
	user->contact=strDup(contact);

	if (!user->contact) {
		CCERR("[ccUserSetContact] memory allocation failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

CCLAPI 
RCODE 
ccUserGetContact(IN pCCUser *const pUser,IN OUT char* buffer,IN OUT UINT32* length)
{
	pCCUser user;
	UINT32 len;
	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserGetContact] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserGetContact] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	if (!user->contact) {
		return RC_ERROR;
	}

	/* check input buffer size */
	if(!length){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetContact] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	len=strlen(user->contact);
	if(*length==0){
		*length=len+1;
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetContact] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	if( !buffer){
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetContact] Invalid argument : buffer point is NULL\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < (unsigned int)*length ){
		/* clean buffer */
		memset(buffer,0,(unsigned int)*length);
		/* copy data */
		strncpy(buffer,user->contact,len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserGetContact] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

/* set cridential input(realm,username,password)*/
CCLAPI 
RCODE 
ccUserSetCredential(IN pCCUser *const pUser,IN const char*realm,IN const char*name,IN const char*passwd)
{
	pCCUser user;
	pccAuthz authx=NULL;

	/* check parameter */
	if ( ! pUser ) {
		CCERR ("[ccUserSetCredential] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	user=*pUser;
	if( !user ) {
		CCERR ("[ccUserSetCredential] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	if( !realm ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetCredential] Invalid argument : Null pointer to realm\n" );
		return RC_ERROR;
	}
	
	if( !name ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetCredential] Invalid argument : Null pointer to username\n" );
		return RC_ERROR;
	}

	if( !passwd ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUserSetCredential] Invalid argument : Null pointer to password\n" );
		return RC_ERROR;
	}

	/* check realm */
	if(user->authdata)
		;
	else{
		user->authdata=dxLstNew(DX_LST_POINTER);
		if (!user->authdata) {
			CCERR ("[ccUserSetCredential] Memory allocation error\n" );
			return RC_ERROR;
		}
	}

	authx=(pccAuthz)calloc(1,sizeof(ccAuthz));
	if (!authx) {
		CCERR ("[ccUserSetCredential] Memory allocation error\n" );
		return RC_ERROR;
	}
	authx->realm=strDup(realm);
	authx->username=strDup(name);
	authx->passwd=strDup(passwd);

	if (!authx->realm || !authx->username||!authx->passwd) {
		CCERR ("[ccUserSetCredential] Memory allocation error\n" );
		return RC_ERROR;
	}

	return dxLstPutTail(user->authdata,(void*)authx);
}

void ccAuthDataFree(pccAuthz authdata)
{

	if (!authdata) {
		return;
	}
	if(authdata->realm)
		free(authdata->realm);
	if(authdata->username)
		free(authdata->username);
	if(authdata->passwd);
		free(authdata->passwd);

	memset(authdata,0,sizeof(ccAuthz));
	free(authdata);
}

pccAuthz ccAuthDataDup(pccAuthz authdata)
{
	pccAuthz newauthdata;

	if (!authdata || !authdata->realm || !authdata->username || !authdata->passwd) {
		CCWARN ("[ccAuthDataDup] Invalid argument : Null pointer to authdata\n" );
		return NULL;
	}

	newauthdata=(pccAuthz)calloc(1,sizeof(ccAuthz));

	if (!newauthdata) {
		CCERR ("[ccAuthDataDup] Memory allocation error\n" );
		return NULL;
	}

	if(authdata->realm)
		newauthdata->realm=strDup(authdata->realm);
	if(authdata->username)
		newauthdata->username=strDup(authdata->username);
	if(authdata->passwd);
		newauthdata->passwd=strDup(authdata->passwd);

	if (!newauthdata->realm||!newauthdata->username||!authdata->passwd) {
		ccAuthDataFree(newauthdata);
		CCERR ("[ccAuthDataDup] Memory allocation error\n" );
		return NULL;
	}

	return newauthdata;
}


