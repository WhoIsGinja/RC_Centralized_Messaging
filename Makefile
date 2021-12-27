CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

#User compilation
user: user_src/auxiliar/connection_controller.o user_src/main.o
	$(LD) $(CFLAGS) -o user user_src/auxiliar/connection_controller.o user_src/main.o

user_src/auxiliar/connection_controller.o: user_src/auxiliar/connection_controller.c user_src/auxiliar/connection_controller.h protocol_constants.h
	$(LD) $(CFLAGS) -o user_src/auxiliar/connection_controller.o -c user_src/auxiliar/connection_controller.c

user_src/main.o: user_src/auxiliar/connection_controller.h protocol_constants.h
	$(LD) $(CFLAGS) -o user_src/main.o -c user_src/main.c

#TODO Server compilation

clean:
	rm user_src/*.o user_src/auxiliar/*.o user

run: user
	./user