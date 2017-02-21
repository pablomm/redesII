#include "../includes/funciones_servidor.h"


void liberarEstructuras(void){

	printf("Llamada libera estructura\n");

}

void *manejaMensaje(void* pdesc){

	pDatosMensaje datos;
	datos = (pDatosMensaje) pdesc;

	/*
    syslog(LOG_INFO, "Mensaje: [%s], len=%lu", (char*) datos->msg, strlen(datos->msg));
    next = IRC_UnPipelineCommands (datos->msg, &comando, strlen(comando));
    do {
        procesaMensaje(comando, pdesc);
        if(comando!=NULL) free(comando);
        next = IRC_UnPipelineCommands(NULL, &comando, next);
    }while(next=!NULL)  */

	printf("Mensaje(%d): %s",datos->sckfd, datos->msg);
	
	liberaDatosMensaje(datos);
	addFd(datos->sckfd);

	return NULL;
}
status nuevaConexion(int desc, struct sockaddr_in address){

	addFd(desc);

	printf("Llamada nueva conexion\n");

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


