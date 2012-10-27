CC=gcc
LIBS=-lpthread
DIST=dist/
CFLAGS=-ggdb -L/usr/lib/x86_64-linux-gnu/

all: server client

server: 
	$(CC) -o $(DIST)server $(CFLAGS) ex2server.c $(LIBS)

client: 
	$(CC) -o $(DIST)client $(CFLAGS) ex2client.c $(LIBS)

.PHONY: clean

clean: 
	rm -f $(DIST)server $(DIST)client 
