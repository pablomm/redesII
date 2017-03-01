
#ifndef RED_SERVIDOR_H
#define RED_SERVIDOR_H

#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>

#include "config.h"


#define MAX_QUEUE 15
#define PING_TIME 30


#define RED_ERROR -1
#define RED_OK 0


/*
 Funcion crear un socket tcp
*/
status crearSocketTCP(int * sckfd, unsigned short port);

/*
 Funcion aceptar una conexion y guardar en estructura valores
*/
status aceptarConexion(int sockval,int *sckfd, struct sockaddr_in * address);

status enviar(int sockfd, char *mensaje);


#endif /* RED_SERVIDOR_H */

