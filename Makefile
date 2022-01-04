LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

all: user server

#User compilation
user: user_src/auxiliar/connection_manager.o user_src/main.o
	$(LD) $(CFLAGS) -o user user_src/auxiliar/connection_manager.o user_src/main.o

user_src/auxiliar/connection_manager.o: user_src/auxiliar/connection_manager.c user_src/auxiliar/connection_manager.h protocol_constants.h
	$(LD) $(CFLAGS) -o user_src/auxiliar/connection_manager.o -c user_src/auxiliar/connection_manager.c

user_src/main.o: user_src/auxiliar/connection_manager.h protocol_constants.h user_src/main.c
	$(LD) $(CFLAGS) -o user_src/main.o -c user_src/main.c


#Server compilation
server: server_src/auxiliar/file_manager.o server_src/main.o
	$(LD) $(CFLAGS) -o server server_src/auxiliar/file_manager.o server_src/main.o

server_src/auxiliar/file_manager.o: server_src/auxiliar/file_manager.c server_src/auxiliar/file_manager.h protocol_constants.h
	$(LD) $(CFLAGS) -o server_src/auxiliar/file_manager.o -c server_src/auxiliar/file_manager.c

server_src/main.o: server_src/auxiliar/file_manager.h protocol_constants.h server_src/main.c
	$(LD) $(CFLAGS) -o server_src/main.o -c server_src/main.c


clean:
	rm user_src/*.o user_src/auxiliar/*.o user server