/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * CTransactionDatabase.c
 *
 * $Id: CTransactionDatabase.c,v 1.13 2009/03/11 07:48:12 tyhuang Exp $
 */
#include "net.h"
#include "TxStruct.h"
#include "Transport.h"
#include "CTransactionDatabase.h"
#include "sipTx.h"

#include <adt/dx_hash.h>

typedef struct structTransactionDbRecord {
	char * strKey;
	TxStruct pTx;
} TTransactionDbRecord;

void InitializeTransactionRecord ( TTransactionDbRecord * pRecord );
BOOL TryFreeTransactionRecord ( TTransactionDbRecord * pRecord );
unsigned char * GetCallIdFromTx ( const TxStruct pTx );

struct structTransactionDB_C {
	BOOL bMatchACK_OF_2XX_TO_INVITE;
	TTransactionDbRecord * pRecords;
	int iCapacity;
	DxHash txHash;
	unsigned int numbercount;
};

/* the constructor of the database */
PCTransactionDB_C CreateTransactionDB ( /*const*/ int iCapacity , BOOL bMatchACK_OF_2XX_TO_INVITE) {
	
	/* initialize the result pointer to NULL */
	TTransactionDB_C * pDB = NULL;

	/* validate arguments */
	if ( 0 >= iCapacity ) {
		return NULL;
	}

	/* allocate memory for the result structure */
	pDB = (TTransactionDB_C*)calloc (1, sizeof(TTransactionDB_C) );

	/* check the result of memory allocation */
	if ( pDB ) {
		
		/* fill the memory block with 0 
		( some people would advise patterns like 0xA3B4 ) 
		memset ( pDB, 0, sizeof(TTransactionDB_C) );*/

		/* setup initial values for all member data */ 
		pDB->bMatchACK_OF_2XX_TO_INVITE = bMatchACK_OF_2XX_TO_INVITE;
		pDB->iCapacity = 0;
		pDB->pRecords = NULL;

		/* allocate the vector here */
		pDB->pRecords = (TTransactionDbRecord*) 
			calloc ( 1, sizeof(TTransactionDbRecord) * iCapacity );

		pDB->txHash = dxHashNew( iCapacity, FALSE, DX_HASH_POINTER );

		/* in case the memory allocation shall fail */ 
		if ( pDB->pRecords ) {

			/* initialize all records 
			int i = 0;
			for ( i = 0; i < iCapacity; ++i ) {
				InitializeTransactionRecord ( pDB->pRecords + i );
			} mark by tyhuang 2006/3/10
			*/
			
			/* set the capacity mark */
			pDB->iCapacity = iCapacity;
		}
		else {

			/* destroy the database and set the pointer to NULL */
			DestroyTransactionDB ( &pDB );
		}
	}

	return pDB;
}

/* the constructor of some record */
void InitializeTransactionRecord ( TTransactionDbRecord * pRecord ) {
	
	/*assert ( pRecord );mark by tyhuang 2006/3/10 */

	if ( pRecord ) {

		memset ( pRecord, 0, sizeof (TTransactionDbRecord) );

		pRecord->strKey = NULL;
		pRecord->pTx = NULL;
	}
}


/* the destructor of some record */
BOOL FreeTransactionRecord ( TTransactionDbRecord * pRecord ) {
	/* NULL pointers are allowed here */
	if ( pRecord ) {

		if ( pRecord->strKey ) {
			/* we don't have to de-allocate the memory */
			/* invalidate the pointer */
			pRecord->strKey = NULL;
		}

		/* check the reference counter here */
		if ( pRecord->pTx ) {
			/* we don't have to de-allocate the memory */
			/* invalidate the pointer */
			pRecord->pTx  = NULL;
			return TRUE;
		}
	}
	return TRUE;
}

RCODE RemoveTransaction ( PCTransactionDB_C pDB, const TxStruct pTx ) {
	
	/*
	assert ( pDB );
	assert ( pTx );
	mark by tyhuang 2006/3/10 */
	char *strKey;
	
	if ( pDB && pTx ) {
		/* new code 
		if( pTx->magicBranch )
			strKey = pTx->magicBranch;
		else
			strKey = (char *)GetCallIdFromTx ( pTx );
		*/
		strKey = (char *)GetCallIdFromTx ( pTx );
		return dxHashDelEx( pDB->txHash, (const char *)strKey, pTx );
	}

	return RC_ERROR;
}

