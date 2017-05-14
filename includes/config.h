/**
 * @file config.h
 * @brief daemonizar y funciones relacionadas con el servidor y descriptores
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef CONFIG_H
#define CONFIG_H


#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

#define SERVICIO "ServidorIRC"
#define DEFAULT_LOG LOG_INFO
#define DEFAULT_PORT 6667
#define DEFAULT_PORT_SSL 6669
#define MAX_BUFFER 1024

#define ALARMPINGPONG 20

#define SERV_OK 0
#define SERV_ERROR -1

#define PATH_SERV_CERT "certs/servidor.pem"
#define PATH_SERV_PKEY "certs/serverkeypri.pem"

#ifndef STATUS
#define STATUS
typedef int status;
#endif

char abspath[1024];

pthread_mutex_t mutexDescr; /**< Semaforo para trabajar con los descriptores */

int running; /**< Para poder parar el while(running) de lanzar servidor desde fuera */

short offensive;

fd_set activeFD;

pid_t pid;

void daemonizar(char *servicio, int logLevel);

void cerrarDescriptores(void);

void abrirLog(char * identificacion, int logLevel);

void manejadorSIGUSR1(int signal);

void manejadorSIGALRM(int sennal);

status inicializarServidor(void);

void cerrarServidor(void);

void manejadorSigint(int signal);

status inicializarFD(int sckt);

status addFd(int sckt);

status deleteFd(int sckt);

status setReadFD(fd_set * readFD);

status lanzarServidor(unsigned int puerto, short pingpong);

status lanzarServidorSSL(unsigned int puerto, short pingpong);

#endif /* CONFIG_H */

