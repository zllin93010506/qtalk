/*
 * FILE:   tfrc.h
 * AUTHOR: Colin Perkins <csp@isi.edu>
 *         Ladan Gharai  <ladan@isi.edu>
 *
 * Copyright (C) 2002-2004 USC Information Sciences Institute
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *      This product includes software developed by the University of Southern
 *      California Information Sciences Institute.
 * 
 * 4. Neither the name of the University nor of the Institute may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Revision: 1.46 $
 * $Date: 2005/06/22 00:50:28 $
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
	
//#if defined(WIN32)	
//#include "..\global.h"
//#endif
	
#define TFRC_MAX_HISTORY      10000
#define TFRC_NUM_INTERVALS        8               /* number of intervals */
#define TFRC_MAX_DROPOUT       5000   /* perhaps this should be smaller? */
	/* anyway > MAX_DROPOUT packets are*/
	/* lost go back to slow start      */
#define TFRC_RTT_INIT	    1000000          /* intial RTT value  = 1sec */
#define TFRC_PSIZE_INIT	       1380	
#define TFRC_INIT_PCKTS_PER_RTT   1000      /* start with 4 packets per RTT */
#define	TFRC_XRECV_SMOOTH		  10		/* Number for SMOOTH x_recv in the receiver, added by Eric */
	
	struct tfrc_snd;
	struct tfrc_rcv;
	
	/*************  tfrc sender functions */
	struct tfrc_snd *tfrc_sndr_init(struct timeval tnow);
	void tfrc_sndr_release(struct tfrc_snd *tfrc);
	void		tfrc_poll_rcvr_feedback(struct tfrc_snd *tfrc, struct timeval tnow);
	uint32_t        tfrc_send_ok           (struct tfrc_snd *tfrc, uint32_t now);
	uint32_t        tfrc_rtt_flag          (struct tfrc_snd *tfrc);
	void            tfrc_set_rtt_flag      (struct tfrc_snd *tfrc, int a);
	uint32_t        tfrc_get_avg_rtt       (struct tfrc_snd *tfrc);
	uint32_t	tfrc_get_ipi           (struct tfrc_snd *tfrc);
	void		tfrc_set_sndr_psize    (struct tfrc_snd *tfrc, uint32_t a);
	uint32_t        tfrc_process_feedback  (struct tfrc_snd *tfrc, 
		struct timeval tnow,
		uint32_t tdelay, 
		uint32_t x_recv, 
		uint32_t send_ts, 
		uint32_t p); 
	uint32_t tfrc_get_send_time(struct tfrc_snd *tfrc);
	uint32_t tfrc_get_xsend(struct tfrc_snd *tfrc);
	double tfrc_get_sndr_p(struct tfrc_snd *tfrc);
	uint32_t tfrc_get_sndr_psize(struct tfrc_snd *tfrc);
	uint32_t tfrc_get_sndr_xrecv(struct tfrc_snd *tfrc);
	uint32_t tfrc_get_xcalc(struct tfrc_snd *tfrc);
	void tfrc_set_max_ipi(struct tfrc_snd *tfrc, uint32_t min_xsend);
    
	/*************  tfrc receiver functions */
	struct tfrc_rcv *tfrc_rcvr_init         (struct timeval tnow);
	void tfrc_rcvr_release(struct tfrc_rcv *tfrc);
	uint32_t	tfrc_transfer_rate      (double p, uint32_t rtt, uint16_t s );
	uint16_t	tfrc_send_rcvr_feedback (struct tfrc_rcv *tfrc, struct timeval now );
	void            tfrc_set_rcvr_psize     (struct tfrc_rcv *tfrc, uint32_t a);
	uint32_t	tfrc_get_rcvr_rtt       (struct tfrc_rcv *tfrc);
	uint16_t	tfrc_record_arrival     (struct tfrc_rcv *tfrc,  uint32_t now, 
		uint16_t seq, 
		uint32_t send_ts, 
		uint16_t rtt_flag, uint32_t rtt );
	void		tfrc_format_feedback    (struct tfrc_rcv *tfrc, 
		uint32_t now,
		uint32_t *tdelay,
		uint32_t *x_recv,
		uint32_t *send_ts,
		uint32_t *p);
	void		tfrc_set_p              (struct tfrc_rcv *tfrc, double p);
	double		tfrc_get_p              (struct tfrc_rcv *tfrc);
	double		tfrc_compute_p          (struct tfrc_rcv *tfrc);
	void tfrc_get_feedback_timer(struct tfrc_rcv *tfrc, struct timeval *t);
	void tfrc_reset_feedback_timer(struct tfrc_rcv *tfrc);

#ifdef __cplusplus
}
#endif

