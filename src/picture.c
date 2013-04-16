#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "picture.h"

void print_bitmap_file_header(struct bitmap_file_header_ *bfh) {
	printf("Read bitmap file header:\n \
	Header field: %c%c\n \
	Image size: %ld\n \
	Bitmap image offset: %ld\n",
	bfh->fields.header_field[0], bfh->fields.header_field[1], *(long int*) bfh->fields.size, *(long int*) bfh->fields.offset);
}

struct bitmap_file_header_* read_bitmap_file_header(int fd) {
	struct bitmap_file_header_ *result = NULL;
	ssize_t read_bytes;

	result = malloc(sizeof(struct bitmap_file_header_));
	if (result == NULL) {
		fprintf (stderr, "Unable to allocate memory for the bitmap file header\n");
		goto out_fail_alloc;
	}

	memset(result, 0, sizeof(struct bitmap_file_header_));

	read_bytes = read(fd, result->buffer, sizeof(struct bitmap_file_header_));
	if (read_bytes == -1) {
		fprintf(stderr, "Unable to read image header file\n");
		goto out_fail_read_file_header;
	}

	if (strncmp(result->fields.header_field, BMP_IDENTIFIER, strlen(BMP_IDENTIFIER))) {
		fprintf (stderr, "Unknowm bitmap file header identifier %s\n", result->fields.header_field);
		goto out_fail_invalid_file_header;
	}

	return result;

out_fail_invalid_file_header:
out_fail_read_file_header:
	free(result);
out_fail_alloc:
	return NULL;
}

void print_dib_header(struct bitmap_info_header_ *dh) {
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
	*(long int*) dh->size, *(long int*) dh->fields.width, *(long int*) dh->fields.height,
	*(short int*) dh->fields.planes, *(short int*) dh->fields.depth,
	*(long int*) dh->fields.compression, *(long int*) dh->fields.data_size,
	*(long int*) dh->fields.horizontal_rule, *(long int*) dh->fields.vertical_resolution,
	*(long int*) dh->fields.colors, *(long int*) dh->fields.vip_colors);
}

int validate_image(struct bitmap_info_header_ *bih) {
	int error_found = 0;
	int bytes_per_pixel = 0;

	if (*(long int*) bih->size != DIB_HEADER_SIZE) {
		fprintf (stderr, "Unsupported DIB header type (size %ld) - currently only %s is supported\n",
			*(long int*)bih->size, DIB_HEADER_TYPE);
		error_found = 1;
	}

	if (*(short int*) bih->fields.planes != 1) {
		fprintf(stderr, "Only 1 color planes are supported\n");
		error_found = 1;
	}

	if (*(short int*) bih->fields.depth != 24 ) {
		fprintf(stderr, "Only 24 color bit depth is supported\n");
		error_found = 1;
	}

	if (*(long int*) bih->fields.compression != 0) {
		fprintf(stderr, "Compressed images are not supported in current implementation\n");
		error_found = 1;
	}

	bytes_per_pixel = *(short int*) bih->fields.depth / 8;

	if (*(long int*) bih->fields.data_size != *(long int*) bih->fields.width * *(long int*) bih->fields.height * bytes_per_pixel) {
		fprintf(stderr, "Data size != width * height * depth / 8\n");
		error_found = 1;
	}

	return error_found;
}

struct bitmap_info_header_* read_dib_header(int fd) {
	struct bitmap_info_header_ *result;
	ssize_t read_bytes;

	result = malloc(sizeof(struct bitmap_info_header_));
	if (result == NULL) {
		fprintf(stderr, "Unable to allocate memory for the bitmap info header\n");
		goto out_fail;
	}

	memset(result, 0, sizeof(struct bitmap_info_header_));

	read_bytes = read(fd, result, sizeof(struct bitmap_info_header_));
	if (read_bytes == -1) {
		fprintf(stderr, "Unable to read image header file\n");
		goto out_unable_read;
	}

	if (validate_image(result)) {
		fprintf(stderr, "Unsupported image format\n");
		goto out_error_validate;
	}

	return result;

out_error_validate:
out_unable_read:
	free(result);
out_fail:
	return NULL;
}

struct bitmap_image_* read_bitmap_image(int fd) {
	struct bitmap_image_* result;
	ssize_t read_bytes;

	result = malloc(sizeof(struct bitmap_image_));
	if (result == NULL) {
		fprintf(stderr, "Unable to allocate memory for bitmap image struct\n");
		goto out_unable_to_allocate_image;
	}

	memset(result, 0, sizeof(struct bitmap_image_));

	result->bfh = read_bitmap_file_header(fd);
	result->bih = read_dib_header(fd);

	if (result->bfh == NULL || result->bih == NULL) {
		fprintf(stderr, "Unable to properly read image and/or info headers\n");
		goto out_unable_to_read_headers;
	}

	// allocate space for the image bytes
	result->data_size = *(long int*)result->bih->fields.data_size;
	result->data = malloc (result->data_size);
	if (result->data == NULL) {
		fprintf(stderr, "Unable to allocate memory for the image data\n");
		goto out_no_memory_img_data;
	}

	read_bytes = read(fd, result->data, result->data_size);
	if (read_bytes == -1) {
		fprintf(stderr, "Unable to read the image data bytes\n");
		goto out_error_read_image_data;
	}

	// allocate space for the image trailer
	result->trailer_size = *(long int*) result->bfh->fields.size -
		sizeof(struct bitmap_file_header_) -
		sizeof(struct bitmap_info_header_) -
		result->data_size;

	if (result->trailer_size < 0) {
		fprintf(stderr, "file size < headers size + data size\n");
		goto out_invalid_trailer;
	}

	if (result->trailer_size > 0) {
		result->trailer = malloc(result->trailer_size);
		if (result->trailer == NULL) {
			fprintf(stderr, "Unable to allocate memory for the image trailer\n");
			goto out_invalid_trailer;
		}

		read_bytes = read(fd, result->trailer, result->trailer_size);
		if (read_bytes == -1) {
			fprintf(stderr, "Unable to read image trailer bytes\n");
			free(result->trailer);
			goto out_invalid_trailer;
		}
	}
	else {
		result->trailer = NULL;
	}

	return result;

out_invalid_trailer:
out_error_read_image_data:
	free(result->data);
out_no_memory_img_data:
	free(result->bfh);
	free(result->bih);
out_unable_to_read_headers:
	free(result);
out_unable_to_allocate_image:
	return NULL;
}

void write_bitmap_image(struct bitmap_image_ *bi, int fd) {
	// TODO check error codes
	write(fd, bi->bfh, sizeof(struct bitmap_file_header_));
	write(fd, bi->bih, sizeof(struct bitmap_info_header_));
	write(fd, bi->data, bi->data_size);
	if (bi->trailer_size > 0) {
		write(fd, bi->trailer, bi->trailer_size);
	}
}

void free_bitmap_image(struct bitmap_image_ *bi) {
	free(bi->bfh);
	free(bi->bih);
	free(bi->data);

	if (bi->trailer != NULL)
		free(bi->trailer);
}
