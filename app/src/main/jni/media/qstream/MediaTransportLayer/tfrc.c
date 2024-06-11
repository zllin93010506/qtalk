/*
 * FILE:     tfrc.c
 * AUTHOR:   Ladan Gharai <ladan@isi.edu>
 * MODIFIED: Colin Perkins <csp@csperkins.org>
 *
 * Copyright (c) 2002-2005 USC Information Sciences Institute
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
 * $Revision: 1.80 $
 * $Date: 2005/06/22 19:19:42 $
 *
 */
#ifdef WIN32
#include "config_unix.h"
#endif
/*
//#include "config.h"
#include <math.h>
//#include <assert.h>
#include <stdlib.h>
//#include <stdint.h>
#include <stdio.h>
//#include <sys/time.h>
//#include <netinet/in.h>
*/

//#include "config.h"
#include <math.h>
//#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifndef WIN32
#include <sys/time.h> // Frank enable for PX2
#endif
//#include <netinet/in.h>

#include "tv.h"
#include "tfrc.h"
#include "debug.h"

#define MY_MIN(A,B)     (A<B? A:B)
#define MY_MAX(A,B)	(A>B? A:B)

#define TFRC_MAGIC	0xbaef03b7	                     /* For debugging */
#define RTP_SEQ_MOD	0x10000
#define MAX_MISORDER	500
#define T_MBI		64              /* the maximum inter-packet backoff   */
      					/* interval = 64 sec 		      */
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct tfrc_history {
        uint32_t seq;
        uint32_t ts;
}; 

/*
 * TFRC reciever variables
 */
struct tfrc_rcv {
        double          p;
	uint32_t        lsend_ts;        /* timestamp of last packet received */
	uint32_t        x_recv;
        uint32_t        rcv_rtt;    /* rtt receiver extracts from rtp packets */
	uint32_t	pckt_size;
        uint32_t        sent_srx;         /* time last TFRC feedback was sent */
	uint32_t        last_pckt_t;               /* rtp packet receive time */
	uint32_t	num_pckts;   /* no packets received since last feedbck*/
	struct timeval  feedbk_timer;

        uint16_t 	last_seq;       
        uint32_t 	ext_last_seq;
        int      	last_seq_jj;
	uint16_t	interval_count;   
        double          W_tot;
        double          weight[TFRC_NUM_INTERVALS+5]; /* weights for p RFC3448 */
        int             jj;       /* index for arrival -1<=i<=TFRC_MAX_HISTORY */
        int             ii;                    /* index for loss    -1<=ii<=10 */
        int             ooo;                 /* number of out of order packets */
        int             cycles;     /* number of times seq number cycles 65535 */
	int             loss_count;
	int             total_pckts;
	struct 
	  tfrc_history  loss[TFRC_MAX_HISTORY],
        	        arrival[TFRC_MAX_HISTORY];
        uint32_t        magic;          	              /* For debugging */
	uint32_t		xrecv_number[TFRC_XRECV_SMOOTH];		//Eric, to smooth X_recv
	uint32_t		xrecv_rtt[TFRC_XRECV_SMOOTH];			//Eric, to smooth X_recv
};

/*
 * TFRC sender variables
 */
struct tfrc_snd {
        uint32_t         avg_rtt;             /* rtt as computed by the sender */
        uint16_t         new_rtt;           /* flag change in value of the RTT */
        uint32_t         pckt_size;          /* data_len of tfrc send data */
        uint32_t         x_send;
	uint32_t	 send_time;
        uint32_t         ipi;
	uint32_t	 tld;
	uint32_t	 x_recv,
			 x_calc;
	uint32_t	 p;
	struct timeval	 nofeedbk_timer;
	uint32_t	max_ipi;				/* Add by Eric, the maxmium inter-packet-interval (us) */
	uint32_t        magic;
};


/******************************************************************************/
/*									      */
/******************************************************************************/
#if 0
static void
validate_tfrc_rcvr (struct tfrc_rcv *tfrc)
{
//	assert(tfrc->magic == TFRC_MAGIC);
}

static void
validate_tfrc_sndr (struct tfrc_snd *tfrc)
{
//        assert(tfrc->magic == TFRC_MAGIC);
}
#endif 

/* use reverse of Matt Mathis formula with c=1.22*/
static double
compute_init_p ( uint32_t RTT, uint16_t s, uint32_t rate ) {
        double rtt, tmp=0;

//        assert (RTT!=0);
//	assert (rate!=0);
        
        /* convert RTT from micro-sec to sec */
        if(RTT == 0 || rate ==0)
	{	
		tmp = 0.05;
	}
	else
	{
			rtt = ((double) RTT) / 1000000.0;
			tmp = (1.2*s)/(rtt*rate);
			tmp = tmp * tmp;
			//tmp = (1.44 * s * s )/(rtt*rtt*rate*rate);
			//tmp = 0.05;
			if(tmp > 0.05)
				tmp = 0.05;
			if(tmp < 0.0001)
				tmp = 0.0001;
	}

	return (tmp);
}


