CC_FLAGS:=-g -Wall
CC:=gcc

all: test_picture

test_picture: main.o picture.o cipher.o
	$(CC) $(CCFLAGS) $^ -o $@

main.o: src/main.c
	$(CC) $(CCFLAGS) -c $^ -o $@

picture.o: src/picture.c
	$(CC) $(CCFLAGS) -c $^ -o $@

cipher.o: src/cipher.c
	$(CC) $(CCFLAGS) -c $^ -o $@

.PHONY: clean

clean:
	rm -f test_picture *.o
