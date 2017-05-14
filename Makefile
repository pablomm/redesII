####################################
# Pablo Marcos Manchón             #
# Dionisio Pérez Alvear            #
# Pareja Nº 2                      #
####################################

# Compiladores
CC = gcc
AR = ar 

# Flags de compilacion
CFLAGS = -L$(LDIR) -I$(IDIR) -g -Wall -W -pedantic `pkg-config --cflags gtk+-3.0` -D_GNU_SOURCE  # -D PRUEBAS_IRC
LDFLAGS = -lpthread -lircredes -lircinterface -lsoundredes -lirctad -lsoundredes -lpulse -lpulse-simple `pkg-config --libs gtk+-3.0` -lssl -lcrypto -rdynamic 

# Carpetas
TAR_FILE= G-2302-02-P3.tar.gz
SDIR = src
SLDIR = srclib
IDIR = includes
LDIR = lib
ODIR = obj
MDIR = man
DDIR = doc
BDIR = .
FDIR = files
CDIR = certs
EDIR = echo
CSDIR = cliente_servidor
MDIR = misc
MOTD= motd.bash
NXCHAT2 = cliente_IRC
NSERVIDOR = servidor_IRC

# Personalizacion colores
NC='\033[0m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'

_LIB = libredes2.a
LIB = $(patsubst %,$(LDIR)/%,$(_LIB))

_LOBJ = red_cliente.o irc_cliente.o file_send.o red_servidor.o comandos_noirc.o comandos_cliente.o audiochat.o conexion_temp.o config.o funciones_registro.o ssl.o funciones_servidor.o bot.o
LOBJ = $(patsubst %,$(ODIR)/%,$(_LOBJ))

_OBJ = xchat2.o servidor.o servidor_echo.o cliente_echo.o bot_galletas.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_BINE = servidor_echo cliente_echo
BINE = $(patsubst %,$(EDIR)/%,$(_BINE))

_BIN = xchat2 servidor $(_BINE) bot_galletas
BIN = $(patsubst %,$(BDIR)/%,$(_BIN))

all: directories certificados $(BIN) mvecho mvcpyxchat chmod

#Crea carpetas
directories:
	@mkdir -p $(ODIR)
	@mkdir -p $(FDIR)
	@mkdir -p $(LDIR)
	@mkdir -p $(EDIR)
	@mkdir -p $(DDIR)
	@mkdir -p $(CSDIR)

# Crea la libreria
$(LIB): $(LOBJ)
	@echo "Creando libreria ${BLUE}$@\'${NC}"
	@echo Objetos incluidos:
	@$(AR) rcv $@ $^
	@echo Libreria creada ${GREEN}[OK]${NC}

# Crea objetos
$(LOBJ):$(ODIR)/%.o: $(SLDIR)/%.c
	@echo -n "Compilando ${BLUE}$@${NC}"
	@$(CC) -c -o $@ $< $(CFLAGS)
	@echo ${GREEN}[OK]${NC}

# Crea objetos de ejecutables
$(OBJ):$(ODIR)/%.o: $(SDIR)/%.c
	@echo -n "Compilando ${BLUE}$@${NC}"
	@$(CC) -c -o $@ $< $(CFLAGS) 
	@echo ${GREEN}[OK]${NC}

# Crea ejecutables
$(BIN):%: $(ODIR)/%.o $(LIB)
	@echo -n "Enlazando ejecutable ${BLUE}$@${NC}"
	@$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)
	@echo ${GREEN}[OK]${NC}

# Genera los certificados
certificados:
	@mkdir -p $(CDIR)
	@chmod +x ca.sh client.sh server.sh 
	@echo -n "Generando certificado CA "
	@./ca.sh 2> /dev/null > /dev/null
	@echo ${GREEN}[OK]${NC}
	@echo -n "Generando certificado cliente "
	@./client.sh 2> /dev/null > /dev/null
	@echo ${GREEN}[OK]${NC}
	@echo -n "Generando certificado servidor "
	@./server.sh 2> /dev/null > /dev/null
	@echo ${GREEN}[OK]${NC}

# Borra los certificados
rmcertificados:
	@rm -rdf $(CDIR)/*.pem $(CDIR)/*.srl  $(CDIR)/*.key

# Da permisos de ejecucion a los scripts
chmod:
	@chmod +x $(MDIR)/motd.bash

# Libera los puertos mal cerrados
port:
	@fuser -k 6669/tcp
	@fuser -k 6667/tcp

# Mueve binarios echo a la carpeta correspondiente
mvecho:
	@mv $(_BINE) $(EDIR)

# Mueve los binarios de xchat y servidor para la prueba del c3po
mvcpyxchat:
	@cp xchat2 $(CSDIR)/$(NXCHAT2)
	@cp servidor $(CSDIR)/$(NSERVIDOR)

# Borra archivos de la carpeta recepcion
rmfiles:
	@rm -rf $(FDIR)/$(MOTD)

# Cambia la fecha de todos los archivos
touch:
	@touch *
	@touch */*

# Crea archivo comprimido
compress: clean doc
	rm -rf $(TAR_FILE)
	rm -rf G-2302-02-P3
	tar -zcvf ../$(TAR_FILE) ../G-2302-02-P3
	mv ../$(TAR_FILE) $(TAR_FILE)

# Genera la documentacion
doc: clean 
	doxygen doc/Doxyfile

.PHONY: clean
clean: rmcertificados
	@rm -fr $(BIN) $(LIB) $(OBJ) $(LOBJ) $(BINE)
	@rm -fr $(CSDIR)/$(NXCHAT2) $(CSDIR)/$(NSERVIDOR)
	@rm -f $(TAR_FILE)