uint32_t 
tfrc_transfer_rate(double p, uint32_t RTT, uint16_t s )
{
	double t1, t2, t3, t4, rtt, tRTO;
	uint32_t t;

//	assert ((p>=0) && (p<=1));

	/* convert RTT from micro-sec to sec */
	rtt = ((double) RTT) / 1000000.0;
	tRTO = 4 * rtt;

	t1 = rtt * sqrt(2 * p / 3);
	t2 = (1 + 32 * p * p);
	t3 = 3 * sqrt(3 * p / 8);
	t4 = t1 + tRTO * t3 * p * t2;

	if((t4 < 0.000001) && (t4 > -0.000001))
		t = 1000000000;
	else
		t = ((double) s) / t4;



	/*TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "\nrtt:%10.8f p:%10.8f \nt1:%10.8f \nt2:%10.8f \nt3:%10.8f  
	\nt4:%10.8f  \nt:%d", rtt, p, t1, t2, t3, t4, t); */

	return (t);
}

static int
set_zero(struct tfrc_rcv *tfrc, int first, int last, uint16_t u)
{
	int i, count = 0;;

//	assert((first >= 0) && (first < TFRC_MAX_HISTORY));
//	assert((last >= 0) && (last < TFRC_MAX_HISTORY));

	if (first == last)
		return (0);
	if ((first + 1) % TFRC_MAX_HISTORY == last)
		return (1);

	if (first < last) {
		for (i = first + 1; i < last; i++) {
			tfrc->arrival[i].seq = 0;
			tfrc->arrival[i].ts = 0;
			count++;
		}
	} else {
		for (i = first + 1; i < TFRC_MAX_HISTORY; i++) {
			tfrc->arrival[i].seq = 0;
			tfrc->arrival[i].ts = 0;
			count++;
		}
		for (i = 0; i < last; i++) {
			tfrc->arrival[i].seq = 0;
			tfrc->arrival[i].ts = 0;
			count++;
		}
	}
//	assert(count == (u - 1));
	return (count);
}


static int
arrived(struct tfrc_rcv *tfrc, int first, int last)
{
	int i, count = 0;

//	assert((first >= 0) && (first < TFRC_MAX_HISTORY));
//	assert((last >= 0) && (last < TFRC_MAX_HISTORY));

	if (first == last)
		return (0);
	if ((first + 1) % TFRC_MAX_HISTORY == last)
		return (0);

	if (first < last) {
		for (i = first; i <= last; i++) {
			if (tfrc->arrival[i].seq != 0)
				count++;
		}
	} else {
		for (i = first; i < TFRC_MAX_HISTORY; i++) {
			if (tfrc->arrival[i].seq != 0)
				count++;
		}
		for (i = 0; i <= last; i++) {
			if (tfrc->arrival[i].seq != 0)
				count++;
		}
	}
	return (count);
}


static void
record_loss(struct tfrc_rcv *state, uint32_t s1, uint32_t s2, uint32_t ts1, uint32_t ts2)
{
	/* Mark all packets between s1 (which arrived at time ts1) and */
	/* s2 (which arrived at time ts2) as lost.                     */
	int 		i;
	uint32_t 	est;
	uint32_t 	seq = s1 + 1;

	//est = ts1 + (ts2 - ts1) * ((seq - s1) / (s2 - seq));
	
	est = (uint32_t)(ts1 + (ts2 - ts1) * ((seq - s1)/(double)(s2 - s1)) + 0.5);
	
	state->loss_count++;
	if (state->ii <= -1) {
		/* first loss! */
		state->ii++;
		state->loss[state->ii].seq = seq;
		state->loss[state->ii].ts = est;
		return;
	}

	/* not a new event */
	if (est - state->loss[state->ii].ts <= state->rcv_rtt) {	
		return;
	}

	state->interval_count++;
	if (state->ii > (TFRC_NUM_INTERVALS + 1)) {
		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "how did this happen?\n");
	}

	if (state->ii >= (TFRC_NUM_INTERVALS + 1)) {	/* shift */
		for (i = 0; i < (TFRC_NUM_INTERVALS + 1); i++) {
			state->loss[i].seq = state->loss[i + 1].seq;
			state->loss[i].ts = state->loss[i + 1].ts;
		}
		state->ii = TFRC_NUM_INTERVALS;
	}

	state->ii++;
	state->loss[state->ii].seq = seq;
	state->loss[state->ii].ts = est;

//	TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n ii=%d, loss_count=%d, interval_count=%d", state->ii, state->loss_count, state->interval_count);
//	for (i = 0; i < (TFRC_NUM_INTERVALS + 1); i++) {
//		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n--------------------------------Loss_Record loss[%d] = %d", i, state->loss[i].seq);
//	}
}

