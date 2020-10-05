CC = gcc -g
CCFLAGS = -Wall
DEPS = applicationFunctions.h arpFunctions.h epollFunctions.h interfaceFunctions.h linkedList.h mip_deamon.h rawFunctions.h socketFunctions.h protocol.h routing_deamon.h msgQ.h routing.h routingTable.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: mipDeamon pingClient pingServer routingDeamon

mipDeamon: mip_deamon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o msgQ.o routing.o
		$(CC) $(CCFLAGS) -o mipDeamon mip_deamon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o msgQ.o routing.o

pingClient: pingClient.o socketFunctions.o applicationFunctions.o
		$(CC) $(CCFLAGS) -o pingClient pingClient.o socketFunctions.o applicationFunctions.o

pingServer: pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o
		$(CC) $(CCFLAGS) -o pingServer pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o

routingDeamon: routing_deamon.o socketFunctions.o epollFunctions.o applicationFunctions.o linkedList.o routingTable.o
		$(CC) $(CCFLAGS) -o routingDeamon routing_deamon.o socketFunctions.o applicationFunctions.o epollFunctions.o linkedList.o routingTable.o

clean:
	rm mipDeamon pingServer pingClient routingDeamon *.o
