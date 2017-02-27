#include "../includes/funciones_servidor.h"

/* #include <irc.h> */


void liberarEstructuras(void){

	printf("Llamada libera estructura\n");

}

void *manejaMensaje(void* pdesc){

	pDatosMensaje datos;
	datos = (pDatosMensaje) pdesc;
	char *next;

	
    syslog(LOG_DEBUG, "Mensaje: [%s], len=%lu", (char*) datos->msg, strlen(datos->msg));

    next = IRC_UnPipelineCommands (datos->msg, &comando, strlen(comando));

    do {

        if(procesaComando(comando, datos) == COM_QUIT){

			printf("mal procesado implementar luego\n");
			break;
		}

        if(comando!=NULL) {
			free(comando);
		}

        next = IRC_UnPipelineCommands(NULL, &comando, next);

    }while(next!=NULL)  
	
	addFd(datos->sckfd);
    liberaDatosMensaje(datos);

	return NULL;
}

status procesaComando(char *comando, pDatosMensaje datos){
	long dev;
	
	if(comando == NULL){
		return COM_ERROR;
	}

	dev = IRC_CommandQuery(comando);
	
	/* RECORDAR IRC_MAX_COMMANDS */
	if(dev >= NCOMMANDS){
		return COM_ERROR;
	}

	return comandos[dev](comando, datos);

}

status nuevaConexion(int desc, struct sockaddr_in address){

	

	printf("Llamada nueva conexion\n");

	addFd(desc);
	return 0;
}

void liberaDatosMensaje(pDatosMensaje datos){
	
	free(datos->msg);
	datos->msg = NULL;
	free(datos);
}
/*
status crea_comandos() {
    int i;    
    for(i=0; i<N_COMANDOS; i++) {
        comandos[i] = comandoVacio; 
	}

    comandos[0]=nick;
    comandos[1]=user; 
	return 0;   
}

*/


