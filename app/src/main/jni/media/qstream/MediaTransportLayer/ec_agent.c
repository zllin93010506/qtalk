/*
 * FILE:     ec_agent.c
 * AUTHOR:   Corey Lin
 * MODIFIED: 
 *
 *
 * implement relative functions that are to query the ec (Error Correction) module 
 * 
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */

#include "../GlobalDefine.h"
#include "ec_agent.h"
#include "ec/ec.h"
#include "stdlib.h"

#define FEC_BANDWIDTH_RATIO 0.2

typedef struct ec_agent{
    ec_t* ecMod;
    int fec_on;
} ec_agent_t;


unsigned long ea_get_ecMod_id(struct ec_agent *ea)
{
    if(ea == NULL) return 0;
    else return (unsigned long) ea->ecMod;
}

bool ea_pbuf_isTimeout(struct ec_agent *ea, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time)
{
    ec_t* ecMod;
    bool isTimeout;

    if(ea == NULL) return true;


    ecMod =  ea->ecMod;

    isTimeout = ecMod->receiver_is_time_out(ecMod, frame_count, packet_count, round_trip_time, playout_time);

    return isTimeout;
}

bool ea_rfc4585_isRequestRetransmit(struct ec_agent *ea, int sequence_number, unsigned int round_trip_time)
{
    ec_t* ecMod;
    bool isRequestRetransmit;

    if(ea == NULL) return false;

    ecMod = ea->ecMod;

    isRequestRetransmit = ecMod->receiver_is_request_re_transmit(ecMod, sequence_number, round_trip_time);

    return isRequestRetransmit;
}

bool ea_rfc4588_isRetransmit(struct ec_agent *ea, unsigned int sequence_number)
{
    ec_t* ecMod;
    bool isRetransmit;

    if(ea == NULL) return false;

    ecMod = ea->ecMod;

    isRetransmit = ecMod->sender_is_re_transmit(ecMod, sequence_number);

    return isRetransmit;
}


int ea_update_plr(struct ec_agent *ea, double packet_loss_rate)
{
    ec_t* ecMod;
    int r;

    if(ea == NULL) return 0;


    ecMod = ea->ecMod;

    r  = ecMod->sender_update_current_packet_loss_rate(ecMod, packet_loss_rate);

    return r;

}

int ea_fec_isOn(struct ec_agent *ea)
{
    if(ea == NULL) return 0;

    return ea->fec_on;
}


static void _ea_recvRtp_in(struct rtp *session, struct ec_agent* ea,int pad, int x, uint32_t rtp_ts, char pt, int m, 
				int cc, unsigned char *data, int data_len, uint16_t seq, unsigned char *extn, uint16_t extn_len)
{
	ec_t* ecMod;
	rtp_packet_t* pkt;
	unsigned char* payloadForEC;
	uint16_t hdr_extn_len;

	//TODO:copy csrc fields
	if(extn_len > 0) hdr_extn_len = extn_len + 1;
	else	hdr_extn_len = extn_len;


	ecMod = (ec_t*) ea->ecMod;
	//fill in value
	payloadForEC = (unsigned char *)malloc(data_len+ hdr_extn_len*4);
	memcpy(payloadForEC, extn,	hdr_extn_len*4	) ;// allocate memory and merge data
	memcpy(payloadForEC + hdr_extn_len*4, data, data_len);
	pkt = rtp_packet_create(2,pad,x,cc,m,pt,seq,rtp_ts,rtp_my_ssrc(session),payloadForEC, data_len + hdr_extn_len*4);
	
    ecMod->receiver_rtp_in( ecMod, pkt);
		
	free(payloadForEC);
}

/*
 *Receiver function
 *
 *
 */