/* the destructor of the database */
void DestroyTransactionDB ( TTransactionDB_C ** const ppDB ) 
{

	char *strKey;

	/*assert ( ppDB ); mark by tyhuang 2006/3/10 */
	
	/* validate arguments */
	if ( ppDB ) {

		/* the ANSI assert will try to evaluate the expression no matter
		it is compiled under DEBUG or RELEASE mode, be careful ! */
		/*assert ( *ppDB );mark by tyhuang 2006/3/10 */

		/* validate arguments */
		if ( *ppDB ) {

			/* if the vector has been allocated, free it */
			if ( (*ppDB)->pRecords ) {
				
				int i = 0;
				Entry hashlist;
				char *key;
				TxStruct pTx;
				void* tmp = NULL;

				/* try to free every transaction state */
				for ( i = 0; i < (*ppDB)->iCapacity; ++i ) {
					FreeTransactionRecord ( (*ppDB)->pRecords + i );
				}

				/* free the whole vector here */
				free ( (*ppDB)->pRecords );

				/*
				 * modified by tyhuang
				 */
				/* free the transaction Hashtable */
				if((*ppDB)->txHash){
					StartKeys((*ppDB)->txHash);
					while ((key=NextKeys((*ppDB)->txHash))) {
						hashlist=dxHashItemLst((*ppDB)->txHash,key);
						while(hashlist){
							pTx=(TxStruct)dxEntryGetValue(hashlist);
							hashlist=dxEntryGetNext(hashlist);
							if (!pTx) 
								continue; 
							if( pTx->magicBranch )
								strKey = pTx->magicBranch;
							else
								strKey = (char *)GetCallIdFromTx ( pTx );
							dxHashDelEx((*ppDB)->txHash,strKey,(void*)pTx);
							if (NULL != pTx->txid) free(pTx->txid);
							if (NULL != pTx->OriginalReq) sipReqFree(pTx->OriginalReq);
							if (NULL != pTx->Ack) sipReqFree(pTx->Ack);
							if (NULL != pTx->LatestRsp) sipRspFree(pTx->LatestRsp);
							if (NULL != pTx->profile) UserProfileFree(pTx->profile);
							if (-1 != pTx->TimerHandle1) CommonTimerDel(pTx->TimerHandle1,&tmp);
							if (-1 != pTx->TimerHandle2) CommonTimerDel(pTx->TimerHandle2,&tmp);
							if (-1 != pTx->TimerHandle3) CommonTimerDel(pTx->TimerHandle3,&tmp);

							if (NULL != pTx->transport) ReleaseTcpConnection ( pTx->transport );
							if (NULL != pTx->raddr) free(pTx->raddr);

							if (NULL != pTx->mutex) cxMutexFree(pTx->mutex);

							free(pTx);
						}
					}
				}

				dxHashFree( (*ppDB)->txHash, NULL );

				/* set all invalid pointers to NULL */
				(*ppDB)->pRecords = NULL;
				(*ppDB)->iCapacity = 0;
				(*ppDB)->txHash = NULL;
			}

			/* clear the pointer, so the caller will not 
			be able to use it anymore */
			free(*ppDB); /* ljchuang */
			*ppDB = NULL;
		}
	}
}

unsigned int GetTransactionDBSize(TTransactionDB_C ** const ppDB)
{
	if((NULL==ppDB)||(NULL==*ppDB)){
		return 0;
	}else{
		return dxHashSize((*ppDB)->txHash);
	}
}

/* get call-id from a transaction state and copy into the buffer */
unsigned char * GetCallIdFromTx ( const TxStruct pTx ) {
	SipReq req=sipTxGetOriginalReq(pTx);
	SipRsp rsp;
	if ( req ) {
		/* get a copy of the call id */
		return sipReqGetHdr ( req, Call_ID );
	}

	rsp=sipTxGetLatestRsp(pTx);
	if ( rsp ) {
		/* get a copy of the call id */
		return sipRspGetHdr ( rsp, Call_ID );
	}
	return NULL;
}

/* get call-id from a transaction state and copy into the buffer */
unsigned char * GetCallIdFromMsg ( void * const pMsg, const BOOL bIsRequest ) {
	if ( pMsg ) {
		/* get a copy of the call id */
		return bIsRequest ? sipReqGetHdr ( pMsg, Call_ID ) : sipRspGetHdr ( pMsg, Call_ID );
	}
	return NULL;
}


#define MAGIC_BRANCH "z9hG4bK"

/* get magic-cookie branch ID from a transaction state and copy into the buffer */
char * GetMagicBranchFromTx ( const TxStruct pTx ) 
{
	SipViaParm *pVia;
	SipReq msg;

	if ( (msg=sipTxGetOriginalReq(pTx)) ) {
		/* get the pointer of the branch in the top Via */
		pVia = GetTopMostVia ( msg, TRUE);
		if ( pVia ) {
			/* check if this message's topmost via has the new branch ID */
			if ( pVia->branch && (0 == strncmp ( pVia->branch, MAGIC_BRANCH, 7 /* strlen(MAGIC_BRANCH) */ )) ) {
				return pVia->branch;
			}
		}
	}
	return NULL;

}

/* get magic-cookie branch ID from a transaction state and copy into the buffer */
char * GetMagicBranchFromMsg ( void * const pMsg, const BOOL bIsRequest ) 
{
	SipViaParm *pVia;

	if ( pMsg ) {
		/* get the pointer of the branch in the top Via */
		pVia = GetTopMostVia ( pMsg, bIsRequest );
		if ( pVia ) {
			/* check if this message's topmost via has the new branch ID */
			if ( pVia->branch && (0 == strncmp(pVia->branch, MAGIC_BRANCH, 7/* strlen(MAGIC_BRANCH)*/)) ) {
				return pVia->branch;
			}
		}
	}
	return NULL;
}


