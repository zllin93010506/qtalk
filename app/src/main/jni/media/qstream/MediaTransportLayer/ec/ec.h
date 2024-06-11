#ifndef EC_H
#define EC_H

#include <stdbool.h>
#include <sys/time.h>
#include "rtp_packet.h"

#define EC_SUCCESS 0
#define EC_FAIL -1

#define ENABLE_FEC 1
#define DISABLE_FEC 0

//#define EC_DEBUG

typedef struct _ec_t {
	// member function
	int (*sender_rtp_in)(struct _ec_t *thiz, rtp_packet_t *p);
	int (*sender_register_rtp_out)(struct _ec_t *thiz, int(*rtp_out)(rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);

	bool (*sender_is_re_transmit)(struct _ec_t *thiz, unsigned int sequence_number);

	int (*sender_set_fec_bandwidth_ratio_upper_bound)(struct _ec_t *thiz, double ratio);
	int (*sender_set_target_packet_loss_rate)(struct _ec_t *thiz, double packet_loss_rate);
	int (*sender_update_current_packet_loss_rate)(struct _ec_t *thiz, double packet_loss_rate);
	
	unsigned int (*sender_get_involved_rtp_count)(struct _ec_t *thiz);
	double (*sender_get_fec_bandwidth_ratio)(struct _ec_t *thiz);


	int (*receiver_rtp_in)(struct _ec_t *thiz, rtp_packet_t *p);
	int (*receiver_fec_in)(struct _ec_t *thiz, rtp_packet_t *p);
	rtp_packet_t *(*receiver_get_rtp)(struct _ec_t *thiz, unsigned int sequence_number);

	bool (*receiver_is_time_out)(struct _ec_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
	bool (*receiver_is_request_re_transmit)(struct _ec_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);

	char priv[0];
} ec_t;


// class function
ec_t *ec_create(int fec_mode);
int ec_destroy(ec_t *ec);






#endif
