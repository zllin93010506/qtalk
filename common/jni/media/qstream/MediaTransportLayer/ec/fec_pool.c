#include "fec_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


// priv var
typedef struct {
	fec_packet_t **packets;
	unsigned size;
} fec_pool_priv_t;



// member function
static int insert_fec(struct _fec_pool_t *thiz, fec_packet_t *p);
static int _insert_fec(struct _fec_pool_t *thiz, fec_packet_t *p);
static fec_packet_t *remove_fec(struct _fec_pool_t *thiz, unsigned int sequence_number);
static fec_packet_t *_remove_fec(struct _fec_pool_t *thiz, unsigned int sequence_number);
static fec_packet_t *get_fec(struct _fec_pool_t *thiz, unsigned int sequence_number);
static fec_packet_t *_get_fec(struct _fec_pool_t *thiz, unsigned int sequence_number);

static unsigned int get_index(struct _fec_pool_t *thiz, unsigned int sequence_number);
static bool check_fec(struct _fec_pool_t *thiz, unsigned int sequence_number);
static int compute_sequence_number_base(struct _fec_pool_t *thiz, unsigned int sequence_number, unsigned int *sequence_number_base);


// class function
fec_pool_t *fec_pool_create(unsigned int size)
{
	fec_pool_t *pool = (fec_pool_t*)malloc(sizeof(fec_pool_t)+sizeof(fec_pool_priv_t));
	
	fec_pool_priv_t *priv = (fec_pool_priv_t *)pool->priv;
	priv->packets = (fec_packet_t**)malloc(sizeof(fec_packet_t *) * size);
	priv->size = size;
	int i=0;
	for (i=0; i<priv->size; i++) priv->packets[i] = NULL;

	pool->insert_fec = insert_fec;
	pool->remove_fec = remove_fec;
	pool->get_fec = get_fec;
	pool->check_fec = check_fec;

	return pool;
}

void fec_pool_destroy(fec_pool_t *pool)
{
	fec_pool_priv_t *priv = (fec_pool_priv_t *)pool->priv;
	fec_packet_t **packets = priv->packets;
	unsigned int size = priv->size;

	unsigned int i=0;
	for (i=0; i<size; i++) {
		if (packets[i] != NULL) {
			fec_packet_destroy(packets[i]);
			packets[i] = NULL;
		}
	}
	free(priv->packets);
	free(pool);
}


// member function
static int insert_fec(struct _fec_pool_t *thiz, fec_packet_t *p)
{
	return _insert_fec(thiz, p);	
}


static int _insert_fec(struct _fec_pool_t *thiz, fec_packet_t *p)
{
	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	fec_packet_t **packets = priv->packets;
	unsigned int sequence_number = p->get_sequence_number_base(p);
	unsigned index = get_index(thiz, sequence_number);

	if (packets[index] != NULL) 
		fec_packet_destroy(packets[index]);

	packets[index] = p;
	
	return FEC_POOL_SUCCESS;	
}

static fec_packet_t *remove_fec(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	return _remove_fec(thiz, sequence_number);	
}


static fec_packet_t *_remove_fec(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	if (!check_fec(thiz, sequence_number)) return NULL;

	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	fec_packet_t **packets = priv->packets;
	
	unsigned int sequence_number_base = 0;
	int ret = compute_sequence_number_base(thiz, sequence_number, &sequence_number_base);
	if (ret == FEC_PACKET_FAIL)
		return NULL;

	unsigned index = get_index(thiz, sequence_number_base);

	fec_packet_t *p = packets[index];
	packets[index] = NULL;
	
	return p;
}


static fec_packet_t *get_fec(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	return _get_fec(thiz, sequence_number);
}

static fec_packet_t *_get_fec(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	if (!check_fec(thiz, sequence_number)) return NULL;

	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	fec_packet_t **packets = priv->packets;

	unsigned int sequence_number_base = 0;
	int ret = compute_sequence_number_base(thiz, sequence_number, &sequence_number_base);
	if (ret == FEC_PACKET_FAIL)
		return NULL;

	unsigned int index = get_index(thiz, sequence_number_base);

	return packets[index];
}


// priv member
static unsigned int get_index(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	unsigned int size = priv->size;

	return sequence_number % size;
}


static bool check_fec(struct _fec_pool_t *thiz, unsigned int sequence_number)
{
	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	fec_packet_t **packets = priv->packets;

	unsigned int sequence_number_base = 0;
	int ret = compute_sequence_number_base(thiz, sequence_number, &sequence_number_base);
	if (ret == FEC_PACKET_FAIL) {
		return false;
	}

	unsigned int index = get_index(thiz, sequence_number_base);	


	if (packets[index] == NULL) {
		return false;
	}
	else if (packets[index]->get_sequence_number_base(packets[index]) != sequence_number_base) {
		return false;
	}
	else {
		return true;
	}
}


static int compute_sequence_number_base(struct _fec_pool_t *thiz, unsigned int sequence_number, unsigned int *sequence_number_base)
{
	fec_pool_priv_t *priv = (fec_pool_priv_t *)thiz->priv;
	fec_packet_t **packets = priv->packets;

	int i=0;
	for (i=0; i<priv->size; i++) {
		if (packets[i] == NULL) continue;	
		//printf("packets[%d]'s address = %p\n", i, packets[i]);
		*sequence_number_base = packets[i]->get_sequence_number_base(packets[i]);
		unsigned short mask = packets[i]->get_mask(packets[i]);
		unsigned short mask_cont = packets[i]->get_mask_cont(packets[i]);

		unsigned int diff = sequence_number - *sequence_number_base;
		if (diff == 0)
			return FEC_POOL_SUCCESS;
		else if (1 <= diff && diff <= 16) {
			if (mask & (1<<(diff-1)))
				return FEC_POOL_SUCCESS;
		}
		else if (17 <= diff && diff <= 32) {
			if (mask_cont & (1<<(diff-17)))
				return FEC_POOL_SUCCESS;
		}
	}
	

	return FEC_PACKET_FAIL;
}
