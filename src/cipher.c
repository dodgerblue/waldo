#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cipher.h"

// ============================================================================
// Sparsing bytes into image data

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
	// TODO
}

char my_reveal_func(char *buf) {
	// TODO
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


// ============================================================================
// Hashing methods and functions
#define TEMP_FILENAME "pure_vitamin_c"

char *hash_message(struct hash_method_ hm, char *message) {
	char cmd[100], *result;
	int fd = -1, ret = -1;
	size_t length = strlen(message);
	ssize_t rw_bytes = 0;

	// write the message to a temporary file
	fd = open(TEMP_FILENAME, O_RDWR | O_CREAT | O_TRUNC);
	if (fd == -1) {
		fprintf(stderr, "Unable to open temporary file %s\n", TEMP_FILENAME);
		goto out_fail;
	}

	rw_bytes = write(fd, message, length);
	if (rw_bytes == -1) {
		fprintf(stderr, "Unable to write to temporary file %s\n", TEMP_FILENAME);
		close(fd);
		goto out_fail;
	}

	close(fd);

	// 2. prepare and run the command in a shell
	snprintf(cmd, 100, "%s %s > %s", hm.cmd, TEMP_FILENAME, TEMP_FILENAME);

	ret = system((const char *) cmd);
	if (ret == -1) {
		fprintf(stderr, "Execution of system command failed\n");
		goto out_fail;
	}

	// 3. read the resulting hash from the same temporary file
	fd = open(TEMP_FILENAME, O_RDONLY);
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
	unlink(TEMP_FILENAME); // TODO check return code

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

struct hash_method_ hash_methods[] = {
	{0, 32, "md5sum", "MD5"},
	{1, 64, "sha512sum", "SHA512"},
};

void print_wrapped_message(struct wrapped_message_ *msg) {
	unsigned int i;
	unsigned int length = msg->msg_length - sizeof(struct wrapped_message_) - hash_methods[msg->hash_id].hash_length;

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

// ============================================================================
