
CC = g++
CFLAGS  = -g -Wall -Wextra -std=c++98 -pedantic

#LDFLAGS = -I../client 
LDFLAGS = -I../client -lnsl -lsocket

default: clean server.cpp
	$(CC) server.cpp -o server $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf server
	rm -rf server.dSYM