RCODE InsertTransaction ( PCTransactionDB_C pDB, TxStruct pTx ) 
{

	char *strKey;

	if ( pDB && pTx ){
		pTx->magicBranch=NULL;

		/* new code 
		if( (strKey=GetMagicBranchFromTx( pTx )) ) {
			pTx->magicBranch = strKey;
		} else {
			strKey = (char *)GetCallIdFromTx ( pTx );
		}
		*/
		strKey = (char *)GetCallIdFromTx ( pTx );

		if( strKey ){
			TXTRACE1("[InsertTransaction] %s\n", strKey);
			dxHashAddEx( pDB->txHash, (const char *)strKey, pTx );
			pTx->InternalNum = ++(pDB->numbercount);
			return RC_OK;
		}else{
			dxHashAddEx( pDB->txHash, (const char *)"NULL", pTx );
			pTx->InternalNum = ++(pDB->numbercount);
			return RC_OK;
		}
	}
	return RC_ERROR;
}

TxStruct FindTxFromInternum( PCTransactionDB_C pDB, unsigned short InterNum )
{
	assert ( pDB );

	if ( pDB ) {

		int i = 0;

		/* try to find an empty slot */
		for ( i = 0; i < pDB->iCapacity; ++i ) {
			if ( NULL != ( pDB->pRecords + i )->pTx ) {
				if ( ( pDB->pRecords + i )->pTx->InternalNum == InterNum )
					return ( pDB->pRecords + i )->pTx;
				
			}
		}
	}
	return NULL;
}

TxStruct FindTxFromSipTCP( PCTransactionDB_C pDB, SipTCP tcp )
{
	PTcpConnection pTCP = NULL;
	Entry hashlist = NULL;
	TxStruct pTx = NULL;
	char *key = NULL;

	assert ( pDB );

	pTCP = FindPTcpConnection(tcp);

	if ( pDB && pTCP ) {

		StartKeys(pDB->txHash);
		while ( NULL!=(key=NextKeys(pDB->txHash)) ) {
			hashlist = dxHashItemLst(pDB->txHash,key);
			while (hashlist) {
				pTx = (TxStruct)dxEntryGetValue(hashlist);
		
				if (pTx->transport == pTCP)
					return pTx;

				hashlist=dxEntryGetNext(hashlist);
		
			}
		}
	}
	return NULL;
}

SipViaParm * GetTopMostVia ( void * const pMsg, const BOOL bIsRequest ) {
	if ( pMsg ) {
		SipVia * pVia = bIsRequest ? sipReqGetHdr ( pMsg, Via ) : sipRspGetHdr ( pMsg, Via );
		if ( pVia ) {
			return pVia->ParmList;
		}
	}
	return NULL;
}

SipMethodType GetRequestMethod ( const SipReq pMsg ) {
	SipReqLine * const pRequestLine = sipReqGetReqLine ( pMsg );
	if ( pRequestLine ) {
		return pRequestLine->iMethod;
	}
	return UNKNOWN_METHOD;
}

