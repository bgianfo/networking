
CC = g++
CFLAGS  = -Wall -Wextra -std=c++98 -pedantic
LDFLAGS = -lnsl -lsocket

default: clean client.cpp
	$(CC) client.cpp -o client $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf client
