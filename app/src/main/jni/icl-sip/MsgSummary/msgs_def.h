/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * msgs_def.h
 *
 * $Id: msgs_def.h,v 1.2 2009/05/25 02:48:17 tyhuang Exp $
 */

#ifndef MSGS_DEF_H
#define MSGS_DEF_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct MsgCountObj			MsgCount;
typedef struct MsgCountObj*			pMsgCount;
typedef struct MsgSummaryObj		MsgSum;
typedef struct MsgSummaryObj*		pMsgSum;

typedef enum {
	MC_VOICE=0,    /* voice mail message */
	MC_FAX,		   /* fax mail message */
	MC_PAGER,	   /* page message */
	MC_MULTIMEDIA, /* multimedia message */
	MC_TEXT,	/* text message */
	MC_NONE /* unknown message */
}MsgContext;

typedef enum {
	MCT_NEW =0,
	MCT_OLD =1,
	MCT_UNEW =2,
	MCT_UOLD =3
}MsgCountType;


#ifdef  __cplusplus
}
#endif

#endif /* MSGS_DEF_H */
