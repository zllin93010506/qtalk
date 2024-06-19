#ifndef _ET1_QSTREAM_RFC4585_H_
#define _ET1_QSTREAM_RFC4585_H_

#include "rtp.h"
#include "pbuf.h"
#include "ET1_QStream_SendingBuf.h"


uint8_t *rfc4585_format_rtpfb_generic_nack(uint8_t *buffer, int buflen, int pid, int bitmask, struct rtp *session);
uint8_t *rfc4585_format_psfb_picture_loss_indication(uint8_t *buffer, int buflen, struct rtp *session);
uint8_t *rfc4585_format_psfb_slice_loss_indication(uint8_t *buffer, int buflen, uint16_t first, uint16_t number, uint8_t picture_id, struct rtp *session);

int RFC4585_ProcessGenricNACK(struct rtp *session, rtcp_t *packet);

#endif // _ET1_QSTREAM_RFC4585_H_
