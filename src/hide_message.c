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

	fprintf(stderr, "Usage: %s [OPTIONS] <image> <message>\n\n \
	image - the image file used to hide the message\n \
	message - the message to hide, as a string\n \
	\n \
	OPTIONS can be one or more of the following: \n \
	-h <hash method> - the method using for hashing the message\n \
	-s <scatter method> - the method used for hiding the message\n \
	-f - the message to hide will be read from a file, provided as the 'message' parameter\n \
	-z - just zeroize the bits in the new image, w/o adding any info\n \
	-r <suffix> - add custom suffix to the new image (default is '_new.bmp')\n \
	\n",
	program_name);

	fprintf(stderr, "\tAvailable hash methods:\n");
	for (i = 0; hash_methods[i].id != UINT_MAX; i ++) {
		fprintf(stderr, "\t%u - %s\n", hash_methods[i].id, hash_methods[i].name);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "\tAvailable scatter methods:\n");
	for (i = 0; cipher_methods[i].id != UINT_MAX; i ++) {
		fprintf(stderr, "\t%u - %s - %s\n", cipher_methods[i].id, cipher_methods[i].codename, cipher_methods[i].description);
	}
	fprintf(stderr, "\n");
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

	if (args->hash_id == UINT_MAX) args->hash_id = 0;
	if (args->scatter_id == UINT_MAX) args->scatter_id = 0;

	if (DEBUG)
		print_arguments(args);

	return args;
}

char *prepare_message(struct arguments_ *args) {
	char *message = NULL;

	message = get_message_to_hide(args->message, args->msg_from_file);
	if (message == NULL) {
		fprintf(stderr, "Unable to read message to hide\n");
		return NULL;
	}

	if (DEBUG)
		printf("Message to hide: %s\n", message);

	return message;
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

char *prepare_wrapped_message(int hash_type, char *message) {
	char *msg = NULL;

	msg = wrap_message(hash_methods[hash_type], message);
	if (msg == NULL) {
		fprintf(stderr, "Unable to wrap message\n");
		return NULL;
	}

	if (DEBUG)
		print_wrapped_message((struct wrapped_message_*) msg);

	return msg;
}

int write_back_image(struct bitmap_image_ *image, char *filename, char *suffix) {
	char *new_filename = NULL;
	int length = strlen(filename);
	int fd;

	if (suffix != NULL)
		length += strlen(suffix);
	else {
		length += strlen(IMG_SUFFIX);
		suffix = IMG_SUFFIX;
	}

	new_filename = malloc(length);
	if (new_filename == NULL) {
		fprintf(stderr, "Unable to allocate memory for write back image filename\n");
		goto out_fail;
	}

	sprintf(new_filename, "%s%s", filename, suffix);

	fd = open(new_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		fprintf(stderr, "Unable to open new image file for writing\n");
		goto out_fail_open;
	}

	free(new_filename);

	write_bitmap_image(image, fd); // TODO check return code and everything
	close(fd);

	return 0;

out_fail_open:
	free(new_filename);
out_fail:
	return 1;
}

int main(int argc, char *argv[]) {
	struct arguments_ *args = NULL;
	char *message = NULL;
	struct bitmap_image_ *image = NULL;
	char *wrapped_message = NULL;
	int ret = 0;

	// prepare the needed structures
	if ((args = prepare_arguments(argc, argv)) == NULL) {
		fprintf(stderr, "Unable to prepare arguments\n");
		ret = 1;
		goto out;
	}

	if ((message = prepare_message(args)) == NULL) {
		fprintf(stderr, "Unable to prepare message\n");
		ret = 1;
		goto out;
	}

	if ((image = prepare_image(args)) == NULL) {
		fprintf(stderr, "Unable to prepare image\n");
		ret = 1;
		goto out;
	}

	if (args->hash_id != 0 && is_valid_hash_method(args->hash_id) == 0) {
		fprintf(stderr, "Unsupported hash method\n");
		ret = 1;
		goto out;
	}

	if (args->scatter_id != 0 && is_valid_cipher_method(args->scatter_id) == 0) {
		fprintf(stderr, "Unsupported cipher method\n");
		ret = 1;
		goto out;
	}

	// prepare the wrapped message
	if ((wrapped_message = prepare_wrapped_message(args->hash_id, message)) == NULL) {
		fprintf(stderr, "Unable to prepare the wrapped message\n");
		ret = 1;
		goto out;
	}

	// check if the message "fits" in the image
	if (message_fits(image, (struct wrapped_message_ *) wrapped_message, cipher_methods[args->scatter_id]) == 0) {
		fprintf(stderr, "The provided image is too small to fit the message\n");
		ret = 1;
		goto out;
	}

	if (args->just_zeroize) {
		ret = zeroize_image(image, cipher_methods[args->scatter_id]);
		if (ret == 1) goto out;
		goto write_image;
	}

	ret = hide_message_in_image(image, (struct wrapped_message_ *) wrapped_message, cipher_methods[args->scatter_id]);
	if (ret == 1) goto out;

write_image:
	ret = write_back_image(image, args->image, args->suffix);

out:
	if (image != NULL)
		free_bitmap_image(image);
	if (message != NULL)
		free(message);
	if (args != NULL)
		free_arguments(args);

	return ret;
}
