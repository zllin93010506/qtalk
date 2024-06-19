#include "rtp_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


// priv var
typedef struct {
	rtp_packet_t **packets;
	unsigned size;
} rtp_pool_priv_t;



// member function
static int insert_rtp(struct _rtp_pool_t *thiz, rtp_packet_t *p);
static int _insert_rtp(struct _rtp_pool_t *thiz, rtp_packet_t *p);
static rtp_packet_t *remove_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number);
static rtp_packet_t *_remove_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number);
static rtp_packet_t *get_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number);
static rtp_packet_t *_get_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number);

static unsigned int get_index(struct _rtp_pool_t *thiz, unsigned int sequence_number);
static bool check_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number);


// class function
rtp_pool_t *rtp_pool_create(unsigned int size)
{
	rtp_pool_t *pool = (rtp_pool_t*)malloc(sizeof(rtp_pool_t)+sizeof(rtp_pool_priv_t));
	
	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)pool->priv;
	priv->packets = (rtp_packet_t**)malloc(sizeof(rtp_packet_t *) * size);
	priv->size = size;
	int i=0;
	for (i=0; i<size; i++) 
		priv->packets[i] = NULL;

	pool->insert_rtp = insert_rtp;
	pool->remove_rtp = remove_rtp;
	pool->get_rtp = get_rtp;
	pool->check_rtp = check_rtp;

	return pool;
}

void rtp_pool_destroy(rtp_pool_t *pool)
{
	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)pool->priv;
	rtp_packet_t **packets = priv->packets;
	unsigned int size = priv->size;

	unsigned int i=0;
	for (i=0; i<size; i++) {
		if (packets[i] != NULL) {
			rtp_packet_destroy(packets[i]);
			packets[i] = NULL;
		}
	}
	free(priv->packets);
	free(pool);
}


// member function
static int insert_rtp(struct _rtp_pool_t *thiz, rtp_packet_t *p)
{
	return _insert_rtp(thiz, p);	
}


static int _insert_rtp(struct _rtp_pool_t *thiz, rtp_packet_t *p)
{
	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)thiz->priv;
	rtp_packet_t **packets = priv->packets;
	unsigned int sequence_number = p->sequence_number;
	unsigned index = get_index(thiz, sequence_number);

	if (packets[index] != NULL) {
		rtp_packet_destroy(packets[index]);
		packets[index] = NULL;
	}

	packets[index] = p;
	
	return RTP_POOL_SUCCESS;	
}

static rtp_packet_t *remove_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	return _remove_rtp(thiz, sequence_number);	
}


static rtp_packet_t *_remove_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	if (!check_rtp(thiz, sequence_number)) return NULL;

	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)thiz->priv;
	rtp_packet_t **packets = priv->packets;
	unsigned index = get_index(thiz, sequence_number);

	rtp_packet_t *ret = packets[index];
	packets[index] = NULL;
	
	return ret;
}


static rtp_packet_t *get_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	return _get_rtp(thiz, sequence_number);
}

static rtp_packet_t *_get_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	if (!check_rtp(thiz, sequence_number)) return NULL;

	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)thiz->priv;
	rtp_packet_t **packets = priv->packets;
	unsigned int index = get_index(thiz, sequence_number);

	return packets[index];
}


// priv member
static unsigned int get_index(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)thiz->priv;
	unsigned int size = priv->size;

	return sequence_number % size;
}


static bool check_rtp(struct _rtp_pool_t *thiz, unsigned int sequence_number)
{
	rtp_pool_priv_t *priv = (rtp_pool_priv_t *)thiz->priv;
	rtp_packet_t **packets = priv->packets;
	unsigned int index = get_index(thiz, sequence_number);	

	if (packets[index] == NULL)
		return false;
	else if (packets[index]->sequence_number != sequence_number)
		return false;
	else
		return true;
}