static void
reverse_record_loss(struct tfrc_rcv *state)
{
	int i;

	state->loss_count--;	
	state->interval_count--;

	if(state->ii == -1)
		return;
	
	state->loss[state->ii].seq = 0;
	state->loss[state->ii].ts = 0;
	
	if (state->ii == TFRC_NUM_INTERVALS) {
		for (i = TFRC_NUM_INTERVALS; i > 0; i--) {
			state->loss[i].seq = state->loss[i - 1].seq;
			state->loss[i].ts = state->loss[i - 1].ts;
		}
		state->loss[0].seq = state->loss[1].seq;
		state->loss[0].ts = state->loss[1].ts;
		state->ii = TFRC_NUM_INTERVALS;
	}

	state->ii--;
	

//	TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n ii=%d, loss_count=%d, interval_count=%d", state->ii, state->loss_count, state->interval_count);
//	for (i = 0; i < (TFRC_NUM_INTERVALS + 1); i++) {
//		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n---------------------------Reverse_Loss_Record loss[%d] = %d", i, state->loss[i].seq);
//	}
}
/******************************************************************************/
/*                                                                            */
/*  External API follows...						      */
/*                                                                            */
/******************************************************************************/
uint32_t
tfrc_rtt_flag (struct tfrc_snd *tfrc) 
{
//	assert (tfrc!=NULL);
	return (tfrc->new_rtt==1);
}


void 
tfrc_set_rtt_flag (struct tfrc_snd *tfrc, int a)
{
//	assert (tfrc!=NULL);
	tfrc->new_rtt=a;
}


uint32_t
tfrc_get_avg_rtt (struct tfrc_snd *tfrc)
{
//	assert (tfrc!=NULL);
	return (tfrc->avg_rtt);
}

uint32_t
tfrc_get_ipi (struct tfrc_snd *tfrc)
{
//        assert (tfrc!=NULL);
        return (tfrc->ipi);
}

uint32_t 
tfrc_get_send_time(struct tfrc_snd *tfrc)
{
	return (tfrc->send_time);
}

void
tfrc_set_sndr_psize (struct tfrc_snd *tfrc, uint32_t a)
{
//        assert (tfrc!=NULL);
        tfrc->pckt_size = a;
}

uint32_t
tfrc_get_sndr_psize(struct tfrc_snd *tfrc)
{
	return (tfrc->pckt_size);
}

uint32_t tfrc_get_xsend(struct tfrc_snd *tfrc)
{
	return (tfrc->x_send);
}

double tfrc_get_sndr_p(struct tfrc_snd *tfrc)
{
	double temp;
	temp = tfrc->p/(double)4294967295.0;
	return (temp);
}

uint32_t 
tfrc_get_sndr_xrecv(struct tfrc_snd *tfrc)
{
	return (tfrc->x_recv);	
}

uint32_t 
tfrc_get_xcalc(struct tfrc_snd *tfrc)
{
	return (tfrc->x_calc);
}

uint32_t
tfrc_get_rcvr_rtt(struct tfrc_rcv *tfrc)
{
//	assert (tfrc!=NULL);
	return tfrc->rcv_rtt;
}


void
tfrc_set_rcvr_psize (struct tfrc_rcv *tfrc, uint32_t a)
{
	//assert (tfrc!=NULL);
	if (tfrc)
		tfrc->pckt_size = a;
}


void
tfrc_set_p (struct tfrc_rcv *tfrc, double p)
{
//	assert (tfrc!=NULL);
        tfrc->p = p;
}


double
tfrc_get_p (struct tfrc_rcv *tfrc)
{
//	assert (tfrc!=NULL);
       	return (tfrc->p);
}

void tfrc_get_feedback_timer(struct tfrc_rcv *tfrc, struct timeval *t)
{
	t->tv_sec = tfrc->feedbk_timer.tv_sec;
	t->tv_usec = tfrc->feedbk_timer.tv_usec;
}


double
tfrc_compute_p(struct tfrc_rcv *tfrc)
{
        int             i;
        //uint32_t        temp, I_tot0 = 0, I_tot1 = 0, I_tot = 0;
        double temp, I_tot0 = 0, I_tot1 = 0, I_tot = 0;
		double          p,
                        I_mean;
	static double init_p=0;
	//static double last_p=0;
	//static double diff_p=0;
	


//	assert (tfrc!=NULL);

        if (tfrc->ii < TFRC_NUM_INTERVALS) {
                if (tfrc->ii ==-1){
                        /* no loss occured yet */
                        return 0;
		}	
                else {
		        /* have seen some loss */
			if (!init_p) {
				init_p = compute_init_p (tfrc->rcv_rtt, tfrc->pckt_size, tfrc->x_recv );
				//TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n%8.6f x_recv:%d %d %d\n", 
				//init_p, (tfrc->x_recv*8)/1000000, tfrc->pckt_size, tfrc->rcv_rtt);
				return init_p; 
			}
			for (i = 0; i <= tfrc->ii; i++) {
				temp = tfrc->loss[i + 1].seq - tfrc->loss[i].seq;
				I_tot0 = I_tot0 + temp;
				I_tot1 = I_tot0;
			} 
		     
		}
        }

        for (i = tfrc->ii - TFRC_NUM_INTERVALS; i < tfrc->ii; i++) {	
                temp = tfrc->loss[i + 1].seq - tfrc->loss[i].seq;
		//if(diff_p > 0) {
		//	TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n--------------------------------------------------------------------- loss_packet_seq = %u ",tfrc->loss[i + 1].seq);
		//}
                I_tot0 = I_tot0 + temp * tfrc->weight[i];
                if (i >= (tfrc->ii - TFRC_NUM_INTERVALS + 1)) {
                        I_tot1 = I_tot1 + temp * tfrc->weight[i - 1];
                }
        }
        I_tot1 = I_tot1 + (tfrc->arrival[tfrc->jj].seq - tfrc->loss[tfrc->ii].seq) 
						* tfrc->weight[TFRC_NUM_INTERVALS - 1];

        I_tot = (I_tot1 > I_tot0) ? I_tot1 : I_tot0;
        I_mean = I_tot / tfrc->W_tot;
        p = 1 / I_mean;
	//if(diff_p > 0.00001) {
	//	TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2,"\nEnd, loss_event_rate = %5.6f\n", p);
	//}	
	//diff_p = p - last_p;
	//last_p = p;
        /*TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n%d %d %d %10.2f", I_tot, I_tot1, I_tot0, p );*/

        return p;
}


