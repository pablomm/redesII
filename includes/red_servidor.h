
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

/**
 *
 * Estructura para almacenar temporalmente
 * datos de una conexion antes de registrar 
 * un usuario
 *
 */
typedef struct _redinf{
    int sckfd;	/* Numero de descriptor */
    struct sockaddr_in address; /* Informacion direccion */
	socklen_t scklen; /* Longitud direccion */
	void * next; /* Puntero al siguiente lista enlazada */

}Redinf, *pRedinf;

/*
 Funcion crear un socket tcp
*/
status crearSocketTCP(int * sckfd, unsigned short port);

/*
 Funcion aceptar una conexion y guardar en estructura valores
*/
status aceptarConexion(int sockval, pRedinf addrinf);


#endif /* RED_SERVIDOR_H */

