#include <stdio.h>

void print_usage(char *name) {
	printf("\n\
	Usage: %s <file_name> <message>\n \
	file_name	-	absolute or relative path to bmp file\n \
	message		-	text string to hide in the bmp image\n\n",
	name);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		print_usage(argv[0]);
		return 1;
	}

	return 0;
}
