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

extern int debug;

struct arguments_ *prepare_arguments(int argc, char *argv[]) {
	struct arguments_ *args = NULL;

	if (argc < 3 || argv[argc - 1][0] == '-' || argv[argc - 2][0] == '-') {
		return NULL;
	}

	args = parse_arguments(argc, argv);
	if (args == NULL) {
		return NULL;
	}

	if (debug)
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

	if (debug) {
		printf("Image read successfully\n");
		print_bitmap_file_header(image->bfh);
		print_dib_header(image->bih);
	}

	return image;
}
