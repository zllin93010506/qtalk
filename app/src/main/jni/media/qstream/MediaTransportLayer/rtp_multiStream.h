#ifndef RTP_MULTISTREAM
#define RTP_MULTISTREAM

#include "ec/rtp_packet.h"
#include "ec/ec.h"
#include "rtp.h"




struct _rtp_mulStream;

typedef struct _rtp_mulStream rtp_mulStream;





rtp_mulStream* rtp_mulStream_init(unsigned long session_id, RTP_ECControl_t ec_arg);
void free_rtp_mulStream(rtp_mulStream* stream);
uint16_t get_rtp_mulStream_sendpt(rtp_mulStream* stream);
uint16_t get_rtp_mulStream_recvpt(rtp_mulStream* stream);

//sender function
int rtp_mulStream_fec_send(rtp_packet_t *p, void *context, unsigned int context_size);

//receiver function
void rtp_mulStream_recvFec_in(struct rtp *session, rtp_mulStream* fec_stream, rtp_packet *pkt, unsigned char* data, int data_len);
int rtp_mulStream_recover_rtp(struct rtp *session, rtp_mulStream* fec_stream,uint16_t seq);

#endif
