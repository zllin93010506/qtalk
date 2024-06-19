#include "fec_packet.h"
#include "parity.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#if 0
typedef struct _fec_t {
	unsigned short cc_recovery:4;		// csrc_count_recovery;
	unsigned short x_recovery:1;		// extenstion_recovery;
	unsigned short p_recovery:1;		// padding_recovery;
	unsigned short l:1;					// long_mask; set as 1
	unsigned short e:1;					// extension; set as 0

	unsigned short pt_recovery:7;		// payload_type_recovery;
	unsigned short m_recovery:1;		// marker_bit_recovery;
	
	unsigned short sn_base;				// sequence_number_base;	
	unsigned int ts_recovery;			// timestamp_recovery;	
	unsigned short length_recovery;		// length_recovery;	


typedef struct _fec_level_t {
	unsigned short protection_length;
	unsigned short mask;
	unsigned int mask_cont;	//we set long_mask == 1
} fec_level_t;

} fec_t;

typedef struct _fec_packet_priv {
	fec_t 
	fec_level_t level_
	unsigned char level_payload[2000];
} fec_packet_priv_t;
#endif 

#define FEC_PACKET_MAXIMUM_PAYLOAD_SIZE 2000

typedef struct _fec_packet_priv {
	unsigned short cc_recovery:4;		// csrc_count_recovery;
	unsigned short x_recovery:1;		// extenstion_recovery;
	unsigned short p_recovery:1;		// padding_recovery;
	unsigned short l:1;					// long_mask; set as 1
	unsigned short e:1;					// extension; set as 0

	unsigned short pt_recovery:7;		// payload_type_recovery;
	unsigned short m_recovery:1;		// marker_bit_recovery;
	
	unsigned short sn_base;				// sequence_number_base;	
	unsigned int ts_recovery;			// timestamp_recovery;	
	unsigned short length_recovery;		// length_recovery;	


	unsigned short protection_length;
	unsigned short mask;
	unsigned int mask_cont;	//we set long_mask == 1

	unsigned char level_payload[FEC_PACKET_MAXIMUM_PAYLOAD_SIZE];

} fec_packet_priv_t;


// member function
static int rtp_in(fec_packet_t *thiz, rtp_packet_t *rtp_packet);
static int _rtp_in(fec_packet_t *thiz, rtp_packet_t *rtp_packet);
static unsigned char *to_net_stream(fec_packet_t *thiz, unsigned int *size);
static unsigned char *_to_net_stream(fec_packet_t *thiz, unsigned int *size);

static unsigned short get_p_recovery(fec_packet_t *thiz);
static unsigned short get_x_recovery(fec_packet_t *thiz);
static unsigned short get_cc_recovery(fec_packet_t *thiz);
static unsigned short get_pt_recovery(fec_packet_t *thiz);
static unsigned short get_m_recovery(fec_packet_t *thiz);
static unsigned int get_timestamp_recovery(fec_packet_t *thiz);
static unsigned short get_length_recovery(fec_packet_t *thiz);

static unsigned char *get_payload_recovery(fec_packet_t *thiz);

static unsigned short get_sequence_number_base(fec_packet_t *thiz);
static unsigned short get_mask(fec_packet_t *thiz);
static unsigned int get_mask_cont(fec_packet_t *thiz);

fec_packet_t *fec_packet_create()
{
	fec_packet_t *p = (fec_packet_t *)malloc(sizeof(fec_packet_t) + sizeof(fec_packet_priv_t));
	fec_packet_priv_t *priv = (fec_packet_priv_t *)p->priv;
	memset(priv, 0, sizeof(fec_packet_priv_t));
	
	// member functions
	p->rtp_in = rtp_in;
	p->to_net_stream = to_net_stream;
	p->get_sequence_number_base = get_sequence_number_base;
	p->get_mask = get_mask;
	p->get_mask_cont = get_mask_cont;

	p->get_p_recovery = get_p_recovery;
	p->get_x_recovery = get_x_recovery;
	p->get_cc_recovery = get_cc_recovery;
	p->get_pt_recovery = get_pt_recovery;
	p->get_m_recovery = get_m_recovery;
	p->get_timestamp_recovery = get_timestamp_recovery;
	p->get_length_recovery = get_length_recovery;
	
	p->get_payload_recovery = get_payload_recovery;

	return p;
}


