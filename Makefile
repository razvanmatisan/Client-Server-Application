CC=g++

all: build

build: server subscriber


server: utils.o server.o
		$(CC) $^ -o $@

server.o: server.cpp
		$(CC) $^ -c

utils.o: utils.cpp
		$(CC) $^ -c

subscriber: utils.o subscriber.o
		$(CC) $^ -o $@

subscriber.o: subscriber.cpp
		$(CC) $^ -c

clean:
		rm *.o -f server subscriber
	