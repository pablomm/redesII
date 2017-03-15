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
  @brief Imprime por stderr la ayuda de usuario
*/
void usage(void){
	fprintf(stderr, "usage: [-sdp]\n");
	fprintf(stderr, " -s\tActiva comunicacion con SSL\n");
	fprintf(stderr, " -d\tLanza el servidor en modo daemon\n");
	fprintf(stderr, " -p\tNo ejecuta el protocolo Ping-Pong\n");
}
/**
  @brief Imprime mensaje de bienvenida
  @param short daemon Indica si se ha daemonizado
  @param short pingpong Indica si se ha lanzado ping-pong
  @param short ssl Indica si se ha lanzado con ssl
*/
void welcome(short daemon, short pingpong, short ssl){
	printf("Servidor IRC - \033[0;32mRedes II\033[0m - 2017\n");
	printf("\tPablo Marcos Manchon\n\tDionisio Perez Alvear\n\n");
	if(daemon)
		printf("Ejecutado en modo daemon\n");
	else
		printf("Para ejecutar en modo daemon ./servidor -d\n");
	if(ssl)
		printf("Funcionalidad ssl aun no implementada\n");
}

/**
  @brief main de prueba para el servidor
  @param void
  @return EXIT_SUCCESS
*/
int main(int argc, char *argv[]){

	char c;
	short daemon = 0, pingpong=1, ssl=0;
	
	while ((c = getopt (argc, argv, "sdp:")) != -1){
		switch (c) {
			case 's': /* Flag ssl */
				ssl = 1;
				break;
			case 'd': /* Flag no daemon */
				daemon = 1;
				break;
			case 'p': /* Flag no Ping-Pong */
				pingpong=0;
			case '?':
			default:
				usage();
				exit(EXIT_SUCCESS);
		}
	}

	/* Mensaje bienvenida */
	welcome(daemon,pingpong, ssl);

	/* daemonizamos */
	if(daemon) {
		daemonizar(SERVICIO, LOG_INFO);
	} else {
		abrirLog(SERVICIO, LOG_INFO);
	}

	/* Inicializamos */
	inicializarServidor();

	/* Lanzamos el servidor */
	lanzarServidor(DEFAULT_PORT);	

	/* Liberamos estructuras */
	cerrarServidor();

	exit(EXIT_SUCCESS);
}

