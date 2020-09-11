CC = gcc
CCFLAGS = -g

all: mipDeamon pingClient pingServer

mipDeamon:
	$(CC) $(CCFLAGS) mip_deamon.c arpFunctions.c applicationFunctions.c epollFunctions.c interfaceFunctions.c linkedList.c rawFunctions.c socketFunctions.c -o mipDeamon

pingClient:
		$(CC) $(CCFLAGS) pingClient.c socketFunctions.c applicationFunctions.c -o pingClient

pingServer:
	$(CC) $(CCFLAGS) pingServer.c socketFunctions.c applicationFunctions.c epollFunctions.c -o pingServer

clean:
	rm mipDeamon pingServer pingClient *.o