fec_packet_t *fec_packet_create_with_net_stream(unsigned char *net_stream, unsigned int size)
{
	fec_packet_t *p = fec_packet_create();
	fec_packet_priv_t *priv = (fec_packet_priv_t *)p->priv;
	fec_packet_priv_t *stream = (fec_packet_priv_t*)net_stream;

	memcpy(priv, stream, size); 

//	priv->pt_recovery = ntohs(stream->pt_recovery)>>1;

//	priv->sn_base = ntohs(stream->sn_base);
//	priv->ts_recovery = ntohl(stream->ts_recovery);
//	priv->length_recovery = ntohs(stream->length_recovery);

//	priv->protection_length = ntohs(stream->protection_length);
//	priv->mask = ntohs(stream->mask);
//	priv->mask_cont = ntohl(stream->mask_cont);

	return p;
}


fec_packet_t *fec_packet_create_with_rtp_packet(rtp_packet_t *rtp_packet)
{
	if (rtp_packet->payload_size > FEC_PACKET_MAXIMUM_PAYLOAD_SIZE) {
		printf("<FEC> Error in function %s, line %d\n", __func__, __LINE__);
		exit(0);
	}

	fec_packet_t *p = fec_packet_create();
	fec_packet_priv_t *priv = (fec_packet_priv_t *)p->priv;


	// private attributes
	priv->e = 0;
	priv->l = 1;
	priv->p_recovery = rtp_packet->padding;
	priv->x_recovery = rtp_packet->extension;
	priv->cc_recovery = rtp_packet->csrc_count; 
	priv->m_recovery = rtp_packet->marker;
	priv->pt_recovery = rtp_packet->payload_type;
	priv->sn_base = rtp_packet->sequence_number;
	priv->ts_recovery = rtp_packet->timestamp;
	priv->length_recovery = rtp_packet->payload_size;

	priv->protection_length = rtp_packet->payload_size;
	priv->mask = 0;
	priv->mask_cont = 0;

	parity_xor_data(priv->level_payload, priv->protection_length, 
					priv->level_payload, priv->protection_length, 
					rtp_packet->payload, rtp_packet->payload_size);
		

	return p;
}


void fec_packet_destroy(fec_packet_t *fec_packet)
{
	free(fec_packet);
}


void _fec_packet_print(fec_packet_priv_t *priv)
{
	printf("[fec]\n");
	printf("extension: %u\n", priv->e);
	printf("long mask: %u\n", priv->l);
	printf("p_recovery: %u\n", priv->p_recovery);
	printf("x_recovery: %u\n", priv->x_recovery);
	printf("cc_recovery: %u\n", priv->cc_recovery);
	printf("m_recovery: %u\n", priv->m_recovery);
	printf("pt_recovery: %u\n", priv->pt_recovery);
	
	printf("sn_base: %u\n", priv->sn_base);
	printf("ts_recovery: %u\n", priv->ts_recovery);
	printf("length_recovery: %u\n", priv->length_recovery);

	printf("protect_length: %u\n", priv->protection_length);

	int i=0;
	printf("mask: ");
	for (i=0; i<16; i++) printf("%u", priv->mask&(1<<i)?1:0);
	printf("\n");
	printf("mask cont: ");
	for (i=0; i<32; i++) printf("%u", priv->mask_cont&(1<<i)?1:0);
	printf("\n");

	parity_print_data(priv->level_payload, priv->protection_length);
}


void fec_packet_print_packet(fec_packet_t *p, unsigned int size)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)p->priv;
	_fec_packet_print(priv);
}


