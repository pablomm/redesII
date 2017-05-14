/**
 * @file funciones_servidor.h
 * @brief funciones referidas a los comandos
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef FUNCIONES_SERVIDOR_H
#define FUNCIONES_SERVIDOR_H

#include <redes2/irc.h>
#include <arpa/inet.h>


#include "config.h"
#include "red_servidor.h"

#define COM_OK 0
#define COM_ERROR -1
#define COM_QUIT -2

#define FMOTD "misc/motd" /**< Archivo del motd */

#define MOTD_SCRIPT "misc/motd.bash"

#define FMOTDOFFENSIVE "misc/motdoffensive"

typedef struct _datosMensaje {
	int sckfd;
	char * msg;
	size_t len;
} DatosMensaje, *pDatosMensaje;

typedef status (*ArrayComandos)(char*, pDatosMensaje);

ArrayComandos comandos[IRC_MAX_COMMANDS]; /**< Declaramos el array de comandos con el numero maximo de comandos */

void liberarEstructuras(void);

void *manejaMensaje(void* pdesc);

status procesaComando(char *comando, pDatosMensaje datos);

status nuevaConexion(int desc, struct sockaddr_in * address);

status cerrarConexion(int socket);

void liberaDatosMensaje(pDatosMensaje datos);

status liberarUserData(char *user, char *nick, char *real, char *host, char *IP, char *away);

status rutinaPingPong(void);

status crea_comandos(void);

status nick(char* comando, pDatosMensaje datos);

status user(char* comando, pDatosMensaje datos);

status join(char* comando, pDatosMensaje datos);

status list(char* comando, pDatosMensaje datos);

status whois(char* comando, pDatosMensaje datos);

status names(char* comando, pDatosMensaje datos);

status privmsg(char* comando, pDatosMensaje datos);

status ping(char* comando, pDatosMensaje datos);

status pong(char* comando, pDatosMensaje datos);

status part(char* comando, pDatosMensaje datos);

status topic(char* comando, pDatosMensaje datos);

status kick(char* comando, pDatosMensaje datos);

status away(char* comando, pDatosMensaje datos);

status mode(char* comando, pDatosMensaje datos);

status quit(char* comando, pDatosMensaje datos);

status motd(char* comando, pDatosMensaje datos);

status who(char* comando, pDatosMensaje datos);

status comandoVacio(char* comando, pDatosMensaje datos);

void enviarMensajeACanal(int sckfd, char *mensaje, char *canal, char * nickorigin);

#endif /* FUNCIONES_SERVIDOR_H */
