/**
 * @file conexion_temp.h
 * @brief estructura y funciones del usuario temporal
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef CONEXION_TEMP_H
#define CONEXION_TEMP_H

#include <arpa/inet.h>

#include "config.h"

#define CON_OK 0
#define CON_ERROR -1

typedef struct _TempUser {

	int socket; /**< Socket en el que almacenamos el usuario */
	char * nick;
	char *host;
	char IP[INET6_ADDRSTRLEN];
	struct _TempUser * previous; /**< Puntero al usuario anterior */
	struct _TempUser * next; /**< Puntero al usuario siguiente */

} TempUser, *pTempUser;

pTempUser usuarioPrimero; /**< Almacenamos el primer y ultimo usuario de manera global */
pTempUser usuarioUltimo;

pthread_mutex_t mutexTempUser; /**< Semaforo para trabajar con usuarios temporales */

status newTempUser(int socket,  char *ip, char *host);

status setNickTemporal(pTempUser usuario, char* nick);

pTempUser pullTempUser(int socket);

status deleteTempUser(int socket);

status liberaTempUser(pTempUser usuario);

status liberaTodosTempUser(void);

status printDebugUsers(void);

#endif /* CONEXION_TEMP_H */
