#include <stdio.h>
#include "rtp_multiStream.h"
#include "drand48.h"
#include "pdb.h"

#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)


struct _rtp_mulStream{
	uint16_t second_seq;
	uint16_t send_pt;
	uint16_t recv_pt;
	unsigned long rtp_session_id; /*record which rtp session this mulStream belong to */
	unsigned long ecMod_id;
};



uint16_t get_rtp_mulStream_sendpt(rtp_mulStream* stream)
{
	//printf("-> %s, %d\n",__func__,__LINE__);

	if(stream)
		return stream->send_pt; 
	
	printf("<warning> stream is NULL, %s()\n", __FUNCTION__);
	return 0;
}

uint16_t get_rtp_mulStream_recvpt(rtp_mulStream* stream)
{
	//printf("-> %s, %d\n",__func__,__LINE__);

	if(stream)
		return stream->recv_pt; 
	
	printf("<warning> stream is NULL, %s()\n", __FUNCTION__);
	return 0;
}


/*
 *rtp_mulStream_fec_send is a callback function which is used by FEC module
 *
 *
 */
int rtp_mulStream_fec_send(rtp_packet_t *p, void *context, unsigned int context_size)
{
	uint16_t sent_seq;
	int r=0;
	rtp_mulStream* stream ;
	

	//printf("-> %s\n",__func__);
	stream = (rtp_mulStream*)context;
	
	//printf("Send FEC packet : seq=%d, pt=%d\n",stream->second_seq, stream->send_pt);
	r = rtp_send_fec_hdr( (struct rtp*)stream->rtp_session_id, stream->second_seq, p->timestamp, stream->send_pt, p->marker, p->csrc_count,/*csrc*/ NULL, NULL, 0, p->payload, p->payload_size, &sent_seq, 0, 0, 0);
	
	//destroy rtp_packet
	rtp_packet_destroy(p);

	if(r > 0) stream->second_seq++;


	return r;
}

void _rtp_mulStream_recvFec_in(struct rtp *session, rtp_mulStream* fec_stream, int pad, int x, uint32_t rtp_ts, char pt, int m, 
				int cc, unsigned char *data, int data_len, uint16_t seq, unsigned char *extn, uint16_t extn_len)
{
	ec_t* ecMod;
	rtp_packet_t* pkt;
	unsigned char* payloadForEC;
	uint16_t hdr_extn_len;
	//printf("-> %s, %d\n",__func__,__LINE__);

	ecMod = (ec_t*) fec_stream->ecMod_id;
	//fill in value
	if(extn_len > 0) hdr_extn_len = extn_len + 1;
	else	hdr_extn_len = extn_len;

//	should not pass extn to fec module, set hdr_extn_len = 0, FEC doesn't need extn info, should not combine into data_payload
	hdr_extn_len = 0;
	payloadForEC = (unsigned char *)malloc(data_len+hdr_extn_len*4);
	memcpy(payloadForEC, extn,	hdr_extn_len*4	) ;// allocate memory and merge data
	memcpy(payloadForEC + hdr_extn_len*4, data, data_len);
	pkt = rtp_packet_create(2,pad,x,cc,m,pt,seq,rtp_ts,rtp_my_ssrc(session),payloadForEC, data_len + hdr_extn_len*4);
	
	ecMod->receiver_fec_in( ecMod, pkt);
		
	free(payloadForEC);
}

/*
 *Receiver function
 *
 *
 */

