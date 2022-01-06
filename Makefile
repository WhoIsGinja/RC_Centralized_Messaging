LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

all: client server

#client compilation
client: src_client/auxiliar/connection_manager.o src_client/main.o
	$(LD) $(CFLAGS) -o client src_client/auxiliar/connection_manager.o src_client/main.o

src_client/auxiliar/connection_manager.o: src_client/auxiliar/connection_manager.c src_client/auxiliar/connection_manager.h protocol_constants.h
	$(LD) $(CFLAGS) -o src_client/auxiliar/connection_manager.o -c src_client/auxiliar/connection_manager.c

src_client/main.o: src_client/auxiliar/connection_manager.h protocol_constants.h src_client/main.c
	$(LD) $(CFLAGS) -o src_client/main.o -c src_client/main.c


#Server compilation
server: src_server/auxiliar/data_manager.o src_server/main.o
	$(LD) $(CFLAGS) -o server src_server/auxiliar/data_manager.o src_server/main.o

src_server/auxiliar/data_manager.o: src_server/auxiliar/data_manager.c src_server/auxiliar/data_manager.h protocol_constants.h
	$(LD) $(CFLAGS) -o src_server/auxiliar/data_manager.o -c src_server/auxiliar/data_manager.c

src_server/main.o: src_server/auxiliar/data_manager.h protocol_constants.h src_server/main.c
	$(LD) $(CFLAGS) -o src_server/main.o -c src_server/main.c


clean:
	rm src_client/*.o src_client/auxiliar/*.o client src_server/auxiliar/data_manager.o src_server/*.o server

rclient:
	clear
	./client

rserver:
	clear
	./server