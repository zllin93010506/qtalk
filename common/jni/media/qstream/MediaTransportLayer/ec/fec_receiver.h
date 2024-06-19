#ifndef FEC_RECEIVER_H
#define FEC_RECEIVER_H

#include "rtp_packet.h"
#include "sys/time.h"
#include <stdbool.h>

#define FEC_RECEIVER_SUCCESS 0
#define FEC_RECEIVER_FAIL -1

#define RTP_VERSION 2

typedef struct _fec_receiver_t {
	// member function
	int (*rtp_in)(struct _fec_receiver_t *thiz, rtp_packet_t *p);
	int (*fec_in)(struct _fec_receiver_t *thiz, rtp_packet_t *p);
	rtp_packet_t *(*get_rtp)(struct _fec_receiver_t *thiz, unsigned int sequence_number);
	
	bool (*is_time_out)(struct _fec_receiver_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
	bool (*is_request_re_transmit)(struct _fec_receiver_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);

	char priv[0];
} fec_receiver_t;


// class function
fec_receiver_t *fec_receiver_create();
void fec_receiver_destroy(fec_receiver_t *fec);



#endif
