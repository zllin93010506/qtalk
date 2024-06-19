/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * msgcount.c
 *
 * $Id: msgcount.c,v 1.2 2009/05/25 02:48:17 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <common/cm_utl.h>
#include "msgcount.h"

struct MsgCountObj{
	/* all values must be 0~2^32-1 */
	int msgsize[4];
};


CCLAPI 
pMsgCount MsgCountNew(int newmsg,int oldmsg,int newumsg,int oldumsg)
{
	pMsgCount ret=NULL;

	ret=(pMsgCount)calloc(1,sizeof(MsgCount));

	if(NULL==ret){
		printf("[MsgCountNew] alloc memory for MsgCount is failed!\n" );
		return NULL;
	}

	ret->msgsize[MCT_NEW]=newmsg;
	ret->msgsize[MCT_OLD]=oldmsg;
	ret->msgsize[MCT_UNEW]=newumsg;
	ret->msgsize[MCT_UOLD]=oldumsg;

	return ret;
}

CCLAPI 
pMsgCount MsgCountDup(pMsgCount oldmc)
{
	pMsgCount ret=NULL;

	if(NULL==oldmc){
		return NULL;
	}

	ret=(pMsgCount)calloc(1,sizeof(MsgCount));

	if(NULL==ret){
		printf("[MsgCountNew] alloc memory for MsgCount is failed!\n" );
		return NULL;
	}

	ret->msgsize[MCT_NEW]=oldmc->msgsize[MCT_NEW];
	ret->msgsize[MCT_OLD]=oldmc->msgsize[MCT_OLD];
	ret->msgsize[MCT_UNEW]=oldmc->msgsize[MCT_UNEW];
	ret->msgsize[MCT_UOLD]=oldmc->msgsize[MCT_UOLD];

	return ret;
}

CCLAPI 
void MsgCountFree(pMsgCount* mc)
{
	pMsgCount freeptr=NULL;

	if(mc!=NULL){
		freeptr=*mc;
	}

	if(freeptr!=NULL){
		memset(freeptr,0,sizeof(MsgCount));
		free(freeptr);
		*mc=NULL;
	}
}

CCLAPI 
int MsgCountGetCount(pMsgCount mc,MsgCountType mct)
{
	if(mc!=NULL){
		if((mct>=MCT_NEW)&&(mct<=MCT_UOLD)){
			return mc->msgsize[mct];
		}else{
			return -1;
		}
	}else
		return -1;
}


CCLAPI 
RCODE MsgCountSetCount(pMsgCount mc,MsgCountType mct ,int size)
{
	if(NULL==mc){
		return RC_ERROR;
	}else{
		if((mct>=MCT_NEW)&&(mct<=MCT_UOLD)){
			mc->msgsize[mct]=size;
			return RC_OK;
		}else{
			return RC_ERROR;
		}
	}
}


CCLAPI 
pMsgCount MsgCountNewFromStr(const char* body)
{
	char* slash=NULL;
	char* lparen=NULL;
	char* rparen=NULL;
	char *buf=NULL,*freeptr=NULL;
	int newmsg=0,oldmsg=0,newumsg=0,oldumsg=0; 

	pMsgCount ret=NULL;

	buf=strDup(body);
	if(NULL==buf){
		return NULL;
	}
	freeptr=buf;

	slash=strchr(buf,'/');
	if(NULL==slash){
		free(freeptr);
		return NULL;
	}else{
		lparen=strchr(slash,'(');
		*slash='\0';
	}

	/* get new msg count */
	newmsg=atoi(buf);
	buf=slash+1;
	
	if(lparen !=NULL){
		rparen=strchr(lparen,')');
		*lparen='\0';
	}

	oldmsg=atoi(buf);

	/* it has urgent part */
	if(rparen!=NULL){
		*rparen = '\0';

		buf=lparen+1;
		slash=strchr(buf,'/');
		if(slash!=NULL){
			*slash='\0';
			newumsg=atoi(buf);
			buf=slash+1;
			oldumsg=atoi(buf);
		}
	}

	ret=MsgCountNew(newmsg,oldmsg,newumsg,oldumsg);

	free(freeptr);

	return ret;
	
}

CCLAPI 
char* MsgCountToStr(pMsgCount mc)
{
	char* ret=NULL;
	char buf[64];

	if(NULL==mc){
		return NULL;
	}

	memset(buf,0,sizeof(buf));
	if(mc->msgsize[MCT_UNEW]>-1){
		sprintf(buf,"%u/%u (%u/%u)",mc->msgsize[MCT_NEW],mc->msgsize[MCT_OLD],mc->msgsize[MCT_UNEW],mc->msgsize[MCT_UOLD]);
	}else{
		sprintf(buf,"%u/%u",mc->msgsize[MCT_NEW],mc->msgsize[MCT_OLD]);
	}
	
	ret=strDup(buf);
	return ret;
}

