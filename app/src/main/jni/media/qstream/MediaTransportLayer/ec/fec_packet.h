#ifndef FEC_PACKET_H
#define FEC_PACKET_H

#define FEC_PACKET_SUCCESS 0
#define FEC_PACKET_FAIL -1

#include "rtp_packet.h"

typedef struct _fec_packet_t {
	int (*rtp_in)(struct _fec_packet_t *thiz, rtp_packet_t *rtp_packet);
	unsigned char *(*to_net_stream)(struct _fec_packet_t *thiz, unsigned int *size);
	unsigned short (*get_sequence_number_base)(struct _fec_packet_t *thiz);
	unsigned short (*get_mask)(struct _fec_packet_t *thiz);
	unsigned int (*get_mask_cont)(struct _fec_packet_t *thiz);

	unsigned short (*get_p_recovery)(struct _fec_packet_t *thiz);
	unsigned short (*get_x_recovery)(struct _fec_packet_t *thiz);
	unsigned short (*get_cc_recovery)(struct _fec_packet_t *thiz);
	unsigned short (*get_pt_recovery)(struct _fec_packet_t *thiz);
	unsigned short (*get_m_recovery)(struct _fec_packet_t *thiz);
	unsigned int (*get_timestamp_recovery)(struct _fec_packet_t *thiz);
	unsigned short (*get_length_recovery)(struct _fec_packet_t *thiz);
	unsigned char *(*get_payload_recovery)(struct _fec_packet_t *thiz);

	char priv[0];		
} fec_packet_t;

fec_packet_t *fec_packet_create();
fec_packet_t *fec_packet_create_with_rtp_packet(rtp_packet_t *rtp_packet);
fec_packet_t *fec_packet_create_with_net_stream(unsigned char *stream, unsigned int size);
void fec_packet_destroy(fec_packet_t *fec_packet);
void fec_packet_print_packet(fec_packet_t *p, unsigned int size);
void fec_packet_print_net_stream(unsigned char *fec_packet_net_stream, unsigned int size);



#endif
