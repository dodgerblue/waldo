#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

struct arguments_ {
	int hash_id;
	int scatter_id;
	int msg_from_file;

	char *image;
	char *message;
};

struct arguments_* alloc_arguments();
void free_arguments(struct arguments_ *args);
void print_arguments(struct arguments_ *args);
struct arguments_* parse_arguments(int argc, char *argv[]);

#endif
