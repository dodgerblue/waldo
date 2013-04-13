#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "picture.h"
#include "cipher.h"
#include "arguments.h"

#define DEBUG 1

extern struct hash_method_ hash_methods[];

void print_usage(char *program_name) {
	int i;

	fprintf(stderr, "Usage: %s [OPTIONS] <image> <message>\n\n \
	image - the image file used to hide the message\n \
	message - the message to hide, as a string\n \
	\n \
	OPTIONS can be one or more of the following: \n \
	-h <hash method> - the method using for hashing the message\n \
	-s <scatter method> - the method used for hiding the message\n \
	\n",
	program_name);

	fprintf(stderr, "\tAvailable hash methods:\n");
	for (i = 0; hash_methods[i].id != UINT_MAX; i ++) {
		fprintf(stderr, "\t%u - %s\n", hash_methods[i].id, hash_methods[i].name);
	}
}

int main(int argc, char *argv[]) {
	struct arguments_  *args = NULL;

	if (argc < 3) {
		print_usage(argv[0]);
		return 1;
	}

	args = parse_arguments(argc, argv);
	if (args == NULL) {
		print_usage(argv[0]);
		return 1;
	}

	if (DEBUG)
		print_arguments(args);

	free_arguments(args);

	return 0;
}
