#include "ec.h"
#include "parity.h"
#include "fec_packet.h"
#include "rtp_packet.h"
#include "fec_sender.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

struct woo_t {
	ec_t *ec;
};

#if 1
static void print_rtp_payload(unsigned char *payload, unsigned int payload_size)
{
//	parity_print_data(payload, payload_size);
}



static int sender_rtp_out(rtp_packet_t *p, void *context, unsigned int context_size)
{
//	printf("[sender_rtp_out]\n");
//	p->print(p, fec_packet_print_net_stream);
//	printf("\n");

	rtp_packet_destroy(p);

	return 0;
}

void test_sender()
{
	// test sender
	ec_t* ec = ec_create(1);
	ec->sender_register_rtp_out(ec, sender_rtp_out, NULL, 0);

	unsigned int i;
	for (i=0; i<5; i++) {
		rtp_packet_t *p = rtp_packet_create(0, 0, 0, 0, 0, 100, 5+i, 10032+i*8, 9999, (unsigned char*)"012345", 6);
		printf("[sender_rtp_in]\n");
		p->print(p, print_rtp_payload);
		printf("\n");
		ec->sender_rtp_in(ec, p);
	}

	ec_destroy(ec);
}

void test_receiver()
{
	ec_t *ec = ec_create(1);
	//ec->sender_register_rtp_out(ec, 

	// generate rtp 1, 2, 3, 4, 5
	// insert rtp 1, 2, 3, 5
	// receiver receive 1, 2, 3, 5, & ec
	// recover 4 and check whether it is same

	ec_destroy(ec);
}

#endif




void test_all()
{
	// test sender
	ec_t* ec = ec_create(ENABLE_FEC);
	//ec_t* ec = ec_create(DISABLE_FEC);
	ec->sender_set_fec_bandwidth_ratio_upper_bound(ec, 0.2);
	ec->sender_set_target_packet_loss_rate(ec, 0.01);
	ec->sender_update_current_packet_loss_rate(ec, 0.1);

	struct woo_t *woo = (struct woo_t*)malloc(sizeof(struct woo_t));
	woo->ec = ec;
	ec->sender_register_rtp_out(ec, sender_rtp_out, woo, sizeof(struct woo_t*));
	free(woo);

	unsigned int i;
	for (i=0; i<10000; i++) {
//		usleep(5000);
		rtp_packet_t *p = NULL;
		rtp_packet_t *p2 = NULL;
		
		unsigned char payload[1000];

		if (i == 0) {
			payload[0] = 0;
			payload[1] = 1;
			payload[2] = 2;
			payload[3] = 3;
			payload[4] = 4;
			payload[5] = 5;
			p = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
			p2 = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
		}
		else if (i==1) {
			payload[0] = 0;
			payload[1] = 1;
			payload[2] = 1;
			payload[3] = 3;
			p = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
			p2 = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
		}
		else {
			payload[0] = 0;
			payload[1] = 1;
			payload[2] = 1;
			payload[3] = 3;
			payload[4] = 6;
			p = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
			p2 = rtp_packet_create(0, 1, 0, 0, 0, 100, 45005+i, 1032+i*8, 9999, payload, 1000);
		}

		if (i != 2) {
			//printf("[receiver_rtp_in]\n");
			ec->receiver_rtp_in(ec, p2);
		}
		else {
			rtp_packet_destroy(p2);
		}

		//printf("[sender_rtp_in]\n");
		//p->print(p, print_rtp_payload);
		//printf("\n");

		ec->sender_rtp_in(ec, p);
	}

	//printf("is time out? %d (should be 0)\n", ec->receiver_is_time_out(ec, 1, 2, 1000));
	//printf("is time out? %d (should be 1)\n", ec->receiver_is_time_out(ec, 10, 50, 150));
	printf("is request re transmit? %d (should be 1)\n", ec->receiver_is_request_re_transmit(ec, 45005 + 42, 0));
	printf("is request re transmit? %d (should be 1)\n", ec->receiver_is_request_re_transmit(ec, 45005 + 49, 0));
	printf("is request re transmit? %d (should be 1)\n", ec->receiver_is_request_re_transmit(ec, 45005 + 49, 100));
	printf("is re transmit? %d (should be 0)\n", ec->sender_is_re_transmit(ec, 45005 + 0));
	printf("is re transmit? %d (should be 0)\n", ec->sender_is_re_transmit(ec, 45005 + 49));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 51));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 52));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 53));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 54));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 55));
	printf("is re transmit? %d (should be 1)\n", ec->sender_is_re_transmit(ec, 45005 + 56));
	printf("is re transmit? %d (should be 0)\n", ec->sender_is_re_transmit(ec, 45005 + 100));
	//rtp_packet_t *p = ec->receiver_get_rtp(ec, 45005+2);
	//if (p) {
	//	printf("receiver recover rtp with seq %u (success)\n", 45005+2);
		//printf("%p \n", p);
		//p->print(p, print_rtp_payload);
		//printf("\n");
	//}
	//else 
//		printf("receiver recover rtp with seq %u (fail)\n", 45005+2);

	ec_destroy(ec);
}

bool test_setup(double ratio, double current_p, double target_p, unsigned int expect_rtp_count)
{
	ec_t* ec = ec_create(ENABLE_FEC);
//	ec_t* ec = ec_create(DISABLE_FEC);

	ec->sender_set_fec_bandwidth_ratio_upper_bound(ec, ratio);
	ec->sender_set_target_packet_loss_rate(ec, target_p);
	ec->sender_update_current_packet_loss_rate(ec, current_p);
	unsigned int involved_rtp_count = ec->sender_get_involved_rtp_count(ec);

	printf("ratio = %.4lf, ", ratio);
	printf("cur p = %.4lf, ", current_p);
	printf("tar p = %.4lf, ", target_p);
	printf("involved rtp count = %u (should be %u) ", involved_rtp_count, expect_rtp_count);

	ec_destroy(ec);

	return expect_rtp_count == involved_rtp_count;
}

void test_setup_all()
{
	if (test_setup(0.2, 0.05, 0.0113, 5)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.05, 0.0133, 6)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.05, 0.0152, 7)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.05, 0.0169, 8)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.05, 0.0186, 9)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.05, 0.0202, 10)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	
	if (test_setup(0.2, 0.01, 0.01, 5)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.2, 0.005, 0.0001, 5)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}

	if (test_setup(0.0, 0.00, 0.0, INVOLVED_RTP_COUNT_MAX)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
	if (test_setup(0.1, 0.00, 0.0, INVOLVED_RTP_COUNT_MAX)) printf("SUCCESS!!!!\n"); else {printf("FAIL\n"); exit(0);}
}

int main()
{
	//test_sender();
	//test_receiver();
	int i=0;
	for (i=0; i<1; i++) {
		printf("%d:\n", i);
		test_all();
	}
	test_setup_all();
	return 0;
}
