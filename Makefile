CFLAGS = -Wall -g
CC = gcc

all: hide_message

test_picture: main.o picture.o cipher.o
	$(CC) $(CFLAGS) $^ -o $@

hide_message: hide_message.o picture.o cipher.o arguments.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f hide_message test_picture *.o
