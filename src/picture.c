#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "picture.h"

void print_bitmap_file_header(struct bitmap_file_header_ bfh) {
	printf("Read bitmap file header:\n \
	Header field: %c%c\n \
	Image size: %ld\n \
	Bitmap image offset: %ld\n",
	bfh.fields.header_field[0], bfh.fields.header_field[1], *(long int*) bfh.fields.size, *(long int*) bfh.fields.offset);
}

struct bitmap_file_header_ read_bitmap_file_header(int fd) {
	struct bitmap_file_header_ result;
	ssize_t read_bytes;

	memset(&result, 0, sizeof(struct bitmap_file_header_));

	read_bytes = read(fd, result.buffer, sizeof(struct bitmap_file_header_));
	if (read_bytes == -1) {
		perror("Unable to read image header file\n");
		exit(1);
	}

	if (strncmp(result.fields.header_field, BMP_IDENTIFIER, strlen(BMP_IDENTIFIER))) {
		fprintf (stderr, "Unknowm bitmap file header identifier %s\n", result.fields.header_field);
		exit(1);
	}

	print_bitmap_file_header(result);

	return result;
}

void print_dib_header(struct bitmap_info_header_ dh) {
	printf("Read DIB header:\n \
	Header size: %ld\n \
	Image width: %ld\n \
	Image height: %ld\n \
	Color planes: %d\n \
	Color depth: %d\n \
	Compression: %ld\n \
	Data size: %ld\n \
	Horizontal rule: %ld\n \
	Vertical resolution: %ld\n \
	No. of colors used: %ld\n \
	No. of VIP colors: %ld\n",
	*(long int*) dh.size, *(long int*) dh.fields.width, *(long int*) dh.fields.height,
	*(short int*) dh.fields.planes, *(short int*) dh.fields.depth,
	*(long int*) dh.fields.compression, *(long int*) dh.fields.data_size,
	*(long int*) dh.fields.horizontal_rule, *(long int*) dh.fields.vertical_resolution,
	*(long int*) dh.fields.colors, *(long int*) dh.fields.vip_colors);
}

void validate_image(struct bitmap_info_header_ bih) {
	if (*(long int*) bih.size != DIB_HEADER_SIZE) {
		fprintf (stderr, "Unsupported DIB header type (size %ld) - currently only %s is supported\n",
			*(long int*)bih.size, DIB_HEADER_TYPE);
		exit(1);
	}


}

struct bitmap_info_header_ read_dib_header(int fd) {
	struct bitmap_info_header_ result;
	ssize_t read_bytes;

	memset(&result, 0, sizeof(struct bitmap_info_header_));

	read_bytes = read(fd, &result, sizeof(struct bitmap_info_header_));
	if (read_bytes == -1) {
		perror("Unable to read image header file\n");
		exit(1);
	}

	print_dib_header(result);

	validate_image(result);

	return result;
}

struct bitmap_image_ read_bitmap_image(int fd) {
	struct bitmap_image_ result;

	memset(&result, 0, sizeof(struct bitmap_image_));

	result.bfh = read_bitmap_file_header(fd);
	result.bih = read_dib_header(fd);

	return result;
}
