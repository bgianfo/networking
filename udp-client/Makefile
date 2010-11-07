
CC = g++
CFLAGS  = -g -Wall -Wextra -std=c++98 -pedantic

LDFLAGS =
#LDFLAGS = -lnsl -lsocket

default: clean udp-client.cpp
	$(CC) udp-client.cpp Timer.C -o udp-client $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf udp-client
	rm -rf udp-client.dSYM