/*
 * update sending rate per section 4.3 bullet 4.
 */
static void
update_sending_rate ( struct tfrc_snd *tfrc, uint32_t now)
{
        double tmp = (double) tfrc->p/4294967295.0;
        static int print_count = 0;
        int thecase = 0;
	
	static double pre_p;
	static uint32_t pre_x_send;
	static uint32_t hold_x_send;
	//static int first_loss = 1;
	
        /* update the sending rate */
       // if (tfrc->p>0) {
                if(tmp < 0.0002)
                {
                    thecase = 0;
                    tmp = (double) 0.0002;
                }
	//=== for p ===//
	if( tmp > pre_p ) {
		tfrc->x_send = 0.85 * pre_x_send;
		hold_x_send = tfrc->x_send;		
	}
	else {

                tfrc->x_calc = tfrc_transfer_rate (tmp, tfrc->avg_rtt, tfrc->pckt_size );
                tfrc->x_send = MY_MIN ( tfrc->x_calc, (uint32_t)(1.2*tfrc->x_recv) );
		if( hold_x_send > tfrc->x_send ) {
			tfrc->x_send = hold_x_send;
		}
                print_count ++;
                if(print_count > 5)
                { 
                    print_count = 0;
                    TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK2, "tfrc->x_send = %u tfrc->x_calc= %u tfrc->x_recv=%u tmp=%f thecase=%d\n",  tfrc->x_send, tfrc->x_calc,  tfrc->x_recv,tmp,thecase);
                }
                //tfrc->x_send = MAY_MAX ( tfrc->x_send, XXX);
			
				//Eric, debug message
               	TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(w P)Xs = Min(Xc:%5d, 1.2Xr:%5d) ", tfrc->x_calc*8/1000, (uint32_t)(1.2*tfrc->x_recv*8)/1000);
		if ( tfrc->x_calc < (uint32_t)(1.2*tfrc->x_recv) ) {
		    TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [    Xc] ");
		} else {
			TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [ 1.2Xr] ");
		}					    
        //}
#if 0
        else if (tfrc->p==0) {
                /* still in slow start */
                /* FIXME: rtt -or- tfrc->avg_rtt */
                if ((tfrc->tld + tfrc->avg_rtt)<now) {
						//Eric, debug message
						TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(n P)Xs = Min(2Xs:%5d, 2Xr:%5d) ", 2*tfrc->x_send*8/1000, 2*tfrc->x_recv*8/1000);
						if (2*tfrc->x_send < 2*tfrc->x_recv) {
				    		TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [   2Xs] ");
						} else {
							TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [   2Xr] ");
						}
                        tfrc->x_send = MY_MIN (2*tfrc->x_send, 2*tfrc->x_recv);
                        //tfrc->x_send = MAY_MAX ( tfrc->x_send, XXX)
                        tfrc->tld=now;                        
                }
				else {
					//Eric, debug message
				    TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(n P)(in rtt)\t\t\t     ");
					TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [      ] ");
				}
        }
#endif
	tfrc->x_send = (tfrc->x_send > 5000000/8) ? (5000000/8) : tfrc->x_send;	//Eric
		
	}
	//=== for p ===//	
	pre_x_send = tfrc->x_send;
	pre_p = tmp;
	//else TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "\n how did we end up with p=%d, %f", tfrc->p, tmp);
}



