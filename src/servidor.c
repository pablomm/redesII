/**
 * @file cliente_echo.c
 * @brief main sencillo que prueba el servidor
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup ServidorIRC Servidor
 *
 * <hr>
 */
#include <getopt.h>

#include "../includes/config.h"
#include "../includes/funciones_servidor.h"
#include "../includes/red_servidor.h"

#include "../includes/ssl.h"

/**
 * @addtogroup ServidorIRC
 * Funciones auxiliares para lanzar un servidor
 *
 * <hr>
 */

/**
 * @ingroup ServidorIRC
 *
 * @brief Imprime por stderr la ayuda de usuario
 *
 * @synopsis
 * @code
 * 	void usage(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void usage(void){
	fprintf(stderr, "usage: [-sdpoh] --port <puerto>\n");
	fprintf(stderr, " -s --ssl\tActiva comunicacion con SSL\n");
	fprintf(stderr, " -d --daemon\tLanza el servidor en modo daemon\n");
	fprintf(stderr, " -p --ping\tEjecuta el protocolo Ping-Pong\n");
	fprintf(stderr, " -o --offensive\tEjecuta el motds ofensivos\n");
	fprintf(stderr, " -P --port <puerto>\tEspecifica puerto a conectar\n");
}

/**
 * @addtogroup ServidorIRC
 * Funciones auxiliares para lanzar un servidor
 *
 * <hr>
 */


 /**
 * @ingroup ServidorIRC
 *
 * @brief Imprime mensaje de bienvenida
 *
 * @synopsis
 * @code
 * 	void welcome(short daemon, short pingpong, short ssl, short offensive, int puerto_definido)
 * @endcode
 *
 * @param[in] daemon Para lanzar el servidor en modo daemon
 * @param[in] pingpong Para activar la rutina pingpong
 * @param[in] ssl Para activar transimion segura SSL
 * @param[in] offensive Para activar motd ofensivo
 * @param[in] puerto_definido Para especificar puerto de recepcion
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void welcome(short daemon, short pingpong, short ssl, short offensive, int puerto_definido){
	printf("Servidor IRC - \033[0;32mRedes II\033[0m - 2017\n");
	printf("\tPablo Marcos Manchon\n\tDionisio Perez Alvear\n\n");
	if(daemon) {
		printf("Ejecutado en modo daemon\n");
	} else {
		printf("Utilice flag -d --daemon para lanzar el servidor en modo daemon\n");
	}

	if(ssl) {
		printf("Activada conexión segura SSL\n");
	} else {
		printf("Utilice flag -s --ssl para lanzar el servidor con seguridad SSL\n");
	}

	if(pingpong) {
		printf("Protocolo PING PONG lanzado con retardo de %d segundos\n", ALARMPINGPONG);
	} else {
		printf("Utilice flag -p --ping para lanzar la rutina PING-PONG\n");
	}

	if(!offensive) {
		printf("Utilice el flag -o --offensive para recibir diferentes motds\n");
	} else {
		printf("Bandera -o activada, no nos hacemos responsables si se siente ofendido\n");
		printf("Asegurese de tener instalados los paquetes fortune y cowsay\n");
	}

	if(puerto_definido) {
		printf("Puerto especificado para la conexión: %d\n", puerto_definido);
	} else {
		printf("Utilice flag -P --port <puerto> para especificar puerto a conectar\n");
		printf("Por defecto se conecta al puerto %d o al puerto %d con SSL\n", DEFAULT_PORT, DEFAULT_PORT_SSL);
	}
}

int main(int argc, char *argv[]){

	char c;
	short daemon = 0, pingpong=0;
	int puerto_definido = 0;
	int index = 0;

	static struct option long_options[] =
        {
          
          {"ssl",     no_argument,       0, 's'},
          {"daemon",  no_argument,       0, 'd'},
          {"offensive",  no_argument,       0, 'o'},
          {"ping",  no_argument,       0, 'p'},
          {"port",  required_argument, 0, 'P'},
        };

	ssl_active = 0;
	offensive = 0;

	while ((c = getopt_long (argc, argv, "sdpohP:", long_options, &index)) != -1){
		switch (c) {
			case 's': /* Flag ssl */
				ssl_active = 1;
				break;
			case 'd': /* Flag no daemon */
				daemon = 1;
				break;
			case 'p': /* Flag no Ping-Pong */
				pingpong=1;
				break;
			case 'o': /* MOTD ofensivo */
				offensive = 1;
				break;
			case 'P':
				puerto_definido = atoi(optarg);
				break;
			case '?':

			default:
				usage();
				exit(EXIT_SUCCESS);
		}
	}

	/* Mensaje bienvenida */
	welcome(daemon,pingpong, ssl_active, offensive, puerto_definido);

	/* Guardamos path desde el que se ha lanzado */
	getcwd(abspath, 1023);

	/* daemonizamos */
	if(daemon) {
		daemonizar(SERVICIO, LOG_INFO);
	} else {
		abrirLog(SERVICIO, LOG_INFO);
	}

	/* Inicializamos */
	inicializarServidor();
	

	/* Lanzamos el servidor */
	if(!ssl_active){
		lanzarServidor(puerto_definido?puerto_definido:DEFAULT_PORT, pingpong);	
	} else {
		lanzarServidorSSL(puerto_definido?puerto_definido:DEFAULT_PORT_SSL, pingpong);
	}

	/* Liberamos estructuras */
	cerrarServidor();

	exit(EXIT_SUCCESS);
}

