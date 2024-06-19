#include "parity.h"
#include "fec_packet.h"
#include "fec_sender.h"
#include "ec.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// priv 

typedef struct _fec_sender_priv_t {
	// call back
	int (*rtp_out)(rtp_packet_t *p, void *context, unsigned int context_size);

	double fec_bandwidth_ratio_upper_bound;
	double target_packet_loss_rate;
	double current_packet_loss_rate;
	
	
	unsigned int involved_rtp_count; //FIXME
	unsigned int cur_rtp_count;
	unsigned int last_sequence_number_base;
	fec_packet_t *cur_fec_packet;

	unsigned int last_re_transmit_request_seq;

	int mode;

	// context
	void *context;
	unsigned int context_size;
	
} fec_sender_priv_t;


// member function

static int rtp_in(fec_sender_t *thiz, rtp_packet_t *p);
static int _rtp_in(fec_sender_t *thiz, rtp_packet_t *p);
static int register_rtp_out(fec_sender_t *thiz, int(*rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);
static int _register_rtp_out(fec_sender_t *thiz, int(*rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);
static bool is_re_transmit(fec_sender_t *thiz, unsigned int sequence_number);
static bool _is_re_transmit(fec_sender_t *thiz, unsigned int sequence_number);

static int set_fec_bandwidth_ratio_upper_bound(struct _fec_sender_t *thiz, double ratio);
static int _set_fec_bandwidth_ratio_upper_bound(struct _fec_sender_t *thiz, double ratio);
static int set_target_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate);
static int _set_target_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate);
static int update_current_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate);
static int _update_current_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate);
static unsigned int get_involved_rtp_count(fec_sender_t *thiz);
static unsigned int _get_involved_rtp_count(fec_sender_t *thiz);
static double get_fec_bandwidth_ratio(fec_sender_t *thiz);
static double _get_fec_bandwidth_ratio(fec_sender_t *thiz);

// priv member function
static int update_involved_rtp_count(fec_sender_t *thiz);


// class function

fec_sender_t *fec_sender_create(int mode)
{
	fec_sender_t *fec = (fec_sender_t*)malloc(sizeof(fec_sender_t) + sizeof(fec_sender_priv_t));

	// private variables
	fec_sender_priv_t *priv = (fec_sender_priv_t*)fec->priv;
	priv->fec_bandwidth_ratio_upper_bound = DEFAULT_FEC_BANDWIDTH_RATIO_UPPER_BOUND;
	priv->target_packet_loss_rate = DEFAULT_TARGET_PACKET_LOSS_RATE;
	priv->current_packet_loss_rate = 0.0;
	priv->mode = mode;
	priv->last_sequence_number_base = 0;
	
	// member function
	fec->rtp_in = rtp_in;
	fec->register_rtp_out = register_rtp_out;
	fec->is_re_transmit = is_re_transmit;
	fec->set_fec_bandwidth_ratio_upper_bound = set_fec_bandwidth_ratio_upper_bound;
	fec->set_target_packet_loss_rate = set_target_packet_loss_rate;
	fec->update_current_packet_loss_rate = update_current_packet_loss_rate;
	fec->get_involved_rtp_count = get_involved_rtp_count;
	fec->get_fec_bandwidth_ratio = get_fec_bandwidth_ratio;

	

	// callback
	priv->rtp_out = NULL;
	priv->context = NULL;
	priv->context_size = 0;


	priv->last_sequence_number_base = 0;
	priv->involved_rtp_count = 5; //FIXME
	priv->cur_rtp_count = 0;
	priv->cur_fec_packet = NULL;


	return fec;
}


int fec_sender_destroy(fec_sender_t *fec)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)fec->priv;

	if (priv->context)
		free(priv->context);
	if (priv->cur_fec_packet) 
		fec_packet_destroy(priv->cur_fec_packet);

	free(fec);

	return FEC_SENDER_SUCCESS;
}


// implement member function

