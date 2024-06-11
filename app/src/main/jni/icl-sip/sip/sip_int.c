/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_int.c
 *
 * $Id: sip_int.c,v 1.2 2009/03/04 06:28:11 tyhuang Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <common/cm_utl.h>
#include <common/cm_trace.h>
#include <adt/dx_str.h>

#include "sip_hdr.h"
#include "sip_int.h"

SipAccept* AcceptDup(SipAccept *accept )
{
	SipAccept *newaccept=NULL;

	if(accept!=NULL){
	
		newaccept=(SipAccept*)calloc(1,sizeof(SipAccept));
		if(NULL==newaccept){
			return newaccept;
		}

		newaccept->numMediaTypes=accept->numMediaTypes;
		if(accept->sipMediaTypeList != NULL)
			newaccept->sipMediaTypeList=sipMediaTypeDup(accept->sipMediaTypeList,accept->numMediaTypes);
	}
	return newaccept;
}

SipAcceptEncoding* AcceptEncodingDup(SipAcceptEncoding* coding)
{
	SipAcceptEncoding *ret=NULL;	

	if(coding!=NULL){
		ret=(SipAcceptEncoding*)calloc(1,sizeof(SipAcceptEncoding));
		if(NULL==ret){
			return ret;
		}

		ret->numCodings=coding->numCodings;
		
		if(coding->sipCodingList != NULL)
			ret->sipCodingList=sipCodingDup(coding->sipCodingList,coding->numCodings);
	}
	return ret;
}

SipAcceptLang* AcceptLanguageDup(SipAcceptLang* lang)
{
	SipAcceptLang *ret=NULL;

	if(lang!=NULL){
	
		ret=(SipAcceptLang*)calloc(1,sizeof(SipAcceptLang));
		if(NULL==ret){
			return NULL;
		}

		ret->numLangs=lang->numLangs;

		if(lang->sipLangList != NULL)
			ret->sipLangList=sipLanguageDup(lang->sipLangList,lang->numLangs);
	}

	return ret;
}

SipInfo* SipInfoDup(SipInfo *info)
{
	SipInfo *ret=NULL,*nowinfo,*preinfo;
	
	while(info!=NULL){
		nowinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(NULL==nowinfo){
			return ret;
		}

		if(ret != NULL) 
			preinfo->next=nowinfo;
		else
			ret=nowinfo;

		
		nowinfo->absoluteURI=strDup((const char *)info->absoluteURI);
		nowinfo->SipParamList=sipParameterDupAll(info->SipParamList);
		
		info=info->next;
		preinfo=nowinfo;
	}
	return ret;
}

SipAllow* AllowDup(SipAllow *allow)
{
	int idx;

	SipAllow *ret=NULL;

	if(allow!=NULL){
		ret=(SipAllow*)calloc(1,sizeof(SipAllow));
		if(NULL==ret){
			return ret;
		}

		for(idx=0;idx<SIP_SUPPORT_METHOD_NUM;idx++)
			ret->Methods[idx]=allow->Methods[idx];
	}
	
	return ret;
}

SipAllowEvt* AllowEventsDup(SipAllowEvt *allowevt)
{
	SipAllowEvt *tmpallowevt;
	SipAllowEvt *ret=NULL;

	if(NULL==allowevt)
		return NULL;

	ret=(SipAllowEvt*)calloc(1,sizeof(SipAllowEvt));
	if(NULL==ret){
		return ret;
	}else{
		tmpallowevt=ret;
	}

	while(1){
	
		if(allowevt->pack != NULL)
			tmpallowevt->pack = strDup(allowevt->pack);
		if(allowevt->tmplate != NULL)
			tmpallowevt->tmplate = sipStrDup(allowevt->tmplate,-1);
			
		allowevt=allowevt->next;

		if(NULL==allowevt){
			break;
		}else{
			tmpallowevt->next=(SipAllowEvt*)calloc(1,sizeof(SipAllowEvt));
			if(NULL==tmpallowevt){
				return ret;
			}
			tmpallowevt=tmpallowevt->next;
		}

	}/* end of while */
	return ret;
}

