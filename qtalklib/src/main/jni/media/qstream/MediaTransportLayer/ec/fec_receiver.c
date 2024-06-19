#include "../../GlobalDefine.h"
#include "fec_receiver.h"
#include "rtp_pool.h"
#include "fec_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

//#define RTP_POOL_SIZE   FEC_RECV_POOL_PACKET_CNT //500
//#define FEC_POOL_SIZE   FEC_RECV_POOL_PACKET_CNT //500
#define RTP_POOL_SIZE  500
#define FEC_POOL_SIZE  500


typedef struct _fec_receiver_priv_t {
	unsigned int version;
	unsigned int ssrc;
	rtp_pool_t *rtp_pool;
	fec_pool_t *fec_pool;

	unsigned int involved_rtp_count;
	unsigned int last_sequence_number_base;

	unsigned int average_rtp_interval; // usecond
	unsigned int last_rtp_sequence_number;
	struct timeval last_rtp_time;


} fec_receiver_priv_t;


// member function declaration
static int rtp_in(fec_receiver_t *thiz, rtp_packet_t *p);
static int _rtp_in(fec_receiver_t *thiz, rtp_packet_t *p);
static int fec_in(fec_receiver_t *thiz, rtp_packet_t *p);
static int _fec_in(fec_receiver_t *thiz, rtp_packet_t *p);
static rtp_packet_t *get_rtp(fec_receiver_t *thiz, unsigned int sequence_number);
static rtp_packet_t *_get_rtp(fec_receiver_t *thiz, unsigned int sequence_number);
static bool is_time_out(fec_receiver_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
static bool _is_time_out(fec_receiver_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);
static bool is_request_re_transmit(fec_receiver_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);
static bool _is_request_re_transmit(fec_receiver_t *thiz, unsigned int sequence_number, unsigned int round_trip_time);

// class function
fec_receiver_t *fec_receiver_create()
{
	fec_receiver_t *receiver = (fec_receiver_t *)malloc(sizeof(fec_receiver_t) + sizeof(fec_receiver_priv_t));
	fec_receiver_priv_t *priv = (fec_receiver_priv_t *)receiver->priv;

	priv->ssrc = 0;
	priv->version = RTP_VERSION;
	priv->involved_rtp_count = 0;
	priv->last_sequence_number_base = 0;
	priv->average_rtp_interval = 0;
	priv->last_rtp_sequence_number = 0;
	gettimeofday(&priv->last_rtp_time, NULL);

	priv->rtp_pool = rtp_pool_create(RTP_POOL_SIZE);
	priv->fec_pool = fec_pool_create(FEC_POOL_SIZE);

	
	receiver->rtp_in = rtp_in;
	receiver->fec_in = fec_in;
	receiver->get_rtp = get_rtp;
	receiver->is_time_out = is_time_out;
	receiver->is_request_re_transmit = is_request_re_transmit;


	return receiver;
}


void fec_receiver_destroy(fec_receiver_t *receiver)
{
	fec_receiver_priv_t *priv = (fec_receiver_priv_t *)receiver->priv;

	rtp_pool_destroy(priv->rtp_pool);
	fec_pool_destroy(priv->fec_pool);

	free(receiver);
}


// member function declaration
static int rtp_in(fec_receiver_t *thiz, rtp_packet_t *p)
{
	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;

	// update ssrc
	priv->ssrc = p->ssrc;

	// update rtp average interval if the rtps are continued
	unsigned int sequence_number = p->sequence_number;
	if (sequence_number - priv->last_rtp_sequence_number == 1) {
		struct timeval cur_time;
		gettimeofday(&cur_time, NULL);

		unsigned int interval = (cur_time.tv_sec - priv->last_rtp_time.tv_sec) * 1000000 +
								(cur_time.tv_usec - priv->last_rtp_time.tv_usec) ;

		if (interval <= 1000000) { // ignore the exception case
			priv->average_rtp_interval = priv->average_rtp_interval * 15 / 16 + interval / 16;
		}
		//printf("average rtp interval = %u\n", priv->average_rtp_interval);
	}

	// update last_rtp_sequence_number, last_rtp_time
	priv->last_rtp_sequence_number = sequence_number;
	gettimeofday(&priv->last_rtp_time, NULL);

	return _rtp_in(thiz, p);
}

static int _rtp_in(fec_receiver_t *thiz, rtp_packet_t *p)
{

	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;
	rtp_pool_t *pool = priv->rtp_pool;

	return pool->insert_rtp(pool, p);
}


static int fec_in(fec_receiver_t *thiz, rtp_packet_t *p)
{
	
	return _fec_in(thiz, p);
}


static int _fec_in(fec_receiver_t *thiz, rtp_packet_t *p)
{
	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;
	fec_pool_t *pool = priv->fec_pool;

	fec_packet_t *fec_packet = fec_packet_create_with_net_stream(p->payload, p->payload_size);
	rtp_packet_destroy(p);


	// update involved_rtp_count
	unsigned short mask = fec_packet->get_mask(fec_packet);
	unsigned int mask_cont = fec_packet->get_mask_cont(fec_packet);

	unsigned int involved_rtp_count = 1;
	int i=0;
	for (i=0; i<16; i++) if (mask & (1<<i)) involved_rtp_count = i+1+1;
	for (i=0; i<32; i++) if (mask_cont & (1<<i)) involved_rtp_count = i+1+1+16;
	priv->involved_rtp_count = involved_rtp_count;
	//printf("%ld vvv\n", involved_rtp_count);


	// update last_sequence_number_base
	priv->last_sequence_number_base = fec_packet->get_sequence_number_base(fec_packet);
	
    

#ifdef FEC_DEBUG
	printf("<FEC> %s: fec_receiver, %d, %d\n", __func__, p->payload[2], p->payload[3]);
	printf("<FEC> %s: fec_receiver fecevie fec with sn_base %d\n", __func__, fec_packet->get_sequence_number_base(fec_packet));
#endif

	return pool->insert_fec(pool, fec_packet);
}


static rtp_packet_t *get_rtp(fec_receiver_t *thiz, unsigned int sequence_number)
{
	return _get_rtp(thiz, sequence_number);
}

static rtp_packet_t *_get_rtp(fec_receiver_t *thiz, unsigned int sequence_number)
{
	// if it is exist in rtp pool, pop it
	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;
	rtp_pool_t *rtp_pool = priv->rtp_pool;
	fec_pool_t *fec_pool = priv->fec_pool;
		//printf("<FEC> Get RTP with sequence number %u \n", sequence_number);

	if (rtp_pool->check_rtp(rtp_pool, sequence_number)) {
#ifdef FEC_DEBUG
		printf("<FEC> Get RTP with sequence number %u Successfully\n", sequence_number);
#endif
		return rtp_pool->remove_rtp(rtp_pool, sequence_number);	
	}

	if (fec_pool->check_fec(fec_pool, sequence_number)) {
		fec_packet_t *p = fec_pool->get_fec(fec_pool, sequence_number);
		unsigned short sequence_number_base = p->get_sequence_number_base(p);
		unsigned short mask = p->get_mask(p);
		unsigned int mask_cont = p->get_mask_cont(p);

		unsigned int index = 0;
		bool is_recover = true;
		for (index=0; index<=48 && is_recover; index++) {
			if (sequence_number_base + index == sequence_number) continue;

			if (index == 0) {
				if (!rtp_pool->check_rtp(rtp_pool, sequence_number_base)) is_recover = false;
			}
			else if (1<=index && index<=16) {
				if (mask & (1<<(index-1))) {
					if (!rtp_pool->check_rtp(rtp_pool, sequence_number_base + index)) is_recover = false;
				}
			}
			else if (17<=index && index<=48) { 
				if (mask_cont & (1<<(index-17))) {
					if (!rtp_pool->check_rtp(rtp_pool, sequence_number_base + index)) is_recover = false;
				}
			}
		}

		if (is_recover) {
			fec_packet_t *fec = fec_pool->remove_fec(fec_pool, sequence_number);
			rtp_packet_t *rtp = NULL;

			for (index=0; index<=48 && is_recover; index++) {
				if (sequence_number_base + index == sequence_number) continue;

				if (index == 0) {
					rtp = rtp_pool->get_rtp(rtp_pool, sequence_number_base);
					fec->rtp_in(fec, rtp);
				}
				else if (1<=index && index<=16) {
					if (mask & (1<<(index-1))) {
						rtp = rtp_pool->get_rtp(rtp_pool, sequence_number_base + index);
						fec->rtp_in(fec, rtp);
					}
				}
				else if (17<=index && index<=48) {
					if (mask_cont & (1<<(index-17))) {
						rtp = rtp_pool->get_rtp(rtp_pool, sequence_number_base + index);
						fec->rtp_in(fec, rtp);
					}
				}
			}

			//fec_packet_print_packet(p, 100);
			unsigned int version = priv->version;
			unsigned int padding = fec->get_p_recovery(fec);
			unsigned int extension = fec->get_x_recovery(fec);
			unsigned int csrc_count = fec->get_cc_recovery(fec);
			unsigned int marker = fec->get_m_recovery(fec);
			unsigned int payload_type = fec->get_pt_recovery(fec);
			//unsigned int sequence is the parameters
			unsigned int timestamp = fec->get_timestamp_recovery(fec);
			unsigned int ssrc = priv->ssrc; 
			unsigned int payload_size = fec->get_length_recovery(fec);

			unsigned char *payload = (unsigned char*)malloc(payload_size);
			memcpy(payload, fec->get_payload_recovery(fec), payload_size);

			fec_packet_destroy(fec);

			rtp_packet_t *recoverd_rtp = rtp_packet_create(version, padding, extension, csrc_count, marker,
												payload_type, sequence_number, timestamp, ssrc, payload, payload_size);

#ifdef FEC_DEBUG
			printf("<FEC> Get RTP with sequence number %u Successfully\n", sequence_number);
#endif
            
            free(payload);
            
			return recoverd_rtp;
		

		}

	}

//	printf("<FEC> Fail to Get RTP with sequence number %u\n", sequence_number);
	return NULL;
}


static bool is_time_out(fec_receiver_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time)
{
	return _is_time_out(thiz, frame_count, packet_count, round_trip_time, playout_time);
}

static bool _is_time_out(fec_receiver_t *thiz, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time)
{
	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;

	//printf("frame_count = %d, packet_count = %d, involved = %d\n", frame_count, packet_count, priv->involved_rtp_count);
	
	// if the playout time is too early or too late, return true
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	int time_dist_ms = (playout_time.tv_sec - cur_time.tv_sec) * 1000 + (playout_time.tv_usec - cur_time.tv_usec) / 1000;
	//if (time_dist_ms > 350) return true;
	if (time_dist_ms < 50) return true;


	if (frame_count <= 3)
		return false;
	
	if (packet_count <= priv->involved_rtp_count + 5) 
		return false;

	if (frame_count * 33 <= 2*round_trip_time)
		return false;

//	printf("frame_count = %d, packet_count = %d, involved = %d\n", frame_count, packet_count, priv->involved_rtp_count);

	return true;
}

static bool is_request_re_transmit(fec_receiver_t *thiz, unsigned int sequence_number, unsigned int round_trip_time)
{
	return _is_request_re_transmit(thiz, sequence_number, round_trip_time);
}

static bool _is_request_re_transmit(fec_receiver_t *thiz, unsigned int sequence_number, unsigned int round_trip_time)
{
	fec_receiver_priv_t *priv = (fec_receiver_priv_t*)thiz->priv;

//	if (priv->last_sequence_number_base > sequence_number)
//		return false;

	//unsigned int received = sequence_number - priv->last_sequence_number_base;
	//unsigned int remain = priv->involved_rtp_count - received;

	//printf("sn_base %u\n", priv->last_sequence_number_base);
	//printf("remain = %u\n", remain);
	//printf("average = %u\n", priv->average_rtp_interval);
	//printf("rtt = %u\n", round_trip_time);

	return true;
	/* FIXME
	if (remain <= 48) {
		if (remain * priv->average_rtp_interval/1000 < round_trip_time * 10 / 8)
			return true;
	}

	return false;
	*/
}

