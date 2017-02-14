
#ifndef CONFIG_H
#define CONFIG_H


#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVICIO "ServidorIRC"

#define DEFAULT_LOG LOG_INFO

#define DEFAULT_PORT 194

#define PING_TIME 30

#ifndef RED_ERROR 
#define RED_ERROR -1
#endif

#ifndef RED_OK
#define RED_OK 0
#endif



#define DOWNNUEVO() pthread_mutex_lock(&mutexnuevo)
 
#define DOWNDESCR() pthread_mutex_lock(&mutexdescr)

#define UPNUEVO() pthread_mutex_unlock(&mutexnuevo)
 
#define UPDESCR() pthread_mutex_unlock(&mutexdescr)

typedef status int;

pthread_mutex_t mutexnuevo;
pthread_mutex_t mutexdescr;


void inicializarServidor(void);
void cerrarServidor(void);
void abrirLog(char * identificacion, int logLevel);
void daemonizar(char * identificacion, int logLevel);

#endif /* CONFIG_H */