void fec_packet_print_net_stream(unsigned char *fec_packet_net_stream, unsigned int size)
{
	//fec_packet_priv_t *priv = (fec_packet_priv_t *)fec_packet_net_stream;
	//_fec_packet_print(priv);
	fec_packet_t *tmp = fec_packet_create_with_net_stream(fec_packet_net_stream, size);
	fec_packet_print_packet(tmp, size);
	free(tmp);
}



// member function

int rtp_in(fec_packet_t *thiz, rtp_packet_t *rtp_packet)
{
	return _rtp_in(thiz, rtp_packet);
}


int _rtp_in(fec_packet_t *thiz, rtp_packet_t *rtp_packet)
{
	if (rtp_packet->payload_size > FEC_PACKET_MAXIMUM_PAYLOAD_SIZE) {
		printf("<FEC> Error in function %s, line %d\n", __func__, __LINE__);
		exit(0);
	}

	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;

	priv->p_recovery = parity_xor_unsigned_char(priv->p_recovery, rtp_packet->padding);
	priv->x_recovery = parity_xor_unsigned_char(priv->x_recovery, rtp_packet->extension);
	priv->cc_recovery = parity_xor_unsigned_char(priv->cc_recovery, rtp_packet->csrc_count);
	priv->m_recovery = parity_xor_unsigned_char(priv->m_recovery, rtp_packet->marker);
	priv->pt_recovery = parity_xor_unsigned_short(priv->pt_recovery, rtp_packet->payload_type);
	priv->ts_recovery = parity_xor_unsigned_int(priv->ts_recovery, rtp_packet->timestamp);
	priv->length_recovery = parity_xor_unsigned_short(priv->length_recovery, rtp_packet->payload_size);

	priv->protection_length = priv->protection_length > rtp_packet->payload_size ?
											priv->protection_length : rtp_packet->payload_size;
											
	//mask
	int dist = rtp_packet->sequence_number - priv->sn_base;
	if (dist <= 16)
		priv->mask += 1<<(dist-1);
	else if (dist <= 48)
		priv->mask_cont += 1<<(dist-1-16);


	parity_xor_data(priv->level_payload, priv->protection_length, 
					priv->level_payload, priv->protection_length, 
					rtp_packet->payload, rtp_packet->payload_size);

	return FEC_PACKET_SUCCESS;
}


static unsigned char *to_net_stream(fec_packet_t *thiz, unsigned int *size)
{
	return _to_net_stream(thiz, size);
}


static unsigned char *_to_net_stream(fec_packet_t *thiz, unsigned int *size)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;

	*size = sizeof(fec_packet_priv_t) - 2000 + priv->protection_length;
	fec_packet_priv_t *net_stream = (fec_packet_priv_t *)malloc(*size);
	memcpy(net_stream, priv, *size); 


//	net_stream->pt_recovery = htons(net_stream->pt_recovery)>>1;

//	net_stream->sn_base = htons(net_stream->sn_base);
//	net_stream->ts_recovery = htonl(net_stream->ts_recovery);
//	net_stream->length_recovery = htons(net_stream->length_recovery);

//	net_stream->protection_length = htons(net_stream->protection_length);
//	net_stream->mask = htons(net_stream->mask);
//	net_stream->mask_cont = htonl(net_stream->mask_cont);

	return (unsigned char *)net_stream;
}


static unsigned short get_sequence_number_base(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->sn_base;
}


static unsigned short get_mask(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->mask;
}


static unsigned int get_mask_cont(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->mask_cont;
}

static unsigned short get_p_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->p_recovery;
}

static unsigned short get_x_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->x_recovery;
}

static unsigned short get_cc_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->cc_recovery;
}

static unsigned short get_pt_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->pt_recovery;
}

static unsigned short get_m_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->m_recovery;
}

static unsigned int get_timestamp_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->ts_recovery;
}

static unsigned short get_length_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->length_recovery;
}

static unsigned char *get_payload_recovery(fec_packet_t *thiz)
{
	fec_packet_priv_t *priv = (fec_packet_priv_t *)thiz->priv;
	
	return priv->level_payload;
}
