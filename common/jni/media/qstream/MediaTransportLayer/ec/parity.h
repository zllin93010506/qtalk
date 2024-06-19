#ifndef PARITY_H
#define PARITY_H

#define PARITY_SUCCESS 0
#define PARITY_FAIL -1

#include <string.h>

unsigned char parity_xor_unsigned_char(unsigned char src1, unsigned char src2);
unsigned short parity_xor_unsigned_short(unsigned short src1, unsigned short src2);
unsigned int parity_xor_unsigned_int(unsigned int src1, unsigned int src2);
int parity_xor_data(unsigned char* dest, size_t dest_nbytes,
					const unsigned char* src1, size_t src1_nbytes, 
					const unsigned char* src2, size_t src2_nbytes	);
void parity_print_data(const unsigned char* data, size_t nbytes);

#endif
