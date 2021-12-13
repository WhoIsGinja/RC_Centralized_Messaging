CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../

user: user_src/auxiliar/TCP.o user_src/auxiliar/UDP.o user_src/main.o
	$(LD) $(CFLAGS) -o user user_src/auxiliar/TCP.o user_src/auxiliar/UDP.o user_src/main.o

user_src/auxiliar/TCP.o: user_src/auxiliar/TCP.c user_src/auxiliar/TCP.h
	$(LD) $(CFLAGS) -o user_src/auxiliar/TCP.o -c user_src/auxiliar/TCP.c

user_src/auxiliar/UDP.o: user_src/auxiliar/UDP.c user_src/auxiliar/UDP.h
	$(LD) $(CFLAGS) -o user_src/auxiliar/UDP.o -c user_src/auxiliar/UDP.c

user_src/main.o: user_src/main.c user_src/auxiliar/TCP.h user_src/auxiliar/UDP.h
	$(LD) $(CFLAGS) -o user_src/main.o -c user_src/main.c

clean:
	rm user_src/*.o user_src/auxiliar/*.o user

run: user
	./user