void
tfrc_poll_rcvr_feedback(struct tfrc_snd *tfrc, struct timeval tnow)
{
	uint32_t now;

//	assert (tfrc!=NULL);

	if (tv_gt (tnow, tfrc->nofeedbk_timer)){
		//Eric, debug message
		TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "\n[TFRC_Sndr, NFB]   ");

		if (tfrc->x_recv) {
			/* if we have received feedback */
			if (tfrc->x_calc > 2*tfrc->x_recv || tfrc->x_calc==0) {
				tfrc->x_recv = MY_MAX ( tfrc->x_recv/2, tfrc->pckt_size/(2*T_MBI));
                                //printf("poll_rcvr_feedback !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				//Eric, debug message
				TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(w Xr)Xr=Xr/2 ");
				//TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [  Xr/2] ");
			}
			else {
			 	tfrc->x_recv = tfrc->x_calc/4;
                                //printf("poll_recvr_feedback @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
				//Eric, debug message
				TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(w Xr)Xr=Xc/4 ");
				//TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [  Xc/4] ");
			}
	
			now = timeval_to_usec (tnow);
			update_sending_rate (tfrc, now);
		}
		else {
			tfrc->x_send = MY_MAX ( tfrc->x_send/2, tfrc->pckt_size/T_MBI);
                        //printf("poll_rcvr_feedback ######################################################################################\n");
			//Eric, debug message
			TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(n Xr)Xs=Xs/2");
			//TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " [  Xs/2] ");
		}

		
//        	assert (tfrc->x_send!=0);
		
		if(tfrc->x_send == 0)
		{
			//Eric, define the shortest ipi = 35ms
			//tfrc->ipi = 35000;
			//tfrc->ipi = 1000000;	
			tfrc->ipi = tfrc->max_ipi;
		}
		else
		{
			tfrc->ipi = (uint32_t)((tfrc->pckt_size*1000000.0)/tfrc->x_send + 0.5);
			//Eric, define the shortest ipi = 35ms
			//if(tfrc->ipi > 35000)
			//	tfrc->ipi = 35000;
			//if(tfrc->ipi > 1000000)
			//	tfrc->ipi = 1000000;
			if(tfrc->ipi > tfrc->max_ipi)
				tfrc->ipi = tfrc->max_ipi;
		}

		//Eric, debug message
     	TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "    ---, %6d kbps | %8.6f ---- %d ipi:%d ",
                             (tfrc->x_send*8)/1000,
                             tfrc->p/4294967295.0, tfrc->avg_rtt, tfrc->ipi);
	
                /* reset the timer */
                tv_add_usec (&tnow, MY_MAX (4*tfrc->avg_rtt, 4*tfrc->ipi));
                tfrc->nofeedbk_timer.tv_sec = tnow.tv_sec;
                tfrc->nofeedbk_timer.tv_usec = tnow.tv_usec;
	}
}

uint16_t
tfrc_send_rcvr_feedback (struct tfrc_rcv *tfrc, struct timeval now) 
{
//	assert (tfrc!=NULL);

	if (tv_gt (now, tfrc->feedbk_timer)){
		/* reset the timer */
		tv_add_usec (&now, tfrc->rcv_rtt);
		tfrc->feedbk_timer.tv_sec = now.tv_sec;
        tfrc->feedbk_timer.tv_usec = now.tv_usec;

		if (tfrc->num_pckts>0) {
			//Eric, debug message
			//TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t[TFRC_rcvr, FB]  rcv_rtt=%7d ", tfrc->rcv_rtt);
			TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n[TFRC_rcvr, FB]  rcv_rtt=%5d ", tfrc->rcv_rtt);
			return (1);
		}
		else
			return (0);

	}
	else
		return (0);
}

void
tfrc_reset_feedback_timer(struct tfrc_rcv *tfrc)
{
	struct timeval now;
	gettimeofday (&now, NULL);	
	tv_add_usec(&now, tfrc->rcv_rtt);
	tfrc->feedbk_timer.tv_sec = now.tv_sec;
	tfrc->feedbk_timer.tv_usec = now.tv_usec;
}

uint32_t        
tfrc_send_ok (struct tfrc_snd *tfrc, uint32_t now)
{
//	assert (tfrc!=NULL);
	//Add by Eric Fang, avoid TFRC warp-around issue
	uint32_t utdelay;
	utdelay = tfrc->send_time - now;
	
	return 1;/* we need not to perform data flow control here, so just return 1 */

	if ( (now < tfrc->send_time) && (utdelay < 2147483648) ) {	//2147483648=2^31=0x80000000
		return (0);
	}
	else {
		if( utdelay < 4294967296 - 10000000 ) {				//4294967296=2^32
			tfrc->send_time = now - 100000;					//send_time sync: avoid that video is rejected when utdelay < 2^31 with "now" wrap-around
			TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK2, "\n[%u] tfrc_send_ok adjust the send_time to do time synci......  ", now);
		}
		tfrc->send_time+=tfrc->ipi;
		//Eric, debug message
		//TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "(%lu)", tfrc->send_time);
		return (1);
	}
}

