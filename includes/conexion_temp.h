
#ifndef CONEXION_TEMP_H
#define CONEXION_TEMP_H

#include "config.h"

#define CON_OK 0
#define CON_ERROR -1

#define IPLEN 4

typedef struct _TempUser {

	int socket;
	char * nick;
	char *host;
	char IP[INET6_ADDRSTRLEN];
	pTempUser previous;
	pTempUser next;

} TempUser, *pTempUser;

pTempUser usuarioPrimero = NULL;
pTempUser usuarioUltimo = NULL;

status deleteTempUser(int socket);
status newTempUser(int socket,  char *ip, char *host);
status pullTempUser(int socket, pTempUser usuario);



#endif /* CONEXION_TEMP_H */
