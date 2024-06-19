#include "ec.h"
#include "fec_sender.h"
#include "fec_receiver.h"
#include "rtp_packet.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

//pthread_mutex_t g_ec_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_fec_sender_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_fec_receiver_mutex = PTHREAD_MUTEX_INITIALIZER;


//#define EC_LOCK

#ifdef EC_LOCK

#define EC_SENDER_LOCK() pthread_mutex_lock(&g_fec_sender_mutex)
#define EC_SENDER_UNLOCK() pthread_mutex_unlock(&g_fec_sender_mutex)

#define EC_RECEIVER_LOCK() pthread_mutex_lock(&g_fec_receiver_mutex)
#define EC_RECEIVER_UNLOCK() pthread_mutex_unlock(&g_fec_receiver_mutex)

#else

#define EC_SENDER_LOCK()
#define EC_SENDER_UNLOCK()

#define EC_RECEIVER_LOCK()
#define EC_RECEIVER_UNLOCK()

#endif


// priv 

typedef struct _ec_priv_t {
	int fec_mode;

	// variables
	struct {
		int version;
		int ssrc;
	} sender_rtp_info;

	// call back
	int (*sender_rtp_out)(rtp_packet_t *p, void *context, unsigned int context_size);

	// object
	fec_sender_t *sender;
	fec_receiver_t *receiver;
	
} ec_priv_t;


// member function

static int _sender_rtp_in(ec_t *thiz, rtp_packet_t *p);
static int sender_rtp_in(ec_t *thiz, rtp_packet_t *p);

static int _sender_register_rtp_out(ec_t *thiz, int(*sender_rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);
static int sender_register_rtp_out(ec_t *thiz, int(*sender_rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size);

static bool sender_is_re_transmit(struct _ec_t *thiz, unsigned int sequence_number);
static bool _sender_is_re_transmit(struct _ec_t *thiz, unsigned int sequence_number);

static int sender_set_fec_bandwidth_ratio_upper_bound(struct _ec_t *thiz, double ratio);
static int _sender_set_fec_bandwidth_ratio_upper_bound(struct _ec_t *thiz, double ratio);
static int sender_set_target_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate);
static int _sender_set_target_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate);
static int sender_update_current_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate);
static int _sender_update_current_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate);

static unsigned int sender_get_involved_rtp_count(struct _ec_t *thiz);
static unsigned int _sender_get_involved_rtp_count(struct _ec_t *thiz);
static double sender_get_fec_bandwidth_ratio(struct _ec_t *thiz);
static double _sender_get_fec_bandwidth_ratio(struct _ec_t *thiz);


static int receiver_rtp_in(ec_t *thiz, rtp_packet_t *p);
static int _receiver_rtp_in(ec_t *thiz, rtp_packet_t *p);
static int receiver_fec_in(ec_t *thiz, rtp_packet_t *p);
static int _receiver_fec_in(ec_t *thiz, rtp_packet_t *p);

static rtp_packet_t *_receiver_get_rtp(struct _ec_t *thiz, unsigned int sequence_number);
static rtp_packet_t *receiver_get_rtp(struct _ec_t *thiz, unsigned int sequence_number);

static bool receiver_is_time_out(struct _ec_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
static bool _receiver_is_time_out(struct _ec_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
static bool receiver_is_request_re_transmit(struct _ec_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);
static bool _receiver_is_request_re_transmit(struct _ec_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);


// class function

ec_t *ec_create(int fec_mode)
{
	EC_SENDER_LOCK();
	EC_RECEIVER_LOCK();

	ec_t *ec = (ec_t*)malloc(sizeof(ec_t) + sizeof(ec_priv_t));

	// private variables
	ec_priv_t *priv = (ec_priv_t*)ec->priv;
	priv->fec_mode = fec_mode;
	
	// member function
	ec->sender_rtp_in = sender_rtp_in;
	ec->sender_register_rtp_out = sender_register_rtp_out;
	ec->sender_is_re_transmit = sender_is_re_transmit;
	ec->sender_set_fec_bandwidth_ratio_upper_bound = sender_set_fec_bandwidth_ratio_upper_bound;
	ec->sender_set_target_packet_loss_rate = sender_set_target_packet_loss_rate;
	ec->sender_update_current_packet_loss_rate = sender_update_current_packet_loss_rate;
	ec->sender_get_involved_rtp_count = sender_get_involved_rtp_count;
	ec->sender_get_fec_bandwidth_ratio = sender_get_fec_bandwidth_ratio;

	ec->receiver_rtp_in = receiver_rtp_in;
	ec->receiver_fec_in = receiver_fec_in;
	ec->receiver_get_rtp = receiver_get_rtp;
	ec->receiver_is_time_out = receiver_is_time_out;
	ec->receiver_is_request_re_transmit = receiver_is_request_re_transmit;
	

	// object
	priv->sender = fec_sender_create(fec_mode);
	priv->receiver = fec_receiver_create();

	EC_RECEIVER_UNLOCK();
	EC_SENDER_UNLOCK();

	return ec;
}


int ec_destroy(ec_t *ec)
{
	EC_SENDER_LOCK();
	EC_RECEIVER_LOCK();

	ec_priv_t *priv = (ec_priv_t*)ec->priv;

	fec_sender_destroy(priv->sender);
	fec_receiver_destroy(priv->receiver);

	free(ec);

	EC_RECEIVER_UNLOCK();
	EC_SENDER_UNLOCK();

	return EC_SUCCESS;
}


// implement member function



static int sender_rtp_in(ec_t *thiz, rtp_packet_t *p)
{
	EC_SENDER_LOCK();

	// do nothing if disable fec
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	if (DISABLE_FEC == priv->fec_mode) {
		rtp_packet_destroy(p);
		return EC_SUCCESS;
	}

	int ret =  _sender_rtp_in(thiz, p);

	EC_SENDER_UNLOCK();

	return ret;
}


static int _sender_rtp_in(ec_t *thiz, rtp_packet_t *p)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->rtp_in(sender, p);
}



static int sender_register_rtp_out(ec_t *thiz, int(*sender_rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size)
{
	EC_SENDER_LOCK();

	int ret = _sender_register_rtp_out(thiz, sender_rtp_out, context, context_size);

	EC_SENDER_UNLOCK();

	return ret;
}



static int _sender_register_rtp_out(ec_t *thiz, int(*sender_rtp_out)( rtp_packet_t *p, void* context, unsigned int context_size), void *context, unsigned int context_size)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;
	
	return sender->register_rtp_out(sender, sender_rtp_out, context, context_size);
}


static bool sender_is_re_transmit(struct _ec_t *thiz, unsigned int sequence_number)
{
	// return true is disable fec
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	if (DISABLE_FEC == priv->fec_mode) return true;

	return _sender_is_re_transmit(thiz, sequence_number);
}

static bool _sender_is_re_transmit(struct _ec_t *thiz, unsigned int sequence_number)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->is_re_transmit(sender, sequence_number);
}


