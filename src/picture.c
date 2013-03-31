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
	int error_found = 0;
	int bytes_per_pixel = 0;

	if (*(long int*) bih.size != DIB_HEADER_SIZE) {
		fprintf (stderr, "Unsupported DIB header type (size %ld) - currently only %s is supported\n",
			*(long int*)bih.size, DIB_HEADER_TYPE);
		error_found = 1;
	}

	if (*(short int*) bih.fields.planes != 1) {
		perror("Only 1 color planes are supported\n");
		error_found = 1;
	}

	if (*(short int*) bih.fields.depth != 24 ) {
		perror("Only 24 color bit depth is supported\n");
		error_found = 1;
	}

	if (*(long int*) bih.fields.compression != 0) {
		perror("Compressed images are not supported in current implementation\n");
		error_found = 1;
	}

	bytes_per_pixel = *(short int*) bih.fields.depth / 8;

	if (*(long int*) bih.fields.data_size != *(long int*) bih.fields.width * *(long int*) bih.fields.height * bytes_per_pixel) {
		perror("Data size != width * height * depth / 8\n");
		error_found = 1;
	}

	if (error_found) {
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
	ssize_t read_bytes;

	memset(&result, 0, sizeof(struct bitmap_image_));

	result.bfh = read_bitmap_file_header(fd);
	result.bih = read_dib_header(fd);

	// allocate space for the image bytes
	result.data_size = *(long int*)result.bih.fields.data_size;
	result.data = malloc (result.data_size);
	if (result.data == NULL) {
		perror ("Unable to allocate memory for the image data\n");
		exit(1);
	}

	read_bytes = read(fd, result.data, result.data_size);
	if (read_bytes == -1) {
		perror ("Unable to read the image data bytes\n");
		exit(1);
	}

	// allocate space for the image trailer
	result.trailer_size = *(long int*) result.bfh.fields.size -
		sizeof(struct bitmap_file_header_) -
		sizeof(struct bitmap_info_header_) -
		result.data_size;

	if (result.trailer_size < 0) {
		perror("file size < headers size + data size\n");
		exit(1);
	}

	if (result.trailer_size > 0) {
		result.trailer = malloc(result.trailer_size);
		if (result.trailer == NULL) {
			perror ("Unable to allocate memory for the image trailer\n");
			exit(1);
		}

		read_bytes = read(fd, result.trailer, result.trailer_size);
		if (read_bytes == -1) {
			perror ("Unable to read image trailer bytes\n");
			exit(1);
		}
	}
	else {
		result.trailer = NULL;
	}

	return result;
}

void write_bitmap_image(struct bitmap_image_ bi, int fd) {
	write(fd, &bi.bfh, sizeof(struct bitmap_file_header_));
	write(fd, &bi.bih, sizeof(struct bitmap_info_header_));
	write(fd, bi.data, bi.data_size);
	if (bi.trailer_size > 0) {
		write(fd, bi.trailer, bi.trailer_size);
	}

}
