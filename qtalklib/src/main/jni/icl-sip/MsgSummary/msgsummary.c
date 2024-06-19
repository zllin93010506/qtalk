/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * msgsummary.c
 *
 * $Id: msgsummary.c,v 1.4 2009/07/30 02:10:37 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <adt/dx_lst.h>
#include <adt/dx_str.h>
#include <common/cm_utl.h>

#include <sip/rfc822.h>

#include "msgsummary.h"
#include "msgcount.h"

struct MsgSummaryObj{
	BOOL status;
	char *account_uri;

	pMsgCount voicemessage; /* for voice-message only */
	DxLst opt_headers;

};

CCLAPI
pMsgSum MsgSumNew(BOOL msgstatus,const char* accounturi,pMsgCount voicesummary)
{
	pMsgSum msg;

	/* check input parameters */
	msg=calloc(1,sizeof(MsgSum));
	if( NULL==msg ) {
		printf("[MsgSumNew] Memory allocation error\n" );
		return NULL;
	}

	msg->status=msgstatus;
	if(NULL!=accounturi){
		msg->account_uri=strDup(accounturi);
		msg->account_uri=unanglequote(msg->account_uri);
	}
	msg->voicemessage= MsgCountDup(voicesummary);
	
	return msg;

}

CCLAPI
void MsgSumFree(pMsgSum* msg)
{
	pMsgSum freeptr;

	if(NULL==msg)
		return;

	freeptr=*msg;


	if(freeptr->account_uri!=NULL)
		free(freeptr->account_uri);

	if(freeptr->voicemessage!=NULL)
		MsgCountFree(&freeptr->voicemessage);

	memset(freeptr,0,sizeof(MsgSum));
	free(freeptr);

	*msg=NULL;
	return;
}

CCLAPI
BOOL MsgSumGetStatus(pMsgSum msg)
{
	if(NULL==msg){
		printf("[MsgSumGetStatus] NULL pointer to pMsg!\n" );
		return FALSE;
	}else
		return msg->status;
}

CCLAPI 
RCODE MsgSumSetStatus(pMsgSum msg,BOOL newstate)
{
	if(NULL==msg){
		return RC_ERROR;
	}else{
		msg->status=newstate;
		return RC_OK;
	}
}

CCLAPI
char* MsgSumGetAccount(pMsgSum msg)
{
	if(NULL==msg){
		printf("[MsgSumGetStatus] NULL pointer to pMsg!\n" );
		return NULL;
	}else
		return msg->account_uri;

}

CCLAPI pMsgCount MsgSumGetSummary(pMsgSum msg,MsgContext type)
{
	if(NULL==msg){
		printf("[MsgSumGetSummary] NULL pointer to pMsg!\n" );
		return NULL;
	}else{
		if(MC_VOICE==type)
			return msg->voicemessage;
		else
			return NULL;
	}
	
}

CCLAPI 
RCODE MsgSumSetSummary(pMsgSum msg,MsgContext type,pMsgCount mc)
{
	if(NULL==msg){
		return RC_ERROR;
	}else{
		if(MC_VOICE==type){
			if(msg->voicemessage!=NULL){
				MsgCountFree(&msg->voicemessage);
			}

			msg->voicemessage=MsgCountDup(mc);

			return RC_OK;
		}else
			return RC_ERROR;

	}
}

CCLAPI
pMsgSum MsgSumNewFromStr(const char* body)
{
	pMsgSum newmsg=NULL;
	MsgHdr hdr,tmp;
	int hdrlength=0;



	/* parse header */
	hdr = msgHdrNew(); /* temporary create, free later */

	if( rfc822_parse((unsigned char *)body, &hdrlength, hdr)==0 ){
		printf("[MsgSumNewFromStr] NULL pointer to pMsg!\n" );
		return NULL;
	}

	tmp = hdr;

	if(strICmp(tmp->name,"Messages-Waiting")==0){
			/* must be only one */
		if(tmp->first->content !=NULL){
			char *content=trimWS(tmp->first->content);
			if(strcmp(content,"yes")==0){
				newmsg=MsgSumNew(TRUE,NULL,NULL);
			}else if (strcmp(content,"no")==0){
				newmsg=MsgSumNew(FALSE,NULL,NULL);
			}else
				goto msgparseend;
		}else
			goto msgparseend;
	}else
		goto msgparseend;

	tmp=tmp->next;

	while(tmp && tmp->name){
			
		if(strICmp(trimWS(tmp->name),"Message-Account")==0){
			newmsg->account_uri=strDup(unanglequote(tmp->first->content));
		}else if(strICmp(trimWS(tmp->name),"Voice-Message")==0){
			newmsg->voicemessage=MsgCountNewFromStr(tmp->first->content);
		}else{
			/* option headers */
			/* to-do */
		}

		tmp=tmp->next;
	}

msgparseend:
	msgHdrFree(hdr);

	return newmsg;
}

CCLAPI
char* MsgSumToStr(pMsgSum msg)
{
	DxStr tmpstr=NULL;
	char* ret=NULL;

	if(NULL==msg){
		return NULL;
	}
	
	/* add Messages-Waiting: */
	tmpstr=dxStrNew();
	if(NULL==tmpstr){
		return NULL;
	}
	
	dxStrCat(tmpstr,"Messages-Waiting: ");
	if(msg->status)
		dxStrCat(tmpstr,"yes");
	else
		dxStrCat(tmpstr,"no");

	dxStrCat(tmpstr,"\r\n");
	

	/* add Messages-Account: */
	if(msg->account_uri){
		dxStrCat(tmpstr,"Messages-Account: ");
		dxStrCat(tmpstr,unanglequote(msg->account_uri));
		dxStrCat(tmpstr,"\r\n");
	}

	/* add Summary-line: */
	if(msg->voicemessage){
		char *mcstr=MsgCountToStr(msg->voicemessage);

		if(NULL!=mcstr){
		
			dxStrCat(tmpstr,"Voice-Message: ");
			dxStrCat(tmpstr,mcstr);
			dxStrCat(tmpstr,"\r\n");


			free(mcstr);
			mcstr=NULL;
		}
	}

	ret=strDup(dxStrAsCStr(tmpstr));
	dxStrFree(tmpstr);
	
	return ret;

}

