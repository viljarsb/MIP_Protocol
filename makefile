CC = gcc -g
CCFLAGS = -Wall
DEPS = applicationFunctions.h arpFunctions.h epollFunctions.h interfaceFunctions.h linkedList.h mip_daemon.h rawFunctions.h socketFunctions.h protocol.h routing_daemon.h msgQ.h routing.h routingTable.h log.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: mipDaemon pingClient pingServer routingDaemon

mipDaemon: mip_daemon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o msgQ.o routing.o log.o
		$(CC) $(CCFLAGS) -o mipDaemon mip_daemon.o arpFunctions.o applicationFunctions.o epollFunctions.o interfaceFunctions.o linkedList.o rawFunctions.o socketFunctions.o msgQ.o routing.o log.o

pingClient: pingClient.o socketFunctions.o applicationFunctions.o log.o
		$(CC) $(CCFLAGS) -o pingClient pingClient.o socketFunctions.o applicationFunctions.o log.o

pingServer: pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o log.o
		$(CC) $(CCFLAGS) -o pingServer pingServer.o socketFunctions.o applicationFunctions.o epollFunctions.o log.o

routingDaemon: routing_daemon.o socketFunctions.o epollFunctions.o applicationFunctions.o linkedList.o routingTable.o log.o
		$(CC) $(CCFLAGS) -o routingDaemon routing_daemon.o socketFunctions.o applicationFunctions.o epollFunctions.o linkedList.o routingTable.o log.o

clean:
	rm mipDaemon pingServer pingClient routingDaemon *.o
