#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "picture.h"
#include "cipher.h"
#include "arguments.h"
#include "util.h"

int debug = 1;

extern struct hash_method_ hash_methods[];
extern struct cipher_method_ cipher_methods[];

void print_usage(char *program_name) {
	int i;

	fprintf(stderr, "Usage: %s [OPTIONS] <image> <filename>\n\n \
	image - the image file with the hidden message\n \
	filename - the file to store the read message to\n \
	\n \
	OPTIONS can be one or more of the following: \n \
	-h <hash method> - use this specific method for verifying the hash\n \
	-s <scatter method> - use this specific method to recover the message\n \
	-f - instead of storing the message into a file, output it to the console (the filename parameter is ignored)\n \
	\n",
	program_name);

	fprintf(stderr, "\tAvailable hash methods:\n");
	for (i = 0; hash_methods[i].id != UINT_MAX; i ++) {
		fprintf(stderr, "\t%u - %s\n", hash_methods[i].id, hash_methods[i].name);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "\tAvailable scatter methods:\n");
	for (i = 0; cipher_methods[i].id != CHAR_MAX; i ++) {
		fprintf(stderr, "\t%u - %s - %s\n", cipher_methods[i].id, cipher_methods[i].codename, cipher_methods[i].description);
	}
	fprintf(stderr, "\n");
}

int validate_message_all(struct wrapped_message_ *msg, int hash_id) {
	int i;

	if (hash_id != UINT_MAX)
		return validate_message(msg, hash_methods[hash_id]);

	if (is_valid_hash_method(msg->hash_id))
		return validate_message(msg, hash_methods[msg->hash_id]);

	for (i = 0; hash_methods[i].id != UINT_MAX; i++) {
		if (validate_message(msg, hash_methods[i]))
			return 1;
	}

	return 0;
}

struct wrapped_message_* reveal_and_validate_message(struct bitmap_image_ *image, struct cipher_method_ cm, int hash_id) {
	struct wrapped_message_ *result = NULL;

	if (debug)
		printf("Trying scatter method: %s - %s\n", cm.codename, cm.description);

	result = recover_message_from_image(image, cm);

	if (result == NULL) {
		fprintf(stderr, "Unable to recover the wrapped message from the image\n");
		goto out_fail;
	}

	if (validate_message_all(result, hash_id) == 0) {
		fprintf(stderr, "Unable to validate the recovered message\n");
		free(result);
		goto out_fail;
	}

	return result;

out_fail:
	return NULL;
}

struct wrapped_message_* reveal_message_from_image(struct bitmap_image_ *image, int scatter_id, int hash_id) {
	struct wrapped_message_ *result = NULL;
	int i;

	if (debug) {
		if (scatter_id == UINT_MAX) printf("Trying all scatter methods\n");
		else printf("Trying scatter method: %s - %s\n", cipher_methods[scatter_id].codename, cipher_methods[scatter_id].description);

		if (hash_id == UINT_MAX) printf("Trying all hash methods (or the one in the message header, in case it is valid)\n");
		else printf("Trying hash method %s\n", hash_methods[hash_id].name);
	}

	if (scatter_id != UINT_MAX)
		return reveal_and_validate_message(image, cipher_methods[scatter_id], hash_id);

	for (i = 0; cipher_methods[i].id != UINT_MAX; i++)
		if ((result = reveal_and_validate_message(image, cipher_methods[i], hash_id)) != NULL)
			return result;

	return NULL;
}

int write_back_message(struct wrapped_message_ *msg, char *filename, int to_screen) {
	unsigned int msg_length = msg->msg_length - sizeof(struct wrapped_message_) - hash_methods[msg->hash_id].hash_length;
	char *message;
	int ret = 1;
	int fd;

	message = malloc(msg_length + 1);
	if (message == NULL) {
		fprintf(stderr, "Unable to alloc memory for message\n");
		ret = 0;
		goto out;
	}

	memcpy(message, msg->buffer, msg_length);
	message[msg_length] = '\0';

	if (to_screen) {
		printf("\nHIDDEN MESSAGE:\n%s\n", message);
		goto out;
	}

	fd = open(filename, O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		fprintf(stderr, "Unable to open filename to store message\n");
		ret = 1;
		goto out_free_message;
	}

	if (write(fd, message, msg_length + 1) == -1) {
		fprintf(stderr, "Unable to write the message to the file\n");
		goto out_fail_write;
	}

	close(fd);

out_fail_write:
	close(fd);
out_free_message:
	free(message);
out:
	return ret;
}

int main(int argc, char *argv[]) {
	int ret = 0;
	struct arguments_ *args = NULL;
	struct bitmap_image_ *image = NULL;
	struct wrapped_message_ *message = NULL;

	if ((args = prepare_arguments(argc, argv)) == NULL) {
		fprintf(stderr, "Unable to prepare arguments\n");
		print_usage(argv[0]);
		ret = 1;
		goto out;
	}

	if ((image = prepare_image(args)) == NULL) {
		fprintf(stderr, "Unable to prepare image\n");
		ret = 1;
		goto out;
	}

	if (args->hash_id != UINT_MAX && is_valid_hash_method(args->hash_id) == 0) {
		fprintf(stderr, "Unsupported hash method\n");
		ret = 1;
		goto out;
	}

	if (args->scatter_id != UINT_MAX && is_valid_cipher_method(args->scatter_id) == 0) {
		fprintf(stderr, "Unsupported cipher method\n");
		ret = 1;
		goto out;
	}

	if ((message = reveal_message_from_image(image, args->scatter_id, args->hash_id)) == NULL) {
		fprintf(stderr, "Unable to reveal hidden message from image\n");
		ret = 1;
		goto out;
	}

	if (write_back_message(message, args->message, args->msg_from_file) == 0) {
		fprintf(stderr, "Unable to write back the message to the file\n");
		ret = 1;
		goto out;
	}

	return 0;

out:
	if (message != NULL)
		free(message);
	if (image != NULL)
		free_bitmap_image(image);
	if (args != NULL)
		free_arguments(args);
	return ret;
}
