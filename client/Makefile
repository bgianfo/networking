
CC = g++
CFLAGS  = -g -Wall -Wextra -std=c++98 -pedantic

#LDFLAGS =
LDFLAGS = -lnsl -lsocket

default: clean client.cpp
	$(CC) client.cpp -o client $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf client
	rm -rf client.dSYM