static int rtp_in(fec_sender_t *thiz, rtp_packet_t *p)
{
#ifdef FEC_DEBUG
	//printf("receive rtp with seq %d\n", p->sequence_number);
	printf("<FEC> %s: fec_sender receive rtp with seq %d\n", __func__, p->sequence_number);
#endif

	return _rtp_in(thiz, p);
}


static int _rtp_in(fec_sender_t *thiz, rtp_packet_t *p)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;


	// if first packet, create a new fec, else, add into the existed fec
	if (priv->cur_rtp_count == 0)
		priv->cur_fec_packet = fec_packet_create_with_rtp_packet(p);
	else 
		priv->cur_fec_packet->rtp_in(priv->cur_fec_packet, p);	
	
	// destroy the rtp packet
	rtp_packet_destroy(p);


	// increase the counter
	priv->cur_rtp_count++;


	// check whether we need to send it or not
	if (priv->cur_rtp_count >= priv->involved_rtp_count) {
		// update last_sequence_number_base
		priv->last_sequence_number_base = priv->cur_fec_packet->get_sequence_number_base(priv->cur_fec_packet);

		//FIXME
		// create a new rtp/
		unsigned int payload_size;

#ifdef FEC_DEBUG
		printf("<FEC> %s: fec_sender send fec with sn_base %d\n", __func__, priv->cur_fec_packet->get_sequence_number_base(priv->cur_fec_packet));
#endif

		unsigned char *payload = priv->cur_fec_packet->to_net_stream(priv->cur_fec_packet, &payload_size);

#ifdef FEC_DEBUG
		fec_packet_t *tmp = fec_packet_create_with_net_stream(payload, payload_size);
		printf("<FEC> %s: fec_sender send fec with sn_base %d\n", __func__, tmp->get_sequence_number_base(tmp));
		fec_packet_destroy(tmp);
#endif

	//	printf("payload_size ==== %u\n", payload_size);
	//	fec_packet_print_packet(priv->cur_fec_packet, payload_size);
	//	fec_packet_print_net_stream(payload, payload_size);
		
		rtp_packet_t *tmp_p = rtp_packet_create(0, 0, 0, 0, 0, 0, 0, 0, 0, payload, payload_size);
		free(payload);
        
		// call back
		priv->rtp_out(tmp_p, priv->context, priv->context_size);

		// destroy the current fec
		fec_packet_destroy(priv->cur_fec_packet);
		priv->cur_fec_packet = NULL;


		// reset cur_rtp_count
		priv->cur_rtp_count = 0;
		
	}
	
	return FEC_SENDER_SUCCESS;
}



static int register_rtp_out(fec_sender_t *thiz, int(*rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size)
{
	return _register_rtp_out(thiz, rtp_out, context, context_size);
}



static int _register_rtp_out(fec_sender_t *thiz, int(*rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;
	
	priv->rtp_out = rtp_out;

	// context
	priv->context = (void *)malloc(context_size);
	memcpy(priv->context, context, context_size);
	priv->context_size = context_size;

	return FEC_SENDER_SUCCESS;
}


static bool is_re_transmit(fec_sender_t *thiz, unsigned int sequence_number)
{
	return _is_re_transmit(thiz, sequence_number);
}

static bool _is_re_transmit(fec_sender_t *thiz, unsigned int sequence_number)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	//if (priv->last_sequence_number_base + priv->involved_rtp_count >= sequence_number)
	//	return false;

//	if (priv->last_sequence_number_base + 2 * priv->involved_rtp_count < sequence_number)
//		return false;
	if (priv->mode == DISABLE_FEC) 
		return true;

	//printf("%u <= %u\n", sequence_number, priv->last_re_transmit_request_seq);
		
	if (sequence_number - priv->last_re_transmit_request_seq < priv->involved_rtp_count) {
		priv->last_re_transmit_request_seq = sequence_number;
		return true;
	}
	
	priv->last_re_transmit_request_seq = sequence_number;
	return false;

}


static int set_fec_bandwidth_ratio_upper_bound(struct _fec_sender_t *thiz, double ratio)
{
	return _set_fec_bandwidth_ratio_upper_bound(thiz, ratio);
}

static int _set_fec_bandwidth_ratio_upper_bound(struct _fec_sender_t *thiz, double ratio)
{

	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	priv->fec_bandwidth_ratio_upper_bound = ratio;

	update_involved_rtp_count(thiz);

	return FEC_SENDER_SUCCESS;
}


static int set_target_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate)
{
	return _set_target_packet_loss_rate(thiz, packet_loss_rate);
}

