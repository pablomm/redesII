
#ifndef REDES2_H
#define REDES2_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVICIO "Servidor IRC"
#define DEFAULT_LOG LOG_INFO

#define ERROR -1
#define OK 0


void abrirLog(int logLevel);
void daemonizar(int logLevel);
int crearSocketTCP(unsigned short port, int connections);
int aceptar_conexion(int sockval);




#endif /* REDES2_H */
