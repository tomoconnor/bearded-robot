CC=gcc
LIBS=-lpthread
DIST=dist/
CFLAGS=-ggdb

all: server client

server: 
	$(CC) $(CFLAGS) $(LIBS) -o $(DIST)server ex2server.c

client: 
	$(CC) $(CFLAGS) $(LIBS) -o $(DIST)client ex2client.c

.PHONY: clean

clean: 
	rm -f $(DIST)server $(DIST)client 
