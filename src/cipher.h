#ifndef CIPHER_H_
#define CIPHER_H_

#include "picture.h"

typedef void (*hide_func)(char *buf, char byte);
typedef char (*reveal_func)(char *buf);
typedef void (*zeroize_func)(char *buf);

struct cipher_method_ {
	int ratio; // no image bits / no info bits

	hide_func hf;
	reveal_func rf;
	zeroize_func zf;
};

long int get_max_message_length(struct cipher_method_ m, long int data_size);

void test_cipher(struct cipher_method_ m, struct bitmap_image_ *img);

#endif