static int sender_set_fec_bandwidth_ratio_upper_bound(struct _ec_t *thiz, double ratio)
{
	return _sender_set_fec_bandwidth_ratio_upper_bound(thiz, ratio);
}

static int _sender_set_fec_bandwidth_ratio_upper_bound(struct _ec_t *thiz, double ratio)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->set_fec_bandwidth_ratio_upper_bound(sender, ratio);
}


static int sender_set_target_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate)
{
	return _sender_set_target_packet_loss_rate(thiz, packet_loss_rate);
}

static int _sender_set_target_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->set_target_packet_loss_rate(sender, packet_loss_rate);
}


static int sender_update_current_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate)
{
	return _sender_update_current_packet_loss_rate(thiz, packet_loss_rate);
}

static int _sender_update_current_packet_loss_rate(struct _ec_t *thiz, double packet_loss_rate)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->update_current_packet_loss_rate(sender, packet_loss_rate);
}


static unsigned int sender_get_involved_rtp_count(struct _ec_t *thiz)
{
	return _sender_get_involved_rtp_count(thiz);
}

static unsigned int _sender_get_involved_rtp_count(struct _ec_t *thiz)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->get_involved_rtp_count(sender);
}

static double sender_get_fec_bandwidth_ratio(struct _ec_t *thiz)
{
	return _sender_get_fec_bandwidth_ratio(thiz);
}

static double _sender_get_fec_bandwidth_ratio(struct _ec_t *thiz)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_sender_t *sender = priv->sender;

	return sender->get_fec_bandwidth_ratio(sender);
}

static int receiver_rtp_in(struct _ec_t *thiz, rtp_packet_t *p)
{
	EC_RECEIVER_LOCK();

	// do nothing if disable fec
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	if (DISABLE_FEC == priv->fec_mode) {
		rtp_packet_destroy(p);
		return EC_SUCCESS;
	}


	int ret = _receiver_rtp_in(thiz, p);

	EC_RECEIVER_UNLOCK();

	return ret;
}

static int _receiver_rtp_in(struct _ec_t *thiz, rtp_packet_t *p)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_receiver_t *receiver = priv->receiver;

	return receiver->rtp_in(receiver, p);
}


static int receiver_fec_in(struct _ec_t *thiz, rtp_packet_t *p)
{
	EC_RECEIVER_LOCK();

	// do nothing if disable fec
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	if (DISABLE_FEC == priv->fec_mode) {
		rtp_packet_destroy(p);
		return EC_SUCCESS;
	}

	int ret = _receiver_fec_in(thiz, p);

	EC_RECEIVER_UNLOCK();

	return ret;
}

static int _receiver_fec_in(struct _ec_t *thiz, rtp_packet_t *p)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_receiver_t *receiver = priv->receiver;

	return receiver->fec_in(receiver, p);
}

static rtp_packet_t *receiver_get_rtp(struct _ec_t *thiz, unsigned int sequence_number)
{
	EC_RECEIVER_LOCK();

	rtp_packet_t *ret = _receiver_get_rtp(thiz, sequence_number);

#ifdef EC_DEBUG
	if (ret != NULL) {
		printf("<EC> recovery: ");
		ret->print(ret, parity_print_data);
	}
#endif

	EC_RECEIVER_UNLOCK();

	return ret;
}


static rtp_packet_t *_receiver_get_rtp(struct _ec_t *thiz, unsigned int sequence_number)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_receiver_t *receiver = priv->receiver;

	return receiver->get_rtp(receiver, sequence_number);
}


static bool receiver_is_time_out(struct _ec_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time)
{
	return _receiver_is_time_out(thiz, frame_count, packet_count, round_trip_time, playout_time);
}

static bool _receiver_is_time_out(struct _ec_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_receiver_t *receiver = priv->receiver;

	return receiver->is_time_out(receiver, frame_count, packet_count, round_trip_time, playout_time);
}


static bool receiver_is_request_re_transmit(struct _ec_t *thiz, unsigned int sequence_number, unsigned int round_trip_time)
{
	// return true if disable fec
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	if (DISABLE_FEC == priv->fec_mode) return true;

	return _receiver_is_request_re_transmit(thiz, sequence_number, round_trip_time);
}

static bool _receiver_is_request_re_transmit(struct _ec_t *thiz, unsigned int sequence_number, unsigned int round_trip_time)
{
	ec_priv_t *priv = (ec_priv_t*)thiz->priv;
	fec_receiver_t *receiver = priv->receiver;

	return receiver->is_request_re_transmit(receiver, sequence_number, round_trip_time);
}

