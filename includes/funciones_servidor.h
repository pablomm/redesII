#ifndef FUNCIONES_SERVIDOR_H
#define FUNCIONES_SERVIDOR_H

#include <redes2/irc.h>
#include <arpa/inet.h>


#include "config.h"
#include "red_servidor.h"

#define COM_OK 0
#define COM_ERROR -1
#define COM_QUIT -2



typedef struct _datosMensaje {
	int sckfd;
	char * msg;
	size_t len;
} DatosMensaje, *pDatosMensaje;

typedef status (*ArrayComandos)(char*, pDatosMensaje);

ArrayComandos comandos[IRC_MAX_COMMANDS];


void liberarEstructuras(void);
void *manejaMensaje(void* pdesc);
status nuevaConexion(int desc, struct sockaddr_in * address);
status cerrarConexion(int socket);
void liberaDatosMensaje(pDatosMensaje datos);
status procesaComando(char *comando, pDatosMensaje datos);

status crea_comandos(void);
status nick(char* comando, pDatosMensaje datos);
status user(char* comando, pDatosMensaje datos);
status join(char* comando, pDatosMensaje datos);
status list(char* comando, pDatosMensaje datos);
status whois(char* comando, pDatosMensaje datos);
status names(char* comando, pDatosMensaje datos);
status privmsg(char* comando, pDatosMensaje datos);
status ping(char* comando, pDatosMensaje datos);
status part(char* comando, pDatosMensaje datos);
status comandoVacio(char* comando, pDatosMensaje datos);


#endif /* FUNCIONES_SERVIDOR_H */
