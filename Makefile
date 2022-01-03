CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

#User compilation
user: user_src/auxiliar/connection_manager.o user_src/main.o
	$(LD) $(CFLAGS) -o user user_src/auxiliar/connection_manager.o user_src/main.o

user_src/auxiliar/connection_manager.o: user_src/auxiliar/connection_manager.c user_src/auxiliar/connection_manager.h protocol_constants.h
	$(LD) $(CFLAGS) -o user_src/auxiliar/connection_manager.o -c user_src/auxiliar/connection_manager.c

user_src/main.o: user_src/auxiliar/connection_manager.h protocol_constants.h user_src/main.c
	$(LD) $(CFLAGS) -o user_src/main.o -c user_src/main.c

#TODO Server compilation

clean:
	rm user_src/*.o user_src/auxiliar/*.o user

run: user
	./user