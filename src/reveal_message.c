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

#define DEBUG 1

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

struct arguments_ *prepare_arguments(int argc, char *argv[]) {
	struct arguments_ *args = NULL;

	if (argc < 3 || argv[argc - 1][0] == '-' || argv[argc - 2][0] == '-') {
		print_usage(argv[0]);
		return NULL;
	}

	args = parse_arguments(argc, argv);
	if (args == NULL) {
		print_usage(argv[0]);
		return NULL;
	}

	if (DEBUG)
		print_arguments(args);

	return args;
}

struct bitmap_image_ *get_image(char *image_file) {
	struct bitmap_image_ *result = NULL;
	int fd;

	fd = open(image_file, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "Unable to open image file\n");
		goto out_fail;
	}

	result = read_bitmap_image(fd);
	if (result == NULL) {
		fprintf(stderr, "Unable to read and parse bitmap image\n");
		close(fd);
		goto out_fail;
	}

	return result;

out_fail:
	return NULL;
}

struct bitmap_image_ *prepare_image(struct arguments_ *args) {
	struct bitmap_image_ *image = NULL;

	image = get_image(args->image);
	if (image == NULL) {
		fprintf(stderr, "Unable to read the image structure\n");
		return NULL;
	}

	if (DEBUG) {
		printf("Image read successfully\n");
		print_bitmap_file_header(image->bfh);
		print_dib_header(image->bih);
	}

	return image;
}



int main(int argc, char *argv[]) {
	int ret = 0;
	struct arguments_ *args = NULL;
	struct bitmap_image_ *image = NULL;
	struct wrapped_message_ *message = NULL;

	// TODO remove duplicated code by adding an util.{h,c} class
	// prepare the needed structures
	if ((args = prepare_arguments(argc, argv)) == NULL) {
		fprintf(stderr, "Unable to prepare arguments\n");
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

	if (args->scatter_id != CHAR_MAX && is_valid_cipher_method(args->scatter_id) == 0) {
		fprintf(stderr, "Unsupported cipher method\n");
		ret = 1;
		goto out;
	}

	if ((message = recover_message_from_image(image, args->scatter_id, args->hash_id)) == NULL) {
	}

out:
	if (image != NULL)
		free_bitmap_image(image);
	if (args != NULL)
		free_arguments(args);
	return ret;
}
