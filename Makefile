CC = gcc
CFLAGS = -fsanitize=address,undefined -Wextra -Wall -g

rle:writer.c writer.h
	$(CC) $(CFLAGS) -o $@ writer.c

test: rle
	./test.sh ./rle

clean:
	rm -rf rle enc.rle out.bin
