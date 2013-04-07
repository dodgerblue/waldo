CFLAGS = -Wall -g
CC = gcc

all: test_picture

test_picture: main.o picture.o cipher.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f test_picture *.o