SipContEncoding* ContentEncodingDup(SipContEncoding* contcoding)
{
	SipContEncoding *ret=NULL;

	if(contcoding!=NULL){

		ret=(SipContEncoding*)calloc(1,sizeof(SipContEncoding));
		if(NULL==ret){
			return NULL;
		}

		ret->numCodings=contcoding->numCodings;

		if(contcoding->sipCodingList != NULL)
			ret->sipCodingList=sipCodingDup(contcoding->sipCodingList,contcoding->numCodings);
	}
	return ret;
}

SipContType* ContentTypeDup(SipContType* conttype)
{
	SipContType *ret=NULL;

	if(conttype!=NULL){

		ret=(SipContType*)calloc(1,sizeof(SipContType));
		if(NULL==ret){
			return ret;
		}
		
		ret->numParam=conttype->numParam;
		ret->type=strDup(conttype->type);	
		ret->subtype=strDup(conttype->subtype);	
		ret->sipParamList=sipParameterDupAll(conttype->sipParamList);
	}
	
	return ret;
}

SipCSeq* CSeqDup(SipCSeq* seq)
{
	SipCSeq *ret=NULL;

	if(seq!=NULL){
	
		ret=(SipCSeq*)calloc(1,sizeof(SipCSeq));
		if(NULL==ret){
			return ret;
		}

		ret->Method=seq->Method;
		ret->seq=seq->seq;
	}
	return ret;
}

SipDate* DateDup(SipDate* InDate)
{
	SipDate *ret=NULL;

	if(InDate!=NULL){
		
		ret=(SipDate*)calloc(1,sizeof(SipDate));
		if(NULL==ret){
			return ret;
		}

		if(InDate->date!= NULL){
			ret->date=(SipRfc1123Date*)calloc(1,sizeof(SipRfc1123Date));

			if(NULL==ret->date){
				free(ret);
				return NULL;
			}

			ret->date->month=InDate->date->month;
			ret->date->weekday=InDate->date->weekday;

			strcpy(ret->date->year,InDate->date->year);
		
			strcpy(ret->date->day,InDate->date->day);
		
			strcpy(ret->date->time,InDate->date->time);
		}
	}
	
	return ret;
}

SipEncrypt* EncryptionDup(SipEncrypt* encrypt)
{
	SipEncrypt *ret=NULL;

	if(encrypt!=NULL){
	
		ret=(SipEncrypt*)calloc(1,sizeof(SipEncrypt));
		if(NULL==ret){
			return ret;
		}

		ret->numParams=encrypt->numParams;

		ret->scheme=strDup(encrypt->scheme);
		ret->sipParamList=sipParameterDupAll(encrypt->sipParamList);
	}
	return ret;
}

SipEvent*	EventDup(SipEvent *event)
{
	SipEvent *ret=NULL;

	if(!event)
		return NULL;

	ret=(SipEvent*)calloc(1,sizeof(*ret));
	if(NULL==ret){
		return NULL;
	}

	ret->numParam=event->numParam;
	ret->paramList=sipParameterDupAll(event->paramList);
	ret->tmplate=sipStrDup(event->tmplate,-1);
	ret->pack=strDup(event->pack);
		
	return ret;
}

SipExpires* ExpiresDup(SipExpires* expire)
{
	SipExpires *ret=NULL;

	if(expire!=NULL){
		ret=(SipExpires*)calloc(1,sizeof(SipExpires));
		if(NULL==ret){
			return NULL;
		}

		ret->expireSecond=expire->expireSecond;
	/*request->expires->expireDate=(rfc1123Date*)malloc(sizeof(rfc1123Date));*/
	/*request->expires->expireDate = expire->expireDate;*/
	/*strcpy(request->expires->expireDate->day,expire->expireDate->day);*/
	/*strcpy(request->expires->expireDate->year,expire->expireDate->year);*/
	/*strcpy(request->expires->expireDate->time,expire->expireDate->time);*/
	/*request->expires->expireDate = ((expire->expireDate)?rfc1123DateDup(expire->expireDate):NULL);*/
	/*request->expires->expireDate->month=expire->expireDate->month;*/
	/*request->expires->expireDate->weekday=expire->expireDate->weekday;*/
	}

	return ret;
}

