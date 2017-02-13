CC = gcc -pedantic -Wall
DEBUG = -g
EXE =  echo
LDFLAGS = -lpthread

all : $(EXE)
	rm -f *.o

.PHONY : clean
clean :
	rm -f *.o core $(EXE)

$(EXE) : % : %.o redes2.o
	$(CC) -o $@ $@.o redes2.o $(LDFLAGS)

%.o : %.c redes2.h
	$(CC) $(CFLAGS) -c $<
