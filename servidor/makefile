CC = gcc -pedantic
CFLAGS = -w
EXE =  echo
LDFLAGS = -lpthread

all : $(EXE)
	rm -f *.o
	@echo "#---------------------------"
	@echo " Pablo Marcos"
	@echo " Dionisio Perez"
	@echo " Pareja 02"
	@echo "#---------------------------"
.PHONY : clean
clean :
	rm -f *.o core $(EXE)

$(EXE) : % : %.o redes2.o
	@echo "gcc -std=c99 -pedantic -w -c -lpthread"
	$(CC) $(CFLAGS) -o $@ $@.o redes2.o $(LDFLAGS)

%.o : %.c redes2.h
	$(CC) $(CFLAGS) -c $<