uint16_t
tfrc_record_arrival(struct tfrc_rcv *tfrc,  uint32_t now, 
			uint16_t seq, uint32_t send_ts, uint16_t rtt_flag, uint32_t rtt )
{
	int 		kk, 
			inc, 
			last_jj;
	int 		i, num_diff;
	uint16_t 	udelta;
	uint32_t	ext_seq;
 
//	assert (tfrc!=NULL);
 
	/* squirrel away some info */ 
        tfrc->last_pckt_t = now;
	tfrc->lsend_ts = send_ts;
	tfrc->num_pckts++;

	if (tfrc->jj == -1) {
		/* first packet arrival */
		tfrc->jj = 0;
		tfrc->last_seq = seq;
		tfrc->ext_last_seq = seq;
		tfrc->last_seq_jj = 0;
		tfrc->arrival[tfrc->jj].seq = seq;
		tfrc->arrival[tfrc->jj].ts = now; 
		tfrc->lsend_ts = send_ts;
                if (rtt_flag) {
                        /* new RTT arrived in RTP packet */
                        tfrc->rcv_rtt = rtt;
		}
		return 0;
	}

	udelta = seq - tfrc->last_seq;
	//if (udelta!=1) printf("\ndiff:%d seq:%u last_seq:%u", udelta, seq, tfrc->last_seq );

	tfrc->total_pckts++;
	if (udelta < TFRC_MAX_DROPOUT) {
		/* in order, with permissible gap */
		if (seq < tfrc->last_seq) {
			tfrc->cycles++;
		}
		/* record arrival */
		last_jj = tfrc->jj;
		tfrc->jj = (tfrc->jj + udelta) % TFRC_MAX_HISTORY;
		set_zero(tfrc, last_jj, tfrc->jj, udelta);
		ext_seq = seq + tfrc->cycles * RTP_SEQ_MOD;
		tfrc->last_seq = seq;
		tfrc->arrival[tfrc->jj].seq = ext_seq;
		tfrc->arrival[tfrc->jj].ts = now;
                tfrc->lsend_ts = send_ts;
                if (rtt_flag) {
                        /* new RTT arrived in RTP packet */
                        tfrc->rcv_rtt = rtt;
		}

		if ((ext_seq - tfrc->ext_last_seq) == 1) {
			/* We got two consecutive packets, no loss */
			tfrc->ext_last_seq = ext_seq;
			tfrc->last_seq_jj = tfrc->jj;
		} else {
			/* Sequence number jumped, we've missed a packet for some reason */
			/* EX: arrived()=2 for  3654 and  3656 */
			if (arrived(tfrc, tfrc->last_seq_jj, tfrc->jj) >= 2) {
				TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\nsequence number jumped, record to loss!        udelta:%d seq:%u last_seq:%u", udelta, ext_seq, tfrc->last_seq );
				//printf("\nsequence number jumped, record to loss!        udelta:%d seq:%u last_seq:%u", udelta, ext_seq, tfrc->last_seq );
				record_loss(tfrc, tfrc->ext_last_seq, ext_seq,
					    tfrc->arrival[tfrc->last_seq_jj].ts, now);
				tfrc->ext_last_seq = ext_seq;
				tfrc->last_seq_jj = tfrc->jj;
			}
			/* the else here can be useful for emualting TCP */
						
		}
	} else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		/* Add by Eric, ****************** to reset the TFRC arrival[] and loss[] *******************/
		//TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n[TFRC_Rcvr]*********u:%d, seq:%u last_seq:%u\n", udelta, seq, tfrc->last_seq);		
		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n[TFRC_Rcvr](Why to go to this step???        u:%d, seq:%u last_seq:%u", udelta, seq, tfrc->last_seq);
		if (seq < tfrc->last_seq) {
			tfrc->cycles++;
		}
		ext_seq = seq + tfrc->cycles * RTP_SEQ_MOD;
		tfrc->jj = 0;
		tfrc->last_seq = seq;
		tfrc->ext_last_seq = ext_seq;
		tfrc->last_seq_jj = 0;
		for (i = 0; i < TFRC_MAX_HISTORY; i++) {
			tfrc->arrival[i].seq = 0;
			tfrc->arrival[i].ts  = 0;
			tfrc->loss[i].seq = 0;
			tfrc->loss[i].ts  = 0;
		}
		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n[TFRC_Rcvr] Reset the arrival and loss record!\n");
		tfrc->arrival[tfrc->jj].seq = seq;
		tfrc->arrival[tfrc->jj].ts = now; 
		tfrc->lsend_ts = send_ts;
                if (rtt_flag) {
                        /* new RTT arrived in RTP packet */
                        tfrc->rcv_rtt = rtt;
		}
		return 0;		
		/* Add by Eric, ****************** end of reset TFRC arrival[] and loss[] *******************/
	} else {
		/* duplicate or reordered packet */
		/* should we save the rtt and lsend_ts in this instance too? */
		/* lots of other consideration here -LG-		     */
		//static int i=0;
		debug_msg ("=================");
		//if (i++>100) abort();
		ext_seq = seq + tfrc->cycles * RTP_SEQ_MOD;
		tfrc->ooo++;
		num_diff = tfrc->ext_last_seq - ext_seq;
		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n ================= [TFRC_Rcvr out-of-order]    Seq_diff: %u - %u = %d\n", ext_seq, tfrc->ext_last_seq, num_diff);						
		//if (ext_seq > tfrc->ext_last_seq) {
		if(num_diff > 0 && num_diff <=1) {
			TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\n ================= [TFRC_Rcvr out-of-order]    Seq_diff: %u - %u = %d\n", ext_seq, tfrc->ext_last_seq, num_diff);
			inc = ext_seq - tfrc->arrival[tfrc->jj].seq;
			TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, "\n\t\t\t\t\t\t    (ext_seq)%u - (arrival[].seq)%u = (inc)%d\n", ext_seq, tfrc->arrival[tfrc->jj].seq, inc);
			kk = (tfrc->jj + inc) % TFRC_MAX_HISTORY;
			if (tfrc->arrival[kk].seq == 0) {
				tfrc->arrival[kk].seq = ext_seq;
				/* NOT the best interpolation */
				reverse_record_loss(tfrc);
				TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK2, "\nreverse record loss!");
				tfrc->arrival[kk].ts  = (tfrc->arrival[tfrc->last_seq_jj].ts + now) / 2;	
			}
			while (tfrc->arrival[tfrc->last_seq_jj + 1].seq != 0
			       && tfrc->last_seq_jj < tfrc->jj) {
				tfrc->last_seq_jj = (tfrc->last_seq_jj + 1) % TFRC_MAX_HISTORY;
			}
			tfrc->ext_last_seq = tfrc->arrival[tfrc->last_seq_jj].seq;
			
		}
	}
	return udelta;
}


