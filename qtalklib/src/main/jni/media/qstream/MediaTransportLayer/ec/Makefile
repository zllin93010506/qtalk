all: ec

test: test_ec


parity: parity.c parity.h test_parity.c
	gcc -Wall -o test_parity parity.c parity.h test_parity.c

test_parity: parity
	./test_parity

ec: ec.h ec.c fec_sender.c fec_sender.h test_ec.c parity fec_packet.c fec_packet.h fec_receiver.c fec_receiver.h rtp_pool.c rtp_pool.h fec_pool.c fec_pool.h
	gcc -Wall -o test_ec test_ec.c ec.c rtp_packet.c fec_sender.c parity.c fec_packet.c fec_receiver.c  rtp_pool.c fec_pool.c -lpthread -lm

test_ec: ec
	./test_ec

rtp_pool: rtp_packet.c rtp_packet.h rtp_pool.c rtp_pool.h test_rtp_pool.c
	gcc -Wall -o test_rtp_pool rtp_packet.c rtp_pool.c test_rtp_pool.c

test_rtp_pool: rtp_pool
	valgrind ./test_rtp_pool -v

clean:
	rm -rf test_parity
	rm -rf test_ec

#parity: parity.h parity.c
#	gcc -DTEST_PARITY -o test_parity parity.c
