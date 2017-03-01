
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
#define MAX_BUFFER 1024

#define SERV_OK 0
#define SERV_ERROR -1


typedef int status;

pthread_mutex_t mutexDescr;

int running;
fd_set activeFD;
pid_t pid;

void daemonizar(char *servicio, int logLevel);
void cerrarDescriptores(void);
void abrirLog(char * identificacion, int logLevel);
status inicializarServidor(void);
void cerrarServidor(void);
void manejadorSigint(int signal);
void manejadorSIGUSR1(int signal);
status inicializarFD(int sckt);
status addFd(int sckt);
status deleteFd(int sckt);
status setReadFD(fd_set * readFD);
status lanzarServidor(unsigned int puerto);




#endif /* CONFIG_H */

