#ifndef CIPHER_H_
#define CIPHER_H_

#include "picture.h"

// ============================================================================
// Structures for sparsing bytes into image data

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

// ============================================================================
// Structures to compute the hash of the text

#define MD5_ID 1
#define SHA512_ID 2

#define HASH_BUF_LEN 20

typedef char* (*hash_function)(char *buf);

struct hash_method_ {
	unsigned int id;
	unsigned int hash_length;

	char cmd[HASH_BUF_LEN];
	char name[HASH_BUF_LEN];
};

struct wrapped_message_ {
	unsigned int hash_id;
	unsigned int msg_length;
	char buffer[0];
};

char *hash_message(struct hash_method_ hm, char *message);
char *wrap_message(struct hash_method_ hm, char *message);
void print_wrapped_message(struct wrapped_message_ *msg);

#endif
