/**
 * @file red_servidor.h
 * @brief crear sockets, aceptar conexion y enviar
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef RED_SERVIDOR_H
#define RED_SERVIDOR_H

#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>

#include "config.h"

#define MAX_QUEUE 15
#define PING_TIME 30 /**< Tiempo maximo entre pings */

#define RED_ERROR -1
#define RED_OK 0

status crearSocketTCP(int * sckfd, unsigned short port);

status aceptarConexion(int sockval,int *sckfd, struct sockaddr_in * address);

status enviar(int sockfd, char *mensaje);

#endif /* RED_SERVIDOR_H */

