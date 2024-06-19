#ifndef FEC_RTP_PACKET_H
#define FEC_RTP_PACKET_H

typedef struct _rtp_packet_t {
	
	unsigned int version;
	unsigned int padding;
	unsigned int extension;
	unsigned int csrc_count;
	unsigned int marker;
	unsigned int payload_type;
	unsigned int sequence_number;
	
	unsigned int timestamp;

	unsigned int ssrc;

	unsigned char *payload;
	unsigned int payload_size;

	void (*print)(struct _rtp_packet_t *thiz, void (*print_payload)(unsigned char *payload, unsigned int payload_size));

} rtp_packet_t;


rtp_packet_t *rtp_packet_create(unsigned int version,
								unsigned int padding,
								unsigned int extension,
								unsigned int csrc_count,
								unsigned int marker,
								unsigned int payload_type,
								unsigned int sequence_number,
								unsigned int timestamp,
								unsigned int ssrc,
								unsigned char *payload,
								unsigned int payload_size);

void rtp_packet_destroy(rtp_packet_t *p);


#endif