void
tfrc_format_feedback ( struct tfrc_rcv *tfrc,  uint32_t now, 
				uint32_t *tdelay, uint32_t *x_recv, uint32_t *send_ts, uint32_t *p)
{
	uint32_t my_tdelay = 0,
		 my_xrecv = 0;
		
	static uint32_t last_time=0;
 
//	assert (tfrc!=NULL);
	//Eric, ************************* Begin of Xrecv_smooth *****************************
	static int 	xx=-1;				
	uint32_t 	sum_packet = 0, period = 0;		
	int 		i=0;					

	xx = ( xx + 1 ) % TFRC_XRECV_SMOOTH;
	
	tfrc->xrecv_number[xx] = tfrc->num_pckts;
	period = (now - tfrc->sent_srx);
	tfrc->xrecv_rtt[xx] = period;	

	sum_packet = 0;
	period = 0;
	for(i=0; i<TFRC_XRECV_SMOOTH; i++) {
		TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK4, "(%3d/%7d)", tfrc->xrecv_number[i], tfrc->xrecv_rtt[i]);
		sum_packet += tfrc->xrecv_number[i];
		period += tfrc->xrecv_rtt[i];
	}
	tfrc->num_pckts = sum_packet;
	//Eric, ************************* End of Xrecv_smooth *****************************
	
	my_xrecv = tfrc->num_pckts*tfrc->pckt_size;	
	my_xrecv = my_xrecv / (period/1000000.0) ;	//Eric
	my_tdelay = now - tfrc->last_pckt_t;
	//Eric, debug message
	TL_DEBUG_MSG( TL_TFRC_RCVR, DBG_MASK3, " (ts:%10u), %5d*%5d/%5d(us) = %6d kbps, p: %8.6f ", tfrc->lsend_ts, tfrc->num_pckts, tfrc->pckt_size, period, my_xrecv*8/1000, tfrc->p );
	
	if(last_time == 0)
		last_time = now;
	if(now - last_time > 1000000) {
		//printf(" \n [TFRC_Rcvr] receving rate = %6d kbps ", my_xrecv*8/1000 );
	}

	*tdelay = my_tdelay;
	*send_ts = tfrc->lsend_ts;
	*x_recv = my_xrecv;
	*p = tfrc->p * 4294967295u;

	tfrc->num_pckts = 0;
	tfrc->sent_srx = now;
	tfrc->x_recv=my_xrecv;
}


uint32_t         
tfrc_process_feedback (struct tfrc_snd *tfrc, struct timeval tnow,
				 uint32_t tdelay, uint32_t x_recv, uint32_t send_ts, uint32_t p) 
{
 	uint32_t rtt,
		 now,
		 prev_rtt;
	//unsigned long temp1;
	//int a;
//	assert (tfrc!=NULL);
	
	//Eric, debug message
	TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, "\n[TFRC_Sndr,  FB] (ts:%10u) ", send_ts);
	
	now = timeval_to_usec (tnow);
	/* compute the RTT */
	if(now - send_ts < tdelay)
		rtt = 10;
	else
		rtt =  now - send_ts - tdelay;
	
	prev_rtt = tfrc->avg_rtt;
	if (tfrc->new_rtt==2) {
		/* no feedback has been received before */
		tfrc->avg_rtt = rtt;
		tfrc->new_rtt=1;
	}
	else
		tfrc->avg_rtt = (0.9)*tfrc->avg_rtt + (1-0.9)*rtt;

	if (abs (prev_rtt - tfrc->avg_rtt) >= 10)  {
		/* convey this new avg to the receiver*/
		tfrc->new_rtt=1;
	}

	tfrc->x_recv = x_recv;
	tfrc->p = p;

	update_sending_rate (tfrc, now);

//	assert (tfrc->x_send!=0);
	
    //add by Ian Tuan
	if(tfrc->x_send == 0)
	{
		//Eric, define the shortest ipi = 35ms
		//tfrc->ipi = 35000;
		//tfrc->ipi = 1000000;
		tfrc->ipi = tfrc->max_ipi;
	}
	else
	{
		tfrc->ipi = (uint32_t)((tfrc->pckt_size*1000000.0)/tfrc->x_send + 0.5);  //uint: ipi(us), pckt_size(byte), x_send(byte/sec)
		//Eric, define the shortest ipi = 35ms
		//if(tfrc->ipi > 35000)
		//	tfrc->ipi = 35000;		
		//if(tfrc->ipi > 1000000)
		//	tfrc->ipi = 1000000;
		if(tfrc->ipi > tfrc->max_ipi)
			tfrc->ipi = tfrc->max_ipi;
	}

	//Eric, debug message
	TL_DEBUG_MSG( TL_TFRC_SNDR, DBG_MASK3, " %6d, %6d kbps | %8.6f new_rtt(%d), rtt:%7d, %7d ipi:%5d  ", 
			(x_recv*8)/1000, 
			(tfrc->x_send*8)/1000, 
			tfrc->p/4294967295.0, tfrc->new_rtt, rtt, tfrc->avg_rtt, tfrc->ipi);
	
	/* reset the feedback timer */
        tv_add_usec (&tnow, MY_MAX (4*tfrc->avg_rtt, 4*tfrc->ipi));
        tfrc->nofeedbk_timer.tv_sec = tnow.tv_sec;
        tfrc->nofeedbk_timer.tv_usec = tnow.tv_usec;

    return rtt;

}

