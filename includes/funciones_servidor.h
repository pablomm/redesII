#ifndef FUNCIONES_SERVIDOR_H
#define FUNCIONES_SERVIDOR_H

#include "config.h"
#include "red_servidor.h"


#define N_COMANDOS 100

typedef struct _datosMensaje {
	int sckfd;
	char * msg;
	size_t len;
} DatosMensaje, *pDatosMensaje;

typedef status (*ArrayComandos)(char*, pDatosMensaje);

ArrayComandos comandos[N_COMANDOS];

void liberarEstructuras(void);
void *manejaMensaje(void* pdesc);
status nuevaConexion(int desc, struct sockaddr_in address);
void liberaDatosMensaje(pDatosMensaje datos);
status crea_comandos();

#endif /* FUNCIONES_SERVIDOR_H */
