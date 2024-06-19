#include "parity.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// class function

unsigned char parity_xor_unsigned_char(unsigned char src1, unsigned char src2)
{
	return src1 ^ src2;
}


unsigned short parity_xor_unsigned_short(unsigned short src1, unsigned short src2)
{
	return src1 ^ src2;
}


unsigned int parity_xor_unsigned_int(unsigned int src1, unsigned int src2)
{
	return src1 ^ src2;
}


int parity_xor_data(unsigned char* dest, size_t dest_nbytes,
					const unsigned char* src1, size_t src1_nbytes, 
					const unsigned char* src2, size_t src2_nbytes	)
{

	int i = 0;
	for (i=0; i<dest_nbytes; i++) {
		unsigned char bit1 = i < src1_nbytes ? src1[i] : 0;
		unsigned char bit2 = i < src2_nbytes ? src2[i] : 0;
		dest[i] = bit1 ^ bit2;
	}

	return PARITY_SUCCESS;
}


void parity_print_data(const unsigned char* data, size_t nbytes)
{
	int i = 0;
	int j = 0;
	char c = 0;
	for (i=0; i<nbytes; i++) {
		c = data[i];
		for (j=7; j>=0; j--) {
			if (c & (1<<j)) 
				printf("1");
			else
				printf("0");
		}
		printf(" ");
	}
	
	printf("\n");

}