static int _set_target_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	priv->target_packet_loss_rate = packet_loss_rate;

	update_involved_rtp_count(thiz);

	return FEC_SENDER_SUCCESS;
}


static int update_current_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate)
{
	return _update_current_packet_loss_rate(thiz, packet_loss_rate);
}

static int _update_current_packet_loss_rate(struct _fec_sender_t *thiz, double packet_loss_rate)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	priv->current_packet_loss_rate = packet_loss_rate;

	update_involved_rtp_count(thiz);

	//printf("priv->involved_rtp_count = %u\n", priv->involved_rtp_count);

	return FEC_SENDER_SUCCESS;
}


static unsigned int get_involved_rtp_count(fec_sender_t *thiz)
{
	return _get_involved_rtp_count(thiz);
}

static unsigned int _get_involved_rtp_count(fec_sender_t *thiz)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	return priv->involved_rtp_count;
}


static double get_fec_bandwidth_ratio(fec_sender_t *thiz)
{
	return _get_fec_bandwidth_ratio(thiz);
}

static double _get_fec_bandwidth_ratio(fec_sender_t *thiz)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	return 1.0/(priv->involved_rtp_count+1);
}


static int update_involved_rtp_count(fec_sender_t *thiz)
{
	fec_sender_priv_t *priv = (fec_sender_priv_t*)thiz->priv;

	double ratio = priv->fec_bandwidth_ratio_upper_bound;
	double target_p = priv->target_packet_loss_rate;
	double current_p = priv->current_packet_loss_rate;

	if (current_p < 0.0001) {
		//priv->involved_rtp_count = INVOLVED_RTP_COUNT_MAX;
        priv->involved_rtp_count = INVOLVED_RTP_COUNT_MIN;
		return FEC_SENDER_SUCCESS;
	}

	if (ratio > 0.5) ratio = 0.5;
	if (ratio <= 0.00001) ratio = 0.0001;

	unsigned int x1 = INVOLVED_RTP_COUNT_MIN;
	if (ratio > 0.00001 && current_p > 0.00001 && current_p < 1.0) {
		// x = log_(1-current_p) (1-target_p/current_p)    lower gauss
		unsigned int tmp = (unsigned int) (log(1-target_p/current_p) / log(1-current_p));

		//printf("x1 = %u\n", tmp);

		x1 = tmp > x1 ? tmp : x1;
		x1 = x1 > INVOLVED_RTP_COUNT_MAX ? INVOLVED_RTP_COUNT_MAX : x1;
	}

	unsigned int x2 = INVOLVED_RTP_COUNT_MIN;
	if (ratio > 0.00001) {
		// x = (1-ratio) / fatio    upper gauss
		double tmp = (1-ratio) / ratio;
		unsigned int tmp_x = tmp - (unsigned int)tmp > 0.00001 ? (unsigned int)tmp + 1 : (unsigned int)tmp;

		//printf("x2 = %u\n", tmp_x);

		x2 = tmp_x > x2 ? tmp_x : x2;
		x2 = x2 > INVOLVED_RTP_COUNT_MAX ? INVOLVED_RTP_COUNT_MAX : x2;
	}

	priv->involved_rtp_count = x1 > x2 ? x1 : x2;

	//printf("priv->involved_rtp_count = %u\n", priv->involved_rtp_count);

	return FEC_SENDER_SUCCESS;	
}
