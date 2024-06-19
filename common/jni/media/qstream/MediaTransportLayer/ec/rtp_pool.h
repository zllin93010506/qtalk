#ifndef RTP_POOL_H
#define RTP_POOL_H

#include  "rtp_packet.h"
#include <stdbool.h>

#define RTP_POOL_SUCCESS 0
#define RTP_POOL_FAIL -1

typedef struct _rtp_pool_t {
	int (*insert_rtp)(struct _rtp_pool_t *thiz, rtp_packet_t *p);
	rtp_packet_t *(*remove_rtp)(struct _rtp_pool_t *thiz, unsigned int sequence_number);
	rtp_packet_t *(*get_rtp)(struct _rtp_pool_t *thiz, unsigned int sequence_number);
	bool (*check_rtp)(struct _rtp_pool_t *thiz, unsigned int sequence_number);

	char priv[0];
} rtp_pool_t;


rtp_pool_t *rtp_pool_create(unsigned int size);
void rtp_pool_destroy(rtp_pool_t* pool);

#endif 
