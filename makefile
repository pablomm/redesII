CC = gcc -pedantic -Wall
DEBUG = -g
EXE =  servidor
PRUEBAS = echo
LDFLAGS = -lpthread -lircinterface -lircredes -lirctad -lsoundredes

all : $(EXE)
	rm -f *.o

.PHONY : clean
clean :
	rm -f *.o core $(EXE) $(PRUEBAS)

test : $(PRUEBAS)
	rm -f *.o

$(EXE) : % : %.o redes2.o
	$(CC) -o $@ $@.o redes2.o $(LDFLAGS)

%.o : %.c redes2.h
	$(CC) $(CFLAGS) -c $<
