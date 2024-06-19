#include "pointers.h"


TxMsgQue * InitTxMsgQue()
{
	TxMsgQue *mq = ( TxMsgQue *) calloc( 1, sizeof(TxMsgQue) );
	mq->tx_mutex = ( pthread_mutex_t *) calloc( 1, sizeof(pthread_mutex_t) );
	mq->tx_cond = ( pthread_cond_t *) calloc( 1, sizeof(pthread_cond_t) );
	pthread_mutex_init( mq->tx_mutex, NULL);
	mq->head = NULL;
	mq->tail = NULL;
	mq->msgcnt = 0;
	if( !mq->tx_mutex )
		return NULL;
	else
		return mq;
}

void DestoryTxQue( TxMsgQue * mq)
{
	if( !mq )
		return;

    pthread_mutex_lock ( mq->tx_mutex ); // mutex lock
	
	while( (mq->head) != NULL)
	{
		TxMsg *tmp = mq->head;
		mq->head = mq->head->nextTxMessage;

		if( (mq->head) == NULL )
		{
			mq->tail = NULL;
		}
		free(tmp);
	}

    pthread_mutex_unlock ( mq->tx_mutex ); // mutex unlock

	pthread_mutex_destroy( mq->tx_mutex );
	free( mq->tx_cond );
	free( mq->tx_mutex );
	free( mq );
}

int RecvTxMsg( TxMsgQue * mq, TxMsg *txmsg)
{
	if( !mq )
		return;

    TxMsg * tmp;// = mq->viewhead;
    do {
        pthread_mutex_lock ( mq->tx_mutex );
        if( (mq->head) == NULL)
        {
            pthread_cond_wait( mq->tx_cond, mq->tx_mutex );
            pthread_mutex_unlock ( mq->tx_mutex );
            //continue;
        }
        else
        {
			/*
            tmp = mq->head;
            mq->head = mq->head->nextTxMessage;
            memcpy( txmsg, tmp, sizeof(TxMsg));
            if( !(mq->head) )
            {
                mq->tail = NULL;
			}
            free(tmp);
            break;
            */
            
            tmp = mq->head;
            mq->head = mq->head->nextTxMessage;
            memcpy( txmsg, tmp, sizeof(TxMsg));
            mq->msgcnt--;
            if( (mq->head) == NULL )
            {
                mq->tail = NULL;
                free(tmp);
				pthread_mutex_unlock ( mq->tx_mutex );
				break;
            }
            else
            {
				free(tmp);
				pthread_mutex_unlock ( mq->tx_mutex );
			}
        }
    } while(1);
    pthread_mutex_unlock ( mq->tx_mutex );
   	return 0;
}

void SendTxMsg( TxMsgQue * mq, TxMsg txmsg)
{
    if( !mq )
		return;

    TxMsg *msg = (TxMsg *) calloc(1, sizeof(TxMsg));
    memcpy( (void*)msg, (const void*)&txmsg, sizeof(TxMsg) );
    msg->nextTxMessage = NULL;
    pthread_mutex_lock ( mq->tx_mutex );
    if( (mq->tail) == NULL)
    {
		mq->head = msg;
	}
    else
    {
		mq->tail->nextTxMessage = msg;
	}

    mq->tail = msg;
    mq->msgcnt++;
    pthread_mutex_unlock ( mq->tx_mutex ); // mutex unlock
    pthread_cond_signal(mq->tx_cond);
}
