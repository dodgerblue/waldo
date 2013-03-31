#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "picture.h"

void print_usage(char *name) {
	printf("\n\
	Usage: %s <file_name> <message>\n \
	file_name	-	absolute or relative path to bmp file\n \
	message		-	text string to hide in the bmp image\n\n",
	name);
}

int main(int argc, char **argv) {
	int fd;
	struct bitmap_image_ image;
	char new_image[100];

	if (argc != 3) {
		print_usage(argv[0]);
		return 1;
	}

	printf("Image file: %s\n", argv[1]);
	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("Unable to open image file\n");
		goto out_fail;
	}

	image = read_bitmap_image(fd);

	close(fd);

	sprintf(new_image, "%s_new.bmp", argv[1]);

	fd = open(new_image, O_RDWR | O_CREAT | O_TRUNC);
	if (fd == -1) {
		perror("Unable to open new image file for writing");
		goto out_fail;
	}

	write_bitmap_image(image, fd);
	close(fd);

	if (image.data != NULL) {
		free(image.data);
	}

	if (image.trailer != NULL) {
		free(image.trailer);
	}

	return 0;

out_fail:
	return 1;
}
