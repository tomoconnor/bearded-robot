CC=gcc
LIBS=-lpthread
DIST=dist/

all: server client

server: 
	$(CC) $(LIBS) -o $(DIST)server ex2server.c

client: 
	$(CC) $(LIBS) -o $(DIST)client ex2client.c

.PHONY: clean

clean: 
	rm -f $(DIST)server $(DIST)client 
