#include <stdio.h>

#include "cipher.h"

long int get_max_message_length(struct cipher_method_ m, long int data_size) {
	return data_size / m.ratio;
}

void test_cipher(struct cipher_method_ m, struct bitmap_image_ *img) {
	long int length = get_max_message_length(m, img->data_size);
	long int i;

	printf("The maximum ammount of characters you can store in this shit is %ld\n", length);

	for (i = 0; i < length * m.ratio; i += m.ratio) {
		m.zf(&img->data[i]);
	}

}

// ===========================================================================
// default cipher method functions
#define MY_CIPHER_COUNT 4
#define MY_CIPHER_MASK 0xFC

void my_hide_func(char *buf, char byte) {
}

char my_reveal_func(char *buf) {
	return 'a';
}

void my_zeroize_func(char *buf) {
	buf[0] &= MY_CIPHER_MASK;
	buf[1] &= MY_CIPHER_MASK;
	buf[2] &= MY_CIPHER_MASK;
	buf[3] &= MY_CIPHER_MASK;
}

struct cipher_method_ my_cipher_method = {
	.ratio = MY_CIPHER_COUNT,
	.hf = my_hide_func,
	.rf = my_reveal_func,
	.zf = my_zeroize_func,
};
