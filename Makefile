CC = gcc
CFLAGS = -Wall -Wextra -pedantic

all: tftp-client tftp-server

tftp-client: tftp-client.c tftp-utils.c
	$(CC) $(CFLAGS) -o $@ $^

tftp-server: tftp-server.c tftp-utils.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f tftp-client tftp-server
