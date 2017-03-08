/**
  @file servidor.c
  @brief main sencillo que prueba el servidor
  @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
  @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#include "../includes/config.h"
#include "../includes/funciones_servidor.h"
#include "../includes/red_servidor.h"

/**
  @brief main de prueba para el servidor
  @param void
  @return EXIT_SUCCESS
*/
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

