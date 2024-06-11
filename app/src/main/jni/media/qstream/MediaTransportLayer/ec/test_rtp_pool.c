#include "rtp_pool.h"
#include "rtp_packet.h"
#include <stdio.h>

int main()
{
	rtp_pool_t *pool = rtp_pool_create(5);
	
	int i=0;
	for (i=0; i<10; i++) {
		pool->insert_rtp(pool, rtp_packet_create(0, 0, 0, 0, 0, 100, 5+i, 10032+i*8, 9999, (unsigned char*)"012345", 6));
	}

#if 0
	// check circular
	for (i=0; i<10; i++) {
		rtp_packet_t *p = pool->get_rtp(pool, 5+i);

		if (p!= NULL)
			p->print(p, NULL);
		else
			printf("NULL\n");
	}
#endif


	rtp_packet_t *p = pool->remove_rtp(pool, 10);
	if (p!=NULL) {
		p->print(p, NULL);
		rtp_packet_destroy(p);
	}

	for (i=0; i<5; i++) {
		rtp_packet_t *p = pool->get_rtp(pool, 5+i);

		if (p!= NULL)
			p->print(p, NULL);
		else
			printf("NULL\n");
	}

	rtp_pool_destroy(pool);
	
	return 0;
}
