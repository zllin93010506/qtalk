#ifndef FEC_SENDER_H
#define FEC_SENDER_H

#include "rtp_packet.h"
#include <stdbool.h>

#define FEC_SENDER_SUCCESS 0
#define FEC_SENDER_FAIL -1

#define DEFAULT_FEC_BANDWIDTH_RATIO_UPPER_BOUND 0.2
#define DEFAULT_TARGET_PACKET_LOSS_RATE 0.001

#define INVOLVED_RTP_COUNT_MIN 5
#define INVOLVED_RTP_COUNT_MAX 15

typedef struct _fec_sender_t {
	// member function
	int (*rtp_in)(struct _fec_sender_t *thiz, rtp_packet_t *p);
	int (*register_rtp_out)(struct _fec_sender_t *thiz, int(*rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);

	bool (*is_re_transmit)(struct _fec_sender_t *thiz, unsigned int sequence_number);

	int (*set_fec_bandwidth_ratio_upper_bound)(struct _fec_sender_t *thiz, double ratio);
	int (*set_target_packet_loss_rate)(struct _fec_sender_t *thiz, double packet_loss_rate);

	int (*update_current_packet_loss_rate)(struct _fec_sender_t *thiz, double packet_loss_rate);
	unsigned int (*get_involved_rtp_count)(struct _fec_sender_t *thiz);
	double (*get_fec_bandwidth_ratio)(struct _fec_sender_t *thiz);

	char priv[0];
} fec_sender_t;


// class function
fec_sender_t *fec_sender_create(int mode);
int fec_sender_destroy(fec_sender_t *fec);



#endif
