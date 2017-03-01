
#include "../includes/config.h"
#include "../includes/funciones_servidor.h"
#include "../includes/red_servidor.h"



int main(void){

	/* deomonizamos */
	abrirLog(SERVICIO, LOG_DEBUG);
	/*daemonizar(SERVICIO, LOG_DEBUG);*/

	/* Inicializamos */
	inicializarServidor();

	/* Lanzamos el servidor */
	lanzarServidor(DEFAULT_PORT);	

	/* Liberamos estructuras */
	cerrarServidor();

	exit(EXIT_SUCCESS);
}

