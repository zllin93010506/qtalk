/*
 * FILE:     ec_agent.h
 * AUTHOR:   Corey Lin
 * DESCRIPTION: 
 *
 *
 * ec agent provides some interfaces 
 * to check if pbuf is timeout 
 * or to check if it's allowed to send the retransmission request and retransmission packet    
 * 
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */



#ifndef EC_AGENT_H
#define EC_AGENT_H

#include <stdbool.h>
#include "ec/rtp_packet.h"
#include "rtp.h"
struct ec_agent;



struct ec_agent* ec_agent_create(int fec_on);

unsigned long ea_get_ecMod_id(struct ec_agent *ea);

bool ea_pbuf_isTimeout(struct ec_agent *ea, unsigned int frame_count, unsigned int packet_count, unsigned int round_trip_time, struct timeval playout_time);

bool ea_rfc4585_isRequestRetransmit(struct ec_agent *ea, int sequence_number, unsigned int round_trip_time);

bool ea_rfc4588_isRetransmit(struct ec_agent *ea, unsigned int sequence_number);

int ea_fec_isOn(struct ec_agent *ea);

int ea_update_plr(struct ec_agent *ea, double packet_loss_rate);

void ea_sendRtp_in(struct rtp *session, struct ec_agent *ea, rtp_packet* pkt, unsigned char *data, int data_len);

void ea_recvRtp_in(struct rtp *session, struct ec_agent *ea, rtp_packet *pkt, unsigned char *data, int data_len);

double ea_get_fec_bandwidth_ratio(struct ec_agent *ea);// add by willy, 20120703


void ec_agent_destroy(struct ec_agent* ea);

#endif
