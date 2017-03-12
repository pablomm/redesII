# Makefile 
################################################################################

# banderas de compilacion
CC = gcc
CFLAGS = -Wall -g -pedantic
LDLIBS = -lpthread -lircinterface -lircredes -lirctad -lsoundredes 


RED='\033[0;31m'
NC='\033[0m'
GREEN='\033[0;32m'

TAR_FILE= G-2302-01-P1.tar.gz

# fuentes a considerar
SOURCES = config.c red_servidor.c funciones_servidor.c conexion_temp.c servidor.c 

OBJECTS = obj/config.o obj/red_servidor.o obj/funciones_servidor.o obj/conexion_temp.o obj/servidor.o 


# ejecutable
EXEC_SOURCES = src/servidor.c

all: $(OBJECTS) exe

exe: servidor

# receta para hacer un .o de src
obj/%.o : src/%.c
	@echo -n compilando objeto \'$<\'...
	@$(CC) -c $(CFLAGS) $< -c -o $@
	@echo ${GREEN}[OK]${NC}

# receta para hacer un ejecutable
servidor : $(OBJECTS)
	@echo -n compilando ejecutable \'$@\'...
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	@echo ${GREEN}[OK]${NC}

# para ejecutar el programa
run:
	@./servidor

#para hacer el tar.gz
comprimir: clean
	@rm -fv G-2302-01-P1.tar.gz
	@tar -zcvf ../$(TAR_FILE) ../G-2302-01-P1/
	@mv ../$(TAR_FILE) $(TAR_FILE)

doc: 
	doxygen

# limpieza
.PHONY: clean
clean:	
	@rm -fv servidor
	@rm -r -fv G-2302-01-P1
	@rm -fv */*~
	@rm -fv *~
	@rm -fv obj/*.o
	@rm -fv $(TAR_FILE)