struct tfrc_rcv *
tfrc_rcvr_init(struct timeval tnow)
{
	int i;
	struct tfrc_rcv *tfrc;

	tfrc= (struct tfrc_rcv *) malloc (sizeof (struct tfrc_rcv));
	if (NULL == tfrc)
	{	
		return NULL;
	}
	
	tfrc->p        = 1.0;
	tfrc->lsend_ts = 0;
	tfrc->x_recv   = 0;
	tfrc->rcv_rtt  = TFRC_RTT_INIT;
	tfrc->pckt_size= TFRC_PSIZE_INIT;
	tfrc->sent_srx = get_local_time (); 
	tfrc->last_pckt_t = 0;
	tfrc->num_pckts = 0;

        tfrc->feedbk_timer.tv_sec  = tnow.tv_sec;
        tfrc->feedbk_timer.tv_usec = tnow.tv_usec;
	tv_add (&(tfrc->feedbk_timer), tfrc->rcv_rtt);

	tfrc->last_seq = 0;
        tfrc->ext_last_seq= 0;
        tfrc->last_seq_jj=0;
	tfrc->interval_count = 0;
	tfrc->W_tot     = 0;
	tfrc->weight[0] = 0.2;
	tfrc->weight[1] = 0.4;
	tfrc->weight[2] = 0.6;
	tfrc->weight[3] = 0.8;
	tfrc->weight[4] = 1.0;
	tfrc->weight[5] = 1.0;
	tfrc->weight[6] = 1.0;
	tfrc->weight[7] = 1.0;
	tfrc->weight[8] = 1.0;
        tfrc->ii       = -1;
        tfrc->jj       = -1;
	tfrc->ooo         = 0;
	tfrc->cycles      = 0;
        tfrc->loss_count  = 0;
	tfrc->total_pckts = 0;

	for (i = 0; i < TFRC_NUM_INTERVALS; i++) {
		tfrc->W_tot = tfrc->W_tot + tfrc->weight[i];
	}
	for (i = 0; i < TFRC_MAX_HISTORY; i++) {
		tfrc->arrival[i].seq = 0;
		tfrc->arrival[i].ts  = 0;
		tfrc->loss[i].seq = 0;
		tfrc->loss[i].ts  = 0;
	}
	//Eric,
	for (i = 0; i<10; i++) {
		tfrc->xrecv_number[i] = 0;
		tfrc->xrecv_rtt[i] = 0;
	}
	
	tfrc->magic = TFRC_MAGIC;
	return (tfrc);
}

struct tfrc_snd *
tfrc_sndr_init (struct timeval tnow)
{
	struct tfrc_snd *tfrc;
	uint32_t  now = timeval_to_usec (tnow);

	tfrc= (struct tfrc_snd *) malloc (sizeof (struct tfrc_snd));
	if (NULL == tfrc)
	{	
		return NULL;
	}
	
        tfrc->new_rtt            = 2;                /* avg_rtt is set to TFRC_RTT_INIT-change it ASAP! */
        tfrc->avg_rtt            = TFRC_RTT_INIT;                                         /*    1 sec   */
        tfrc->pckt_size      	 = TFRC_PSIZE_INIT;               /* 1500 bytes - just so we dont use 0 */
        tfrc->x_send             = TFRC_INIT_PCKTS_PER_RTT*TFRC_PSIZE_INIT;           /* 6000 bytes/sec */
        tfrc->ipi                = TFRC_RTT_INIT/TFRC_INIT_PCKTS_PER_RTT; /* start with 4 pckts per RTT */
		tfrc->max_ipi			 = 112000;			/* send each packet at least 112ms, its sending rate is 100kbps */
	tfrc->p			 = 0;
	tfrc->x_recv             = 0;
	tfrc->x_calc             = 0;
	tfrc->tld 		 = now;
 	tfrc->send_time	 	 = now; 
	tfrc->nofeedbk_timer.tv_sec  = tnow.tv_sec;
	tfrc->nofeedbk_timer.tv_usec = tnow.tv_usec;
	tv_add (&(tfrc->nofeedbk_timer), 2*TFRC_RTT_INIT);			  /* 2 sec per RFC 3448 */
	tfrc->magic = TFRC_MAGIC;
	return (tfrc);
}

void tfrc_sndr_release(struct tfrc_snd *tfrc)
{
	free(tfrc);
}

void tfrc_rcvr_release(struct tfrc_rcv *tfrc)
{
	free(tfrc);
}
void tfrc_set_max_ipi(struct tfrc_snd *tfrc, uint32_t min_xsend)
{
	if (tfrc==NULL)
		return ;
		
	tfrc->max_ipi = (uint32_t)((tfrc->pckt_size*8000.0)/min_xsend + 0.5);		/* uint conversion: 1000000/(1000/8) = 8000 */
}