void ea_recvRtp_in(struct rtp *session, struct ec_agent *ea, rtp_packet *pkt, unsigned char *data, int data_len)
{
	int pad,x;
	char pt;
	uint32_t ts;
	int m,cc;
	uint16_t seq;
	uint16_t extn_len;
	unsigned char* extn;

	pad= pkt->fields.p;
	x  = pkt->fields.x;
	ts = pkt->fields.ts;		
	pt = pkt->fields.pt;	
	m  = pkt->fields.m;
	cc = pkt->fields.cc;
	seq = pkt->fields.seq;
	extn = pkt->meta.extn;
	extn_len = pkt->meta.extn_len;

	_ea_recvRtp_in( session, ea, pad,x, ts,  pt, m,  cc, data, data_len, seq, extn, extn_len);
}

static void _ea_sendRtp_in(struct rtp *session, struct ec_agent *ea,int pad, int x, uint32_t rtp_ts, char pt, int m, 
				int cc, unsigned char *data, int data_len, uint16_t seq, unsigned char *extn, uint16_t extn_len)
{
	ec_t* ecMod;
	rtp_packet_t* pkt;
	unsigned char* payloadForEC;
	uint16_t hdr_extn_len;	


	ecMod = (ec_t*) ea->ecMod;
	//fill in value
	//TODO:copy csrc fields
	if(extn_len > 0) hdr_extn_len  = extn_len + 1;
	else	hdr_extn_len = extn_len;

	payloadForEC = (unsigned char *)malloc(data_len+hdr_extn_len*4);
	memcpy(payloadForEC, extn,	hdr_extn_len*4	) ;// allocate memory and merge data
	memcpy(payloadForEC + hdr_extn_len*4, data, data_len);

//	printf("ec_agent ============= data_len = %d, extn_len = %d\n",data_len,extn_len);
	pkt = rtp_packet_create(2,pad,x,cc,m,pt,seq,rtp_ts,rtp_my_ssrc(session),payloadForEC, data_len + hdr_extn_len*4);

	ecMod->sender_rtp_in( ecMod, pkt);
	
	free(payloadForEC);
}


/*
 *Sender function
 *compute the extension length
 *parse data
 *
 */
void ea_sendRtp_in(struct rtp *session, struct ec_agent *ea, rtp_packet* pkt, unsigned char *data, int data_len)
{	
	int pad,x;
	char pt;
	uint32_t ts;
	int m,cc;
	uint16_t seq;
	uint16_t extn_len;
	unsigned char* extn;
	//printf("-> %s, %d\n",__func__,__LINE__);
	pad= pkt->fields.p;
	x  = pkt->fields.x;
	ts = ntohl(pkt->fields.ts);		
	pt = pkt->fields.pt;	
	m  = pkt->fields.m;
	cc = pkt->fields.cc;
	seq = ntohs(pkt->fields.seq);
	extn = pkt->meta.extn;
	extn_len = pkt->meta.extn_len;

	_ea_sendRtp_in( session, ea, pad,x, ts,  pt, m,  cc, data, data_len, seq, extn, extn_len);
}


double ea_get_fec_bandwidth_ratio(struct ec_agent *ea) // add by willy, 20120703
{
    if(ea == NULL) return 0;

	return ea->ecMod->sender_get_fec_bandwidth_ratio(ea->ecMod);
}

struct ec_agent* ec_agent_create(int fec_on)
{
    
	printf("%s\n",__func__);
    struct ec_agent* ins = (struct ec_agent*)malloc(sizeof(struct ec_agent));

    if(ins == NULL){
        printf("<warnning>: ec agent allocation failed\n");
        return NULL;
    }
    
	memset( ins, 0, sizeof(struct ec_agent));

    ins->ecMod = ec_create(fec_on);
    ins->fec_on = fec_on;
    ins->ecMod->sender_set_fec_bandwidth_ratio_upper_bound(ins->ecMod, FEC_BANDWIDTH_RATIO);
    ins->ecMod->sender_set_target_packet_loss_rate(ins->ecMod, 0.0001);

    return ins;
}


void ec_agent_destroy(struct ec_agent* ea)
{
    if(ea && ea->ecMod){
        printf("%s ()\n",__func__);
        ec_destroy(ea->ecMod);
        free(ea);
    }
}
