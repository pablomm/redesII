
#ifndef RED_SERVIDOR_H
#define RED_SERVIDOR_H

#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

#include "config.h"


status crearSocketTCP(unsigned short port, int connections);
status aceptarConexion(int sockval);


#endif /* RED_SERVIDOR_H */

