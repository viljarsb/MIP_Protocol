CC = gcc
CCFLAGS = -g -Wall
DEPS = applicationFunctions.h arpFunctions.h epollFunctions.h interfaceFunctions.h linkedList.h mipDeamon.h rawFunctions.h socketFunctions.h protocol.h routing_deamon.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: mipDeamon pingClient pingServer

mipDeamon: mip_deamon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o
		$(CC) $(CCFLAGS) -o mipDeamon mip_deamon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o

pingClient: pingClient.o socketFunctions.o applicationFunctions.o
		$(CC) $(CCFLAGS) -o pingClient pingClient.o socketFunctions.o applicationFunctions.o

pingServer: pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o
		$(CC) $(CCFLAGS) -o pingServer pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o

clean:
	rm mipDeamon pingServer pingClient *.o
