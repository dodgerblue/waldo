#ifndef PICTURE_H_
#define PICTURE_H_

struct bitmap_file_header {
	char header_field[2]; // standard reads ASCII BM
	int size; // size of the bmp in bytes
	char reserved[4];
	int offset; // starting address of the byte where the bitmap image data can be found
};

struct bitmap_image {
	struct bitmap_file_header bmp_header; // the image header
};

struct bitmap_image read_bitmap_image(int file_descriptor);

#endif
