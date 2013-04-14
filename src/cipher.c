#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include "cipher.h"

#define DEBUG 1

// ===========================================================================
// default cipher & hash method functions

struct cipher_method_ cipher_methods[] = {
	{0,"RAUNCHY", "least 2 bits of each pixel", 4, 0xFC},
	{1,"SNAPPY","least 1 bit of each pixel", 8, 0xFE},
	{2,"TANTRIC","least 4 bits of each pixel", 2, 0xF0},
	{3,"ULTRA","bits 0 and 2 of each pixel", 4, 0xFA},
	{4,"WIGGLY","bits 1 and 2 of each pixel", 4, 0xF9},
	{5,"SHIPPY","use ALL the bits!", 1, 0x00},
	{CHAR_MAX, "", "", 0, 0},
};

struct hash_method_ hash_methods[] = {
	{0, 32, "md5sum", "MD5"},
	{1, 40, "sha1sum", "SHA1"},
	{2, 56, "sha224sum", "SHA224"},
	{3, 64, "sha256sum", "SHA256"},
	{4, 96, "sha384sum", "SHA384"},
	{5, 128, "sha512sum", "SHA512"},
	{UINT_MAX, 0, "", ""},
};

// ============================================================================
// Sparsing bytes into image data

long int get_max_message_length(struct cipher_method_ m, long int data_size) {
	return data_size / m.ratio;
}

int message_fits(struct bitmap_image_ *image, struct wrapped_message_ *msg, struct cipher_method_ m) {
	long int max_length = get_max_message_length(m, image->data_size);

	return msg->msg_length <= max_length;
}

int is_valid_cipher_method(int cipher_type) {
	int i;

	for (i = 0; cipher_methods[i].id != CHAR_MAX; i++) {
		if (cipher_methods[i].id == cipher_type)
			return 1;
	}

	return 0;
}

// ============================================================================
// Hashing methods and functions
#define TEMP_FILENAME_IN "pure_vitamin_c"
#define TEMP_FILENAME_OUT "tis_but_a_temp"

char *hash_message(struct hash_method_ hm, char *message) {
	char cmd[100], *result;
	int fd = -1, ret = -1;
	size_t length = strlen(message);
	ssize_t rw_bytes = 0;

	// write the message to a temporary file
	fd = open(TEMP_FILENAME_IN, O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		fprintf(stderr, "Unable to open temporary file %s\n", TEMP_FILENAME_IN);
		goto out_fail;
	}

	rw_bytes = write(fd, message, length);
	if (rw_bytes == -1) {
		fprintf(stderr, "Unable to write to temporary file %s\n", TEMP_FILENAME_IN);
		close(fd);
		goto out_fail;
	}

	close(fd);

	// 2. prepare and run the command in a shell
	snprintf(cmd, 100, "%s %s > %s", hm.cmd, TEMP_FILENAME_IN, TEMP_FILENAME_OUT);

	ret = system((const char *) cmd);
	if (ret == -1) {
		fprintf(stderr, "Execution of system command failed\n");
		goto out_fail;
	}

	// 3. read the resulting hash from the same temporary file
	fd = open(TEMP_FILENAME_OUT, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Unable to open the temporary file for reading the hash\n");
		goto out_fail;
	}

	result = malloc(hm.hash_length);
	if (result == NULL) {
		fprintf(stderr, "Unable to alloc memory for hashed string\n");
		goto out_fail;
	}

	rw_bytes = read(fd, result, hm.hash_length);
	if (rw_bytes == -1) {
		fprintf(stderr, "Unable to read hash from temporary file\n");
		goto out_free_result;
	}

	close(fd);
	unlink(TEMP_FILENAME_IN); // TODO check return code
	unlink(TEMP_FILENAME_OUT); // TODO check return code

	return result;

out_free_result:
	free(result);
out_fail:
	return NULL;
}

char *wrap_message(struct hash_method_ hm, char *message) {
	struct wrapped_message_ *result = NULL;
	size_t msg_length = strlen(message);
	char *hash = NULL;

	if (msg_length == 0) {
		fprintf(stderr, "Unable to wrap message of length 0\n");
		goto out_fail;
	}

	result = malloc(sizeof(struct wrapped_message_) + msg_length + hm.hash_length);
	if (result == NULL) {
		fprintf(stderr, "Unable to alloc memory for the wrapped message\n");
		goto out_fail;
	}

	hash = hash_message(hm, message);
	if (hash == NULL) {
		fprintf(stderr, "Unable to hash message\n");
		goto out_fail_hash;
	}

	result->hash_id = hm.id;
	result->msg_length = sizeof(struct wrapped_message_) + msg_length + hm.hash_length;
	strncpy(result->buffer, message, msg_length);
	strncpy(result->buffer + msg_length, hash, hm.hash_length);

	return (char *)result;

out_fail_hash:
	free(result);
out_fail:
	return NULL;
}

