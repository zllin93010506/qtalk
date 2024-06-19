#include "parity.h"
#include <stdio.h>

int main()
{
	unsigned char seq1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	unsigned char seq2[9] = {1, 3, 5, 7, 9, 2, 4, 6, 8};
	unsigned char seq3[10] = {0};

	parity_xor_data(seq3, 10, seq1, 10, seq2, 9);


	printf("test parity_xor_data:\n");
	parity_print_data(seq1, 10);	
	parity_print_data(seq2, 9);	
	parity_print_data(seq3, 10);	
	
	printf("test parity_xor_unsigned_char:\n");
	printf("1001\n");
	printf("1100\n");
	printf("%d\n", parity_xor_unsigned_char(9, 12));




	return 0;
}
