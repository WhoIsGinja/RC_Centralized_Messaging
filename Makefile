LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

all: client server

#client compilation
client: protocol_constants.o src_client/auxiliar/connection_manager.o src_client/main.o 
	$(LD) $(CFLAGS) -o client src_client/auxiliar/connection_manager.o src_client/main.o protocol_constants.o

src_client/auxiliar/connection_manager.o: protocol_constants.h src_client/auxiliar/connection_manager.c src_client/auxiliar/connection_manager.h 
	$(LD) $(CFLAGS) -o src_client/auxiliar/connection_manager.o -c src_client/auxiliar/connection_manager.c

src_client/main.o: protocol_constants.h src_client/auxiliar/connection_manager.h src_client/main.c
	$(LD) $(CFLAGS) -o src_client/main.o -c src_client/main.c


#Server compilation
server: protocol_constants.o src_server/auxiliar/data_manager.o src_server/main.o
	$(LD) $(CFLAGS) -o server src_server/auxiliar/data_manager.o src_server/main.o protocol_constants.o

src_server/auxiliar/data_manager.o: protocol_constants.h src_server/auxiliar/data_manager.c src_server/auxiliar/data_manager.h 
	$(LD) $(CFLAGS) -o src_server/auxiliar/data_manager.o -c src_server/auxiliar/data_manager.c

src_server/main.o: protocol_constants.h src_server/auxiliar/data_manager.h  src_server/main.c
	$(LD) $(CFLAGS) -o src_server/main.o -c src_server/main.c


#Common Lib
protocol_constants.o: protocol_constants.c protocol_constants.h
	$(LD) $(CFLAGS) -o protocol_constants.o -c protocol_constants.c

clean:
	rm protocol_constants.o src_client/*.o src_client/auxiliar/*.o client src_server/auxiliar/data_manager.o src_server/*.o server