BOOL IsRequestUriEqual ( SipReq pMsg1, SipReq pMsg2 ) {
	if ( pMsg1 && pMsg2 ) {
		SipReqLine * pReq1 = sipReqGetReqLine ( pMsg1 );
		SipReqLine * pReq2 = sipReqGetReqLine ( pMsg2 );

		if ( pReq1 && pReq2 ) {
			if ( 0 == strCmp ( pReq1->ptrRequestURI, pReq2->ptrRequestURI ) ) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

char * GetTag ( SipParam * const pParamListHead ) {
	SipParam * pItem = pParamListHead;
	for (; pItem; pItem = pItem->next ) {
		if ( 0 == strICmp ( "TAG", pItem->name ) ) {
			return pItem->value;
		}
	}
	return NULL;
}

BOOL IsFromTagEqual (  SipReq pMsg1, SipReq pMsg2 ) {
	SipFrom * pFrom1 = sipReqGetHdr ( pMsg1, From );
	SipFrom * pFrom2 = sipReqGetHdr ( pMsg2, From );

	/* Both tags don't exist, return true */
	if ( !pFrom1 && !pFrom2 )
		return TRUE;

	if ( pFrom1 && pFrom2 ) {
		char * pTag1 = GetTag ( pFrom1->paramList );
		char * pTag2 = GetTag ( pFrom2->paramList );

		if ( ( pTag1 == NULL ) && ( pTag2 == NULL ) )
			return TRUE;

	
		if ( 0 == strCmp ( pTag1, pTag2 ) ) {
				return TRUE;
		}
	}
	return FALSE;
}

BOOL IsViaValid ( SipViaParm * pVia ) {
	if ( pVia ) {
		if ( ! pVia->sentby ) {
			return FALSE;
		}
		if ( ! pVia->protocol ) {
			return FALSE;
		}
		if ( ! pVia->version ) {
			return FALSE;
		}
		if ( !pVia->transport ) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/* these fields can be both NULL, but must be identical if defined */
#define TestViaFieldEqual(V1,V2,F)				\
	if ( V1->F ) {						\
		if ( ! V2->F ) {				\
			return FALSE;				\
		}						\
		if ( 0 != strICmp ( V1->F, V2->F ) ) {		\
			return FALSE;				\
		}						\
	}							\
	else if ( V2->F ) {					\
		return FALSE;					\
	}

BOOL IsViaEqual ( SipViaParm * pVia1, SipViaParm * pVia2 ) {

	if ( ! IsViaValid ( pVia1 ) ) {
		return FALSE;
	}
	if ( ! IsViaValid ( pVia2 ) ) {
		return FALSE;
	}

	if ( 0 != strCmp ( pVia1->sentby, pVia2->sentby ) ) {
		return FALSE;
	}
	if ( 0 != strCmp ( pVia1->protocol, pVia2->protocol ) ) {
		return FALSE;
	}
	if ( 0 != strCmp ( pVia1->version, pVia2->version ) ) {
		return FALSE;
	}
	if ( 0 != strCmp ( pVia1->transport, pVia2->transport ) ) {
		return FALSE;
	}

	/* compare URI parameters */
	/* marked by tyhuang 2007/7/18
	TestViaFieldEqual ( pVia1, pVia2, branch )
	*/
	TestViaFieldEqual ( pVia1, pVia2, comment )
	TestViaFieldEqual ( pVia1, pVia2, maddr )
	TestViaFieldEqual ( pVia1, pVia2, received )

	if ( pVia1->hidden != pVia2->hidden ) {
		return FALSE;
	}

	if ( pVia1->ttl != pVia2->ttl ) {
		return FALSE;
	}

	return TRUE;
}

BOOL IsToTagEqual (  SipReq pMsg1, void * pMsg2, BOOL IsRequest ) {
	SipTo * pTo1 = sipReqGetHdr ( pMsg1, To );
	SipTo * pTo2 = IsRequest ? sipReqGetHdr ( pMsg2, To ) : sipRspGetHdr ( pMsg2, To );

	/* Both tags don't exist, return true */
	if ( !pTo1 && !pTo2 )
		return TRUE;

	if ( pTo1 && pTo2 ) {
		char * pTag1 = GetTag ( pTo1->paramList );
		char * pTag2 = GetTag ( pTo2->paramList );

		if ( ( pTag1 == NULL ) && ( pTag2 == NULL ) )
			return TRUE;

		
		if ( 0 == strCmp ( pTag1, pTag2 ) ) {
			return TRUE;
		}
		
	}
	return FALSE;
}

TxStruct matchingTxMagicCookie(PCTransactionDB_C pDB, void *const pMsg, const BOOL bIsRequest, BOOL bMatchCancel , char *pDBKey)
{
	TxStruct pResultTx = NULL;
	Entry tmpEntry; 
	SipReq pMsg_Y;
	SipCSeq *pCSeq_X, *pCSeq_Y;
	SipViaParm *pVia_x, *pVia_y;
	SipMethodType method_y , method_x;


	if ( !( pDB && pMsg && pDBKey ) ) 
		return NULL;

	tmpEntry = dxHashItemLst( pDB->txHash, pDBKey );

	while (tmpEntry) {

		const TxStruct pTx = (const TxStruct) dxEntryGetValue(tmpEntry);

		tmpEntry = dxEntryGetNext(tmpEntry);
		if( pTx ){
			if( ! pTx->magicBranch )
				/* non-magic-cookie branch */
				continue;
			/* we don't match TX_NON_ASSIGNED, TX_SERVER_ACK, TX_CLIENT_ACK & TX_HOLDER transaction */
			switch ( sipTxGetType( pTx ) ) {
				case TX_CLIENT_INVITE:
				case TX_CLIENT_NON_INVITE:
					/* we don't match request to Client Transation */
					if ( bIsRequest )
						continue;

					break;

				case TX_SERVER_INVITE:
				case TX_SERVER_NON_INVITE:
				case TX_CLIENT_2XX:
					/* we don't match response to Server Transation */
					if ( ! bIsRequest )
						continue;

					break;

				case TX_NON_ASSIGNED:
				case TX_SERVER_ACK:
				case TX_CLIENT_ACK:
				case TX_HOLDER:
					continue;
				default:
					break;
			}
			/* we don't match messages with terminated transactions */
			if ( TX_TERMINATED == sipTxGetState ( pTx ) ) {
				continue;
			}

			/* if X is a response message, match according to Via Branch and CSeq method */
			if ( !bIsRequest ) {
				/* first, the branch ID must match */
				if ( 0 != strCmp ( pTx->magicBranch, pDBKey ) ) {
					/* if the DBKey didn't use branchID any more, the code here must be modifid */
					continue;
				}

				/* second, the method in CSeq must match */
				pMsg_Y = sipTxGetOriginalReq ( pTx );
	
				pCSeq_X = (SipCSeq *)sipRspGetHdr ( pMsg, CSeq );
				if( ! pCSeq_X ) /* pMsg format error, no matching Tx , break from the loop */
					break;

				pCSeq_Y = (SipCSeq *)sipReqGetHdr ( pMsg_Y, CSeq );
				if( ! pCSeq_Y ) 
					continue;

				if (( pCSeq_X->Method == pCSeq_Y->Method ) && (pCSeq_X->seq == pCSeq_Y->seq)) {
					/* we have found the answer, break from the loop ! */
					pResultTx = pTx;
					break;
				}

			} else { /* pMsg is a request */
				/* first, the branch ID must match */
				if ( 0 != strCmp ( pTx->magicBranch, pDBKey ) ) {
					/* if the DBKey didn't use branchID any more, the code here must be modifid */
					continue;
				}
							
				/* second, the send-by in top Via must match */
				pMsg_Y = sipTxGetOriginalReq ( pTx );

				pVia_x = GetTopMostVia ( pMsg, bIsRequest );
				if ( !pVia_x ) /* pMsg format error, no matching Tx , break from the loop */
					break;

				pVia_y = GetTopMostVia ( pMsg_Y, TRUE);
				if( ! pVia_y ) 
					continue;

				/* compare Via sent by (host:port) this is an inaccurate comparison ! */
				if ( 0 != strCmp ( pVia_x->sentby, pVia_y->sentby ) ) 
						continue;

				/* third, the method must match , exception : ACK, CANCEL */
				method_x = GetRequestMethod ( pMsg );
				method_y = GetRequestMethod ( pMsg_Y );

				if( ACK == method_x ){
					if( INVITE != method_y )
						continue;
				} else if( CANCEL == method_x ){
					if( ( bMatchCancel && method_x == method_y ) ||
					    (!bMatchCancel && method_x != method_y ) )
						continue;
				} else {
					if( method_x != method_y )
						continue;
				}
				pResultTx = pTx;
				break;
			} /*  if ( bIsResponse ) */

		} /* if (pTx) */

	} /*  while (tmpEntry) */

	return pResultTx;
}


TxStruct matchingTxRule2543 ( PCTransactionDB_C pDB, void * const pMsg, const BOOL bIsRequest, BOOL bMatchCancel, char *pDBKey )
{
	TxStruct pResultTx = NULL;
	Entry tmpEntry;
	SipReq pMsg_Y;
	char *callid_x, *callid_y;
	SipViaParm *via_x , *via_y;
	SipCSeq *pCSeq_X, *pCSeq_Y;
	SipMethodType method_x, method_y;

	if ( !( pDB && pMsg && pDBKey ) ) 
		return NULL;

	if ( ! bIsRequest) 
		return NULL;


	if ( !( pDB && pMsg && pDBKey ) ) 
		return NULL;

	tmpEntry = dxHashItemLst( pDB->txHash, pDBKey );

	while (tmpEntry) {

		const TxStruct pTx = (const TxStruct) dxEntryGetValue(tmpEntry);

		tmpEntry = dxEntryGetNext(tmpEntry);

		if( pTx ){
			/* we don't match TX_NON_ASSIGNED, TX_SERVER_ACK, TX_CLIENT_ACK & TX_HOLDER transaction */
			switch ( sipTxGetType( pTx ) ) {

				case TX_SERVER_INVITE:
				case TX_SERVER_NON_INVITE:
				case TX_CLIENT_2XX:
					break;

				case TX_CLIENT_INVITE:
				case TX_CLIENT_NON_INVITE:
				case TX_NON_ASSIGNED:
				case TX_SERVER_ACK:
				case TX_CLIENT_ACK:
				case TX_HOLDER:
					continue;
				default:
					break;
			}
			/* we don't match messages with terminated transactions */
			if ( TX_TERMINATED == sipTxGetState ( pTx ) ) 
				continue;

			pMsg_Y = sipTxGetOriginalReq ( pTx );

			/* first, the Request-uri must match */
			if ( ! IsRequestUriEqual ( pMsg, pMsg_Y ) ) 
				continue;

			/* second, the From tag must match */
			if ( ! IsFromTagEqual ( pMsg, pMsg_Y ) ) 
				continue;

			/* third, the Call-ID must match */
			callid_x = (char *)GetCallIdFromMsg ( pMsg, bIsRequest );
			callid_y = (char *)GetCallIdFromMsg ( pMsg_Y, TRUE);
			if ( 0 != strCmp ( callid_x, callid_y ) ) 
				continue;

			/* fourth, the top Via must match */
			via_x = GetTopMostVia ( pMsg, bIsRequest );
			via_y = GetTopMostVia ( pMsg_Y, TRUE);
			 if ( ! IsViaEqual ( via_x, via_y ) ) 
				continue;

			pCSeq_X = (SipCSeq *)sipRspGetHdr ( pMsg, CSeq );
			if( ! pCSeq_X ) /* pMsg format error, no matching Tx , break from the loop */
				break;

			pCSeq_Y = (SipCSeq *)sipReqGetHdr ( pMsg_Y, CSeq );
			if( ! pCSeq_Y ) 
				continue;

			/* fifth, the CSeq number must match */
			if ( pCSeq_X->seq != pCSeq_Y->seq )
				continue;

			method_x = GetRequestMethod ( pMsg );
			method_y = GetRequestMethod ( pMsg_Y );

			if( INVITE == method_x ) {
				if( INVITE != method_y )
					continue;

				if ( ! IsToTagEqual ( pMsg, pMsg_Y, TRUE ) )
					continue;

				if ( pCSeq_X->Method != pCSeq_Y->Method )
					continue;

			} else if( ACK == method_x ) {
				if( INVITE != method_y )
					continue;

				if ( ! IsToTagEqual ( pMsg, sipTxGetLatestRsp(pTx), FALSE) )
					continue;

			} else { /* other method */
				if ( ! IsToTagEqual ( pMsg, pMsg_Y, TRUE ) ) 
					continue;

				if ( pCSeq_X->Method != pCSeq_Y->Method ){
					/*
					 the invite server statemachine doesn't want
					 any cancel to be matched in.
					 this mechanism was copied from the proxy,
					 but this user agent doesn't inherite the same
					 statemachine design.
					*/
					if ( CANCEL != method_x || ! bMatchCancel ) 
						continue;
				}
			}
			pResultTx = pTx;
			break;
		} /* if (pTx) */

	} /*  while (tmpEntry) */

	return pResultTx;
}


TxStruct FindMatchingTransaction2 ( PCTransactionDB_C pDB, void * const pMsg, const BOOL bIsRequest, BOOL bMatchCancel )
{
	char *branch_x;
	char *callID_x;

	assert ( pDB );
	assert ( pMsg );

	if ( !( pDB && pMsg ) ) {
		return NULL;
	}

	branch_x = GetMagicBranchFromMsg( pMsg, bIsRequest );

	if( branch_x ) {
		return matchingTxMagicCookie ( pDB, pMsg, bIsRequest, bMatchCancel, branch_x) ;
	}

	/* if it is a response message, it must has the new branch ID */
	if ( ! bIsRequest ) {
		/* we can never match a response without the new branch ID */
		return NULL;
	}

	callID_x = (char *)GetCallIdFromMsg ( pMsg, bIsRequest );

	return matchingTxRule2543 ( pDB, pMsg, bIsRequest, bMatchCancel, callID_x) ;
}

TxStruct FindMatchingTransaction( PCTransactionDB_C pDB, void * const pMsg, const BOOL bIsRequest, BOOL bMatchCancel ) 
{
	/* simple reference or state variables that don;t need special care of */
	const char * const strMagicBranch = "z9hG4bK";
	SipViaParm * pVia_X = NULL;
	BOOL bViaBranchConformToNewDraft_X = FALSE;
	char * strTopmostViaBranch_X = NULL;
	const BOOL bIsResponse = ! bIsRequest;
	BOOL bShouldCompareViaStrictly = TRUE;
	TxStruct pResultTx = NULL;
	TxStruct pCancelCandidate = NULL;
	BOOL bIsCancel = FALSE;
	SipMethodType method_X = GetRequestMethod ( pMsg );
	SipReq pMsg_Y;
	SipMethodType method_Y;
	SipViaParm * pVia_Y;

	/* must NOT be freed */
	char * strCallID_X = NULL;

	assert ( pDB );
	assert ( pMsg );

	if ( !( pDB && pMsg ) ) {
		return NULL;
	}

	pVia_X = GetTopMostVia ( pMsg, bIsRequest );

	if ( pVia_X ) {
		/* check if this message's topmost via has the new branch ID */
		if ( pVia_X->branch ) {
			if ( 0 == strncmp ( pVia_X->branch, strMagicBranch, 7 /* strlen(strMagicBranch) */ ) ) {
				bViaBranchConformToNewDraft_X = TRUE;
			}
		}

		strTopmostViaBranch_X = pVia_X->branch;
	}

	/* if it is a response message, it must has the new branch ID */
	if ( bIsResponse ) {
		if ( ! bViaBranchConformToNewDraft_X ) {
			/* we can never match a response without the new branch ID */
			TXTRACE0("we can never match a response without the new branch ID\n");
			return NULL;
		}
	}

	/* designed for user agents. they need ACK for 2XX to be matched into INVITE transactions */
	if ( pDB->bMatchACK_OF_2XX_TO_INVITE ) {
		if ( bIsRequest ) {
			if ( ACK == method_X ) {
				bShouldCompareViaStrictly = FALSE;

				bViaBranchConformToNewDraft_X = FALSE;
			}
		}
	}

	strCallID_X = (char *)GetCallIdFromMsg ( pMsg, bIsRequest );

	if(NULL==strCallID_X)
		strCallID_X="NULL";

	if ( strCallID_X ) {
		/*int index = 0;
		for ( index = 0; index < pDB->iCapacity; ++index ) {
			const TTransactionDbRecord * pRecord_Y = pDB->pRecords + index;
			const TxStruct pTx = pRecord_Y->pTx;*/

			Entry tmpEntry = dxHashItemLst( pDB->txHash, strCallID_X );

			while (tmpEntry) {

			const TxStruct pTx = (const TxStruct) dxEntryGetValue(tmpEntry);

			tmpEntry = dxEntryGetNext(tmpEntry);

			if ( pTx ) {
				/*const char * const strCallID_Y = pRecord_Y->strKey;*/

				/* we don't match TX_NON_ASSIGNED, TX_SERVER_ACK, TX_CLIENT_ACK & TX_HOLDER transaction */
				switch ( sipTxGetType( pTx ) ) {
				case TX_NON_ASSIGNED:
				case TX_SERVER_ACK:
				case TX_CLIENT_ACK:
				case TX_HOLDER:
					continue; 

				case TX_CLIENT_INVITE:
				case TX_CLIENT_NON_INVITE:
					/* we don't match request to Client Transation */
					if ( bIsRequest )
						continue;

					break;

				case TX_SERVER_INVITE:
				case TX_SERVER_NON_INVITE:
				case TX_CLIENT_2XX:
					/* we don't match response to Server Transation */
					if ( bIsResponse )
						continue;

					break;
				}

				/* we don't match messages with terminated transactions */
				if ( TX_TERMINATED == sipTxGetState ( pTx ) ) {
					continue;
				}

				/*if ( strCallID_Y ) {
					if ( 0 == strcmp ( strCallID_X, strCallID_Y ) ) {*/

						/* we have a transaction with the same call-id ! */
						pMsg_Y = sipTxGetOriginalReq ( pTx );
						method_Y = GetRequestMethod ( pMsg_Y );
						pVia_Y = NULL;

						assert ( pMsg_Y );

						if ( ! pMsg_Y ) {
							/* actually this is abnormal and should not be allowed */
							continue;
						}

						pVia_Y = GetTopMostVia ( pMsg_Y, TRUE );

						assert ( pVia_Y );

						if ( ! pVia_Y ) {
							continue;
						}

						/* if X is a response message, match according to Via Branch and CSeq method */
						if ( bIsResponse ) {
							
							SipCSeq * pCSeq_X = (SipCSeq *)sipRspGetHdr ( pMsg, CSeq );
							SipCSeq * pCSeq_Y = (SipCSeq *)sipReqGetHdr ( pMsg_Y, CSeq );

							/* first, the branch ID must match */
							char * strTopmostViaBranch_Y = pVia_Y->branch;
							if ( ! strTopmostViaBranch_Y ) {
								continue;
							}

							if ( 0 != strCmp ( strTopmostViaBranch_X, strTopmostViaBranch_Y ) ) {
								continue;
							}

							if ( pCSeq_X && pCSeq_Y ) {
								if (( pCSeq_X->Method == pCSeq_Y->Method ) && (pCSeq_X->seq==pCSeq_Y->seq)) {
									/* we have found the answer, break from the loop ! */
									pResultTx = pTx;
									break;
								}
							} else {
								/*
								 *  todo : should drop response if no cseq header
								 */

								break;
							}
						} else { /* if ( bIsRequest ) */
							/* this (X) is a request message */
							if ( CANCEL == method_X ) {
								bIsCancel = TRUE;
							}

							/* if the message (X) has a new branch ID */
							if ( bViaBranchConformToNewDraft_X ) {

								/* first, the branch ID must match */
								char * strTopmostViaBranch_Y = pVia_Y->branch;
								if ( ! strTopmostViaBranch_Y ) {
									continue;
								}

								/* compare branch ID */
								if ( 0 != strCmp ( strTopmostViaBranch_X, strTopmostViaBranch_Y ) ) {
									continue;
								}

								/* compare Via sent by (host:port) this is an inaccurate comparison ! */
								if ( 0 != strCmp ( pVia_X->sentby, pVia_Y->sentby ) ) {
										continue;
								} 

								/* Changed: 1.6.8 */
								if (ACK==method_X){
									if (INVITE !=method_Y ){
										continue;
									}else{
										pResultTx = pTx;
										break;
									}
								}

								if ( bIsCancel ) {
									/* CANCEL match with any method other then CANCEL */
									/* WARNING: POTENTIAL RISK!		          */
									if ( method_X != method_Y ) {

										/* 
										the invite server statemachine doesn't want 
										any cancel to be matched in.
										this mechanism was copied from the proxy,
										but this user agent doesn't inherite the same
										statemachine design.
										*/

										/*if ( bIsCancel && bMatchCancel ) {
											if ( NULL == pCancelCandidate ) {
												pCancelCandidate = pTx;
											}
										}*/

										if(bMatchCancel && (NULL == pCancelCandidate))
											pCancelCandidate = pTx;

										continue;
									}
								}

								/* we have found the answer */

								/* if method is the same */
								/*
								 *  todo : should check method in cseq
								 */
								if(method_X == method_Y){
									pResultTx = pTx;
									break;
								}else
									continue;
								
							} else { /* if ( bViaBranchConformToNewDraft_X == FALSE ) */
								/* the message (X) is a request without the new branch ID */
								SipCSeq * pCSeq_X = (SipCSeq *)sipReqGetHdr ( pMsg, CSeq );
								SipCSeq * pCSeq_Y = (SipCSeq *)sipReqGetHdr ( pMsg_Y, CSeq );
								
								if ( bShouldCompareViaStrictly ) {
									if ( ! IsRequestUriEqual ( pMsg, pMsg_Y ) ) {
										continue;
									}
								}

								if ( ! IsFromTagEqual ( pMsg, pMsg_Y ) ) {
									/*TXTRACE0("[FindMatchingTransaction] From Tag not equal\n");*/
									continue;
								}

								/* marked by ljchuang 2004/08/24		  */
								/* WARNING: POTENTIAL RISK!		          */
								/*
								if ( 0 != strcmp ( strCallID_X, strCallID_Y ) ) {
									continue;
								}
								*/

								/* marked by tyhuang 2007/7/18
								 due to no magic cookie issue
								 so do not need to compare via's branch */

								if ( bShouldCompareViaStrictly ) {
									if ( ! IsViaEqual ( pVia_X, pVia_Y ) ) {
										continue;
									}
								}
								

								/*
								else {
									comparing Via sent by (host:port) this is an inaccurate comparison !
									if ( pVia_X->sentby && pVia_Y->sentby ) {
										if ( 0 != strcmp ( pVia_X->sentby, pVia_Y->sentby ) ) {
											continue;
										}
									}
									else {
										continue;
									}
								}
								*/

								if ( pCSeq_X && pCSeq_Y ) {
									if ( pCSeq_X->seq != pCSeq_Y->seq ) {
										TXTRACE0("[FindMatchingTransaction] Command sequence not equal\n");
										continue;
									}
								} else {
									continue;
								}

								if ( INVITE == method_X ) {

									/* INVITE only matches with INVITE */
									if ( INVITE != method_Y ) {
										continue;
									}

									if ( ! IsToTagEqual ( pMsg, pMsg_Y, TRUE ) ) {
										continue;
									}

									if ( pCSeq_X->Method != pCSeq_Y->Method ) {
										continue;
									}

									/* we have found the answer, break from the loop ! */
									pResultTx = pTx;
									break;
								} else if ( ACK == method_X ) {

									/* ACK only match with INVITE */
									if ( INVITE != method_Y ) {
										continue;
									}

									/*
									if ( ! IsToTagEqual ( pMsg, sipTxGetLatestRsp(pTx), FALSE ) ) {
										TXTRACE0("[FindMatchingTransaction] To Tag not equal\n");
										continue;
									}
									*/
									/* we have found the answer, break from the loop ! */
									pResultTx = pTx;
									break;
								} else {
									/* Marked this as a quick fix Xavi 2004/12/15 */
									/* WARNING: POTENTIAL RISK!		          */
									/*
									if ( ! IsToTagEqual ( pMsg, pMsg_Y, TRUE ) ) {
										continue;
									}
									*/
									if ( pCSeq_X->Method != pCSeq_Y->Method ) {
										/* 
										the invite server statemachine doesn't want 
										any cancel to be matched in.
										this mechanism was copied from the proxy,
										but this user agent doesn't inherite the same
										statemachine design.
										*/

										TXTRACE0("[FindMatchingTransaction] Be cancel!\n");
										if ( bIsCancel && bMatchCancel ) {
											if ( NULL == pCancelCandidate ) {
												pCancelCandidate = pTx;
											}
										}

										continue;
									}

									pResultTx = pTx;
									break;
								}
							} /* if ( bViaBranchConformToNewDraft_X ) */
						} /* if ( bIsResponse ) */
					/*}*/ /* if ( 0 == strcmp ( strCallID_X, strCallID_Y ) ) */
				/*}*/ /* if ( strCallID_Y ) */
			} /* if ( pTx ) */
		}
		/*}*/ /* for ( index = 0; index < pDB->iCapacity; ++index ) */
	} /* if ( strCallID_X ) */

	/* mark by tyhuang 2007/11/6
	if ( strCallID_X ) {
		strCallID_X = NULL;
	}
	*/

	if ( bIsCancel && bMatchCancel) {
		return pCancelCandidate;
	}

	return pResultTx;
}

TxStruct FindMatchingTransactionReq ( PCTransactionDB_C pDB, const SipReq pMsg, BOOL bMatchCancel) {
	assert ( pDB );
	assert ( pMsg );

	if ( !( pDB && pMsg ) ) {
		return NULL;
	}

	return FindMatchingTransaction ( pDB, pMsg, TRUE, bMatchCancel);
}

TxStruct FindMatchingTransactionRsp ( PCTransactionDB_C pDB, const SipRsp pMsg) {
	assert ( pDB );
	assert ( pMsg );

	if ( !( pDB && pMsg ) ) {
		return NULL;
	}

	return FindMatchingTransaction ( pDB, pMsg, FALSE, FALSE);
}

TxStruct FindMatchingAck( PCTransactionDB_C pDB, const SipRsp pMsg ) {
	assert ( pDB );
	assert ( pMsg );

	if ( pMsg && pDB ) {
		unsigned char * strCallID = sipRspGetHdr ( pMsg, Call_ID );		
		if ( strCallID ) {
			TxStruct pTx = NULL;

			/*
			int i = 0;
			
			for ( i = 0; i < pDB->iCapacity; ++i ) {
				TTransactionDbRecord * const pRecord = pDB->pRecords + i;
				if ( pRecord ) {
					if ( pRecord->strKey ) {
						if ( 0 == strcmp ( pRecord->strKey , strCallID ) ) {
							if ( pRecord->pTx->txtype == TX_CLIENT_ACK )
								pTx = pRecord->pTx;
						}
					}
				}
			}
			*/
			strCallID = NULL;
			return pTx;
		}
	}
	return NULL;
}