void rtp_mulStream_recvFec_in(struct rtp *session, rtp_mulStream* fec_stream, rtp_packet *pkt, unsigned char *data, int data_len)
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
	ts = pkt->fields.ts;		
	pt = pkt->fields.pt;
	m  = pkt->fields.m;
	cc = pkt->fields.cc;
	seq = pkt->fields.seq;
	extn = pkt->meta.extn;
	extn_len = pkt->meta.extn_len;

	_rtp_mulStream_recvFec_in( session, fec_stream,pad,x,ts,  pt, m,  cc, data, data_len, seq, extn, extn_len);
}
int _rtp_mulStream_insert_rtp(struct rtp *session, rtp_packet_t *pkt)
{
	
	pdb *participants = (pdb *) rtp_get_userdata(session);                                                                                                                                                      
	pdb_e *state = pdb_get(participants, rtp_opposite_ssrc(session) );
	int vlen = 12;
	int buflen = 0;
	rtp_packet *pbuf_packet=NULL;
	uint8_t	        *buffer   = NULL;
	uint8_t	        *buffer_vlen = NULL;

	//assert(state != NULL);
    if(state == NULL) {
        rtp_packet_destroy(pkt);
        return -1;
    }

	pbuf_packet = (rtp_packet *) malloc(sizeof(rtp_packet) + pkt->payload_size );
	if( NULL == pbuf_packet){
	    rtp_packet_destroy(pkt);
        return -1;
    }

	//reformat
	
	buffer =  ((uint8_t *) pbuf_packet) + offsetof(rtp_packet, fields);
	buflen =  sizeof(rtp_packet) - offsetof(rtp_packet,fields) + pkt->payload_size; 	
	buffer_vlen = buffer + vlen;	

	memcpy( buffer_vlen, pkt->payload, pkt->payload_size);


	pbuf_packet->fields.v    = pkt->version;
	pbuf_packet->fields.p    = pkt->padding;
	pbuf_packet->fields.x    = pkt->extension;
	pbuf_packet->fields.cc   = pkt->csrc_count;
	pbuf_packet->fields.m    = pkt->marker;
	pbuf_packet->fields.pt   = pkt->payload_type;
	pbuf_packet->fields.seq  = pkt->sequence_number;
	pbuf_packet->fields.ts	= pkt->timestamp;
	pbuf_packet->fields.ssrc = pkt->ssrc;


	if(pbuf_packet->fields.cc){
		int i;
		pbuf_packet->meta.csrc = (uint32_t *)(buffer_vlen);
		for(i=0; i< pbuf_packet->fields.cc; i++){
			pbuf_packet->meta.csrc[i] = ntohl(pbuf_packet->meta.csrc[i]);
		}
	}else{
		pbuf_packet->meta.csrc = NULL;
	}


	if(pbuf_packet->fields.x){
		pbuf_packet->meta.extn      = buffer_vlen + (pbuf_packet->fields.cc * 4);
		pbuf_packet->meta.extn_len  = (pbuf_packet->meta.extn[2] << 8) | pbuf_packet->meta.extn[3];
		pbuf_packet->meta.extn_type = (pbuf_packet->meta.extn[0] << 8) | pbuf_packet->meta.extn[1];
	}else{
		pbuf_packet->meta.extn      = NULL;
		pbuf_packet->meta.extn_len  = 0;
		pbuf_packet->meta.extn_type = 0;
	}

	pbuf_packet->meta.data     = buffer_vlen + (pbuf_packet->fields.cc * 4);
	pbuf_packet->meta.data_len = buflen -  (pbuf_packet->fields.cc * 4) - vlen;


	if(pbuf_packet->meta.extn != NULL){
		pbuf_packet->meta.data += ((pbuf_packet->meta.extn_len + 1) * 4);
		pbuf_packet->meta.data_len -= ((pbuf_packet->meta.extn_len + 1) * 4);
	}




	//Corey - FEC
    if(pbuf_packet->fields.pt == 109 || pbuf_packet->fields.pt == 110|| pbuf_packet->fields.pt == 99)
    	printf("[FEC, Video] - recover packet:%d, ts = %lu\n", pkt->sequence_number, pkt->timestamp);
    else
    	printf("[FEC, Audio] - recover packet:%d, ts = %lu\n", pkt->sequence_number, pkt->timestamp);

    pbuf_insert(state->playout_buffer, pbuf_packet);

	//destroy packet 
	rtp_packet_destroy(pkt);

	return 0;
}

int rtp_mulStream_recover_rtp(struct rtp *session, rtp_mulStream* fec_stream,uint16_t seq)
{
	ec_t* ecMod;
	rtp_packet_t *pkt_recover=NULL;


	ecMod = (ec_t*) fec_stream->ecMod_id;
	pkt_recover = ecMod->receiver_get_rtp(ecMod, seq);
	

	int ret=-1;
	if(pkt_recover)
		ret = _rtp_mulStream_insert_rtp(session, pkt_recover);

	return ret;

}



/*
 *allocate a rtp_mulStream and return to Caller
 *
 *
 */
rtp_mulStream* rtp_mulStream_init(unsigned long session_id, RTP_ECControl_t ec_arg)
{
	rtp_mulStream* stream;
	
	printf("-> %s, %d\n",__func__,__LINE__);

	stream = (rtp_mulStream *) malloc(sizeof(rtp_mulStream));

	if(NULL == stream){
		return NULL;
	}

	memset( stream, 0, sizeof(rtp_mulStream));
	
	stream->send_pt	       =   ec_arg.send_payloadType;
	stream->recv_pt        =   ec_arg.receive_payloadType;
	stream->second_seq	   =   (uint16_t) lrand48();
	stream->ecMod_id       =   ec_arg.ecMod_id;
	stream->rtp_session_id =   session_id;

	if(stream->ecMod_id > 0){
		// register callback function
		ec_t *fec = (ec_t*) stream->ecMod_id;
		fec->sender_register_rtp_out(fec,rtp_mulStream_fec_send, (void*)stream, sizeof(struct _rtp_mulStream));
	}
	return stream;

}
void free_rtp_mulStream(rtp_mulStream* stream)
{


	if(stream ){
	    printf("%s()\n",__func__);
		free(stream);
	}
}





