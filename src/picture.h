#ifndef PICTURE_H_
#define PICTURE_H_

#define BMP_IDENTIFIER "BM"
#define DIB_HEADER_SIZE 40
#define DIB_HEADER_TYPE "BITMAPINFOHEADER"

struct bitmap_file_header_ {
	union {
		char buffer[14];
		struct {
			char header_field[2]; // standard reads ASCII BM
			char size[4]; // size of the bmp in bytes
			char reserved[4];
			char offset[4]; // starting address of the byte where the bitmap image data can be found
		} fields;
	};
};

struct bitmap_info_header_ {
	char size[4];
	union {
		char buffer[36];
		struct {
			char width[4]; // image width
			char height[4]; // image height
			char planes[2]; // color planes
			char depth[2]; // color depth
			char compression[4]; // color compression
			char data_size[4]; // image data size
			char horizontal_rule[4]; // bitmap image horizontal rule
			char vertical_resolution[4]; // bitmap vertical image resolution
			char colors[4]; // number of colors user
			char vip_colors[4]; // number of important colors
		} fields;
	};
};

struct bitmap_image_ {
	struct bitmap_file_header_ *bfh; // the image header
	struct bitmap_info_header_ *bih; // the DIB header for the image
	long int data_size; // the image data size
	char *data; // the actual image data
	long int trailer_size; // the image trailer size
	char *trailer; // the image trailer - if any - to represent the image in the same state as it was at the beginning
};

void print_bitmap_file_header(struct bitmap_file_header_ *bfh);
void print_dib_header(struct bitmap_info_header_ *dh);

struct bitmap_image_* read_bitmap_image(int file_descriptor);
// TODO check return codes and everything - make this bullet proof as well
void write_bitmap_image(struct bitmap_image_ *bi, int file_descriptor);
void free_bitmap_image(struct bitmap_image_ *bi);

#endif
