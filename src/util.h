#ifndef UTIL_H_
#define UTIL_H_

struct arguments_ *prepare_arguments(int argc, char *argv[]);
struct bitmap_image_ *get_image(char *image_file);
struct bitmap_image_ *prepare_image(struct arguments_ *args);

#endif
