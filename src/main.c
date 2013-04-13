#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "picture.h"
#include "cipher.h"

extern struct cipher_method_ my_cipher_method;
extern struct hash_method_ hash_methods[];

void print_usage(char *name) {
	printf("\n\
	Usage: %s <file_name> <message>\n \
	file_name	-	absolute or relative path to bmp file\n \
	message		-	text string to hide in the bmp image\n\n",
	name);
}

int main(int argc, char **argv) {
	int fd;
	struct bitmap_image_ *image = NULL;
	char new_image[100];
	struct wrapped_message_ *msg = NULL;

	if (argc != 3) {
		print_usage(argv[0]);
		return 1;
	}

	printf("Image file: %s\n", argv[1]);
	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "Unable to open image file\n");
		goto out_fail;
	}

	image = read_bitmap_image(fd);
	if (image == NULL) {
		fprintf(stderr, "Unable to read and parse bitmap image\n");
		close(fd);
		goto out_fail;
	}

	// do a little messing around here
	test_cipher(my_cipher_method, image);

	// let's cipher the message
	msg = (struct wrapped_message_ *) wrap_message(hash_methods[1], argv[2]);
	print_wrapped_message(msg);

	sprintf(new_image, "%s_new.bmp", argv[1]);

	fd = open(new_image, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		fprintf(stderr, "Unable to open new image file for writing\n");
		goto out_fail;
	}

	write_bitmap_image(image, fd);
	close(fd);

	free_bitmap_image(image);

	return 0;

out_fail:
	return 1;
}
