
#ifndef redes2
#define redes2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>

#define SERVICIO "Servidor IRC"
#define DEFAULT_LOG LOG_INFO

#define ERROR -1
#define OK 0


void daemonizar(int logLevel);




#endif /* REDES2 */
