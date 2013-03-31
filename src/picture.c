#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "picture.h"

void print_bitmap_file_header(struct bitmap_file_header bfh) {
	printf("Read bitmap file header:\n \
	Header field: %c%c\n \
	Image size: %d\n \
	Bitmap image offset \n",
	bfh.header_field[0], bfh.header_field[1], bfh.size, bfh.offset);
}

struct bitmap_file_header read_bitmap_file_header(int fd) {
	struct bitmap_file_header result;
	ssize_t read_bytes;

	memset(&result, 0, sizeof(struct bitmap_file_header));

	read_bytes = read(fd, &result, sizeof(struct bitmap_file_header));
	if (read_bytes == -1) {
		perror("Unable to read image header file\n");
		exit(1);
	}

	print_bitmap_file_header(result);

	return result;
}

struct bitmap_image read_bitmap_image(int fd) {
	struct bitmap_image result;

	memset(&result, 0, sizeof(struct bitmap_image));

	result.bmp_header = read_bitmap_file_header(fd);

	return result;
}