void print_wrapped_message(struct wrapped_message_ *msg) {
	unsigned int i;
	unsigned int length;

	if (msg == NULL) {
		fprintf(stderr, "Unable to print NULL message structure");
		return;
	}

	length = msg->msg_length - sizeof(struct wrapped_message_) - hash_methods[msg->hash_id].hash_length;

	printf("Wrapped message:\n");
	printf("Hash type: %s\n", hash_methods[msg->hash_id].name);
	printf("Total message length: %u\n", msg->msg_length);

	printf("Message: ");
	for (i = 0; i < length; i ++) {
		printf("%c", msg->buffer[i]);
	}
	printf("\n");

	printf("Hash: ");
	for (i = length; i < length + hash_methods[msg->hash_id].hash_length; i ++) {
		printf("%c", msg->buffer[i]);
	}
	printf("\n");
}

int is_valid_hash_method(int hash_type) {
	int i;

	for (i = 0; hash_methods[i].id != UINT_MAX; i++) {
		if (hash_methods[i].id == hash_type)
			return 1;
	}

	return 0;
}

// ============================================================================
// Methods that bring it all together

char split_byte(char mask, char *byte) {
	char result = 0;
	int i;

	for (i = 0; i < 8; i ++) {
		if (mask & (1 << i))
			continue;

		if (*byte & 1)
			result += 1 << i;

		*byte = *byte >> 1;
	}

	return result;
}

void merge_byte(char mask, char byte, char *res) {
	int i;

	for (i = 7; i >= 0; i--) {
		if (mask & (1 << i))
			continue;

		*res = *res << 1;

		if (byte & (1 << i))
			*res = *res + 1;
	}
}

void hide_byte(char *buf, char byte, struct cipher_method_ m) {
	int i;

	for (i = m.ratio - 1; i >= 0; i--) {
		buf[i] &= m.mask;

		//if (byte != 0)
		buf[i] += split_byte(m.mask, &byte);
	}
}

char reveal_byte(char *buf, struct cipher_method_ m) {
	char result = 0;
	int i;

	for (i = 0; i < m.ratio; i++) {
		merge_byte(m.mask, buf[i], &result);
		printf("Original: %x\n", buf[i]);
	}

	return result;
}

int zeroize_image(struct bitmap_image_ *image, struct cipher_method_ m) {
	long int i;

	for (i = 0; i + m.ratio < image->data_size; i+=m.ratio)
		hide_byte(image->data + i, 0, m);

	return 0;
}

int hide_message_in_image(struct bitmap_image_ *image, struct wrapped_message_ *msg, struct cipher_method_ m) {
	unsigned int length = msg->msg_length;
	unsigned int i;
	char *buf = (char*) msg;

	for (i = 0; i < length; i ++)
		hide_byte(image->data + i*m.ratio, buf[i], m);

	return 0;
}

char *recover_buffer_from_image(struct bitmap_image_ *image, int scatter_id) {
	int length = sizeof(unsigned int) + sizeof(unsigned int);
	char header[length];
	struct cipher_method_ cm = cipher_methods[scatter_id];
	int i;

	for (i = 0; i < length; i++)
		header[i] = reveal_byte(image->data + i*cm.ratio, cm);

	if (DEBUG) {
		printf("Read message header:\n");
		printf("Hash id: %u %d\n", *(unsigned int *)header, length);
		printf("Message length: %u\n", *(unsigned int *)(header + sizeof(unsigned int)));

		for (i = 0; i < 8; i ++)
			printf("%x ",header[i]);
		printf("\n");
	}

	return NULL;
}

// The message that this function returns must be valid - so that the signature corresponds with the content
struct wrapped_message_* recover_message_from_image(struct bitmap_image_ *image, int scatter_id, int hash_id) {
	if (scatter_id != CHAR_MAX && hash_id != UINT_MAX)
		recover_buffer_from_image(image, scatter_id);

	return NULL;
}
