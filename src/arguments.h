#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#define IMG_SUFFIX "_new.bmp"

struct arguments_ {
	int hash_id;
	int scatter_id;
	int msg_from_file;
	int just_zeroize;
	char *suffix;

	char *image;
	char *message;
};

struct arguments_* alloc_arguments();
void free_arguments(struct arguments_ *args);
void print_arguments(struct arguments_ *args);
struct arguments_* parse_arguments(int argc, char *argv[]);

#endif
