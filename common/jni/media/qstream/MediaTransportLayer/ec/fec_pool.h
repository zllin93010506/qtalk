#ifndef FEC_POOL_H
#define FEC_POOL_H

#include  "fec_packet.h"
#include <stdbool.h>

#define FEC_POOL_SUCCESS 0
#define FEC_POOL_FAIL -1

typedef struct _fec_pool_t {
	int (*insert_fec)(struct _fec_pool_t *thiz, fec_packet_t *p);
	fec_packet_t *(*remove_fec)(struct _fec_pool_t *thiz, unsigned int sequence_number);
	fec_packet_t *(*get_fec)(struct _fec_pool_t *thiz, unsigned int sequence_number);
	bool (*check_fec)(struct _fec_pool_t *thiz, unsigned int sequence_number);

	char priv[0];
} fec_pool_t;


fec_pool_t *fec_pool_create(unsigned int size);
void fec_pool_destroy(fec_pool_t* pool);

#endif 
