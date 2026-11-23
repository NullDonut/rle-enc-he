CC = gcc
CFLAGS = -fsanitize=address,undefined -Wextra -Wall -g

rle:main.c
	$(CC) $(CFLAGS) -o $@ main.c

clean:
	rm -rf rle enc.rle