SipFrom*	FromDup(SipFrom* from)
{
	return (SipFrom*)ToDup(from);
}

SipMinSE*	MinSEDup(SipMinSE* minse)
{
	return (SipMinSE*)SessionExpiresDup(minse);
}

SipSessionExpires* SessionExpiresDup(SipSessionExpires *_sessionexpires)
{
	SipSessionExpires *ret=NULL;

	if(_sessionexpires != NULL){
		ret=(SipSessionExpires*)calloc(1,sizeof(SipSessionExpires));
		if(NULL==ret){
			return NULL;
		}
		ret->deltasecond=_sessionexpires->deltasecond;
		ret->paramList=sipParameterDupAll(_sessionexpires->paramList);
	}
	return ret;
}

SipPrivacy * PrivacyDup(SipPrivacy *pPrivacy)
{
	SipPrivacy *ret=NULL;

	if(pPrivacy != NULL){

		ret=(SipPrivacy*)calloc(1,sizeof(SipPrivacy));
		if(NULL==ret){
			return NULL;
		}
		
		ret->numPriv=pPrivacy->numPriv;
		ret->privvalue=sipStrDup(pPrivacy->privvalue,pPrivacy->numPriv);
		
	}
	return ret;
}

SipReferSub* ReferSubDup(SipReferSub* refersub)
{
	SipReferSub *ret=NULL;

	if(refersub != NULL){
		ret=(SipReferSub*)calloc(1,sizeof(SipReferSub));
		if(NULL==ret){
			return NULL;
		}

		ret->value=refersub->value;

		if(refersub->paramList != NULL)
			ret->paramList=sipParameterDupAll(refersub->paramList);
	}
	
	return ret;
}


SipReferTo* ReferToDup(SipReferTo* referto)
{
	return ToDup((SipTo*)referto);
}

SipReferredBy* ReferredByDup(SipReferredBy* referredby)
{
	return ToDup((SipTo*)referredby);
}

SipReplyTo* ReplyToDup(SipReplyTo* _replyto)
{
	return ToDup((SipTo*) _replyto);
}

SipRequire*	RequireDup(SipRequire* require)
{
	return SupportedDup((SipSupported *)require);
}

SipServer*	ServerHdrDup(SipServer* ser)
{
	return UserAgentDup((SipUserAgent *)ser);
}

SipSupported* SupportedDup(SipSupported *sup)
{
	SipSupported *ret=NULL;

	if(NULL != sup){
		ret=(SipSupported*)calloc(1,sizeof(SipSupported));
		ret->numTags=sup->numTags;
		ret->sipOptionTagList=sipStrDup(sup->sipOptionTagList,sup->numTags);
	}

	return ret;
}

SipTo* ToDup(SipTo* toList)
{
	SipTo *ret=NULL;
	
	if(toList!=NULL){

		ret=(SipTo*)calloc(1,sizeof(SipTo));
		if(NULL==ret){
			return NULL;
		}

		ret->numAddr=toList->numAddr;
		ret->numParam=toList->numParam;

		if(toList->address != NULL)
			ret->address=sipAddrDup(toList->address,toList->numAddr);
	
		ret->paramList=sipParameterDupAll(toList->paramList);
	}

	return ret;
}

SipUnsupported*	UnsupportedDup(SipUnsupported *usup)
{
	return SupportedDup((SipSupported *)usup);
}

SipUserAgent* UserAgentDup(SipUserAgent* agent)
{
	SipUserAgent *ret=NULL;
	
	if(agent!=NULL){
	
		ret=(SipUserAgent*)calloc(1,sizeof(SipUserAgent));
		if(NULL==ret){
			return NULL;
		}
		
		ret->numdata=agent->numdata;
		ret->data=sipStrDup(agent->data,agent->numdata);
	}
	return ret;
}

