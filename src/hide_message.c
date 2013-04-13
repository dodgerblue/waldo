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

void print_usage(char *program_name) {
	int i;

	fprintf(stderr, "Usage: %s [OPTIONS] <image> <message>\n\n \
	image - the image file used to hide the message\n \
	message - the message to hide, as a string\n \
	\n \
	OPTIONS can be one or more of the following: \n \
	-h <hash method> - the method using for hashing the message\n \
	-s <scatter method> - the method used for hiding the message\n \
	-f - the message to hide will be read from a file, provided as the 'message' parameter\n \
	-r <suffix> - add custom suffix to the new image (default is '_new.bmp')\n \
	\n",
	program_name);

	fprintf(stderr, "\tAvailable hash methods:\n");
	for (i = 0; hash_methods[i].id != UINT_MAX; i ++) {
		fprintf(stderr, "\t%u - %s\n", hash_methods[i].id, hash_methods[i].name);
	}
}

char* get_message_to_hide(char *message, int is_file) {
	char *result = NULL;
	off_t start, stop;
	int fd;
	ssize_t read_bytes;

	if (is_file == 0) {
		result = strdup(message);
		return result;
	}

	fd = open(message, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Unable to open message file\n");
		goto out_fail;
	}

	stop = lseek(fd, 0, SEEK_END);
	if (stop == -1) {
		fprintf(stderr, "Unable to seek to end of message file\n");
		goto out_seek_fail;
	}

	if (DEBUG)
		printf("File size: %ld\n", (long int) stop);

	start = lseek(fd, 0, SEEK_SET);
	if (start == -1) {
		fprintf(stderr, "Unable to seek to start of message file\n");
		goto out_seek_fail;
	}

	result = malloc(stop);
	if (result == NULL) {
		fprintf(stderr, "Unable to alloc memory to the message\n");
		goto out_seek_fail;
	}

	read_bytes = read(fd, result, (size_t) stop);
	if (read_bytes == -1) {
		fprintf(stderr, "Unable to read the message from the message file\n");
		goto out_read_fail;
	}

	if (read_bytes != (ssize_t) stop) {
		fprintf(stderr, "Did not read enough bytes from the message file\n");
		goto out_read_fail;
	}

	close(fd);

	return result;

out_read_fail:
	free(result);
out_seek_fail:
	close(fd);
out_fail:
	return NULL;
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

int main(int argc, char *argv[]) {
	struct arguments_ *args = NULL;
	char *message = NULL;
	struct bitmap_image_ *image = NULL;

	// prepare the arguments
	if (argc < 3 || argv[argc - 1][0] == '-' || argv[argc - 2][0] == '-') {
		print_usage(argv[0]);
		return 1;
	}

	args = parse_arguments(argc, argv);
	if (args == NULL) {
		print_usage(argv[0]);
		return 1;
	}

	if (DEBUG)
		print_arguments(args);

	// prepare the message to hide
	message = get_message_to_hide(args->message, args->msg_from_file);
	if (message == NULL) {
		fprintf(stderr, "Unable to read message to hide\n");
		return 1;
	}

	if (DEBUG)
		printf("Message to hide: %s\n", message);

	// prepare the image structure
	image = get_image(args->image);
	if (image == NULL) {
		fprintf(stderr, "Unable to read the image structure\n");
		return 1;
	}

	if (DEBUG) {
		printf("Image read successfully\n");
		print_bitmap_file_header(image->bfh);
		print_dib_header(image->bih);
	}

	free_bitmap_image(image);
	free(message);
	free_arguments(args);

	return 0;
}
