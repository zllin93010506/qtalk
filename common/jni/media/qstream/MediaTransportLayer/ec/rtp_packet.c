#include "rtp_packet.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// member function
void print(struct _rtp_packet_t *thiz, void (*print_payload)(unsigned char *payload, unsigned int payload_size));
void _print(struct _rtp_packet_t *thiz, void (*print_payload)(unsigned char *payload, unsigned int payload_size));

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
								unsigned int payload_size)
{
	rtp_packet_t *p = (rtp_packet_t*)malloc(sizeof(rtp_packet_t));


	p->version = version;
	p->padding = padding;
	p->extension = extension;
	p->csrc_count = csrc_count;
	p->marker = marker;
	p->payload_type = payload_type;
	p->sequence_number = sequence_number;
	p->timestamp = timestamp;
	p->ssrc = ssrc;
	p->payload = (unsigned char *)malloc(payload_size);
	memcpy(p->payload, payload, payload_size);
	p->payload_size = payload_size;

	p->print = print;

	return p;
}

void rtp_packet_destroy(rtp_packet_t *p)
{
	if (p == NULL)
		printf("<FEC> Error, in function %s, in line %d\n", __func__, __LINE__);

	if (p->payload)
		free(p->payload);

	free(p);
}


void print(struct _rtp_packet_t *thiz, void (*print_payload)(unsigned char *payload, unsigned int payload_size))
{
	_print(thiz, print_payload);
}


void _print(struct _rtp_packet_t *thiz, void (*print_payload)(unsigned char *payload, unsigned int payload_size))
{

	printf("version = %u\n", thiz->version);
	printf("padding = %u\n", thiz->padding);
	printf("extension = %u\n", thiz->extension);
	printf("csrc_count = %u\n", thiz->csrc_count);
	printf("marker = %u\n", thiz->marker);
	printf("payload_type = %u\n", thiz->payload_type);
	printf("sequence_number = %u\n", thiz->sequence_number);
	
	printf("timestamp = %u\n", thiz->timestamp);

	printf("ssrc = %u\n", thiz->ssrc);
	printf("payload_size = %u\n", thiz->payload_size);
	
	if (print_payload != NULL)
		print_payload(thiz->payload, thiz->payload_size);


}
