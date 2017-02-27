
#ifndef CONEXION_TEMP_H
#define CONEXION_TEMP_H

#include <arpa/inet.h>

#include "config.h"

#define CON_OK 0
#define CON_ERROR -1



typedef struct _TempUser {

	int socket;
	char * nick;
	char *host;
	char IP[INET6_ADDRSTRLEN];
	struct _TempUser * previous;
	struct _TempUser * next;

} TempUser, *pTempUser;

pTempUser usuarioPrimero;
pTempUser usuarioUltimo;

pthread_mutex_t mutexTempUser;

status deleteTempUser(int socket);
status newTempUser(int socket,  char *ip, char *host);
pTempUser pullTempUser(int socket);
status liberaTempUser(pTempUser usuario);
status liberaTodosTempUser(void);



#endif /* CONEXION_TEMP_H */
