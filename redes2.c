
#include "redes2.h"

/**
 * @ingroup No lo se
 *
 * @page No lo se
 *
 * @brief Daemoniza el proceso desde el que es lanzado
 *
 * @synopsis
 * @code
 *	#include "redes2.h"
 *
 * 	int daemonizar(void);
 * @endcode
 * 
 * @description 
 * Crea un proceso hijo, y mata al padre para depender del proceso
 * init, además se hace lider de la sesión, cierra todos los descriptores
 * abiertos, cambia la carpeta de trabajo al directorio raiz y hace una
 * llamada a log para volcar información en el log del sistema
 * Si hay un error durante la ejecución de esta función se finalizará la
 * ejecucion del programa
 * 
 *
 *
 * @author
 * Pablo Marcos (pablo.marcosm@estudiante.uam.es)
 *
 *<hr>
 */

void daemonizar(int logLevel){

	unsigned short i;

	/* Creamos proceso hijo */
	switch(fork()){
		case -1: /* Caso de error */
			perror("Error en fork para daemonizar proceso");
			exit(EXIT_FAILURE);
		
		case 0: /* Caso del hijo  */
			break;

		default: /* Caso del padre */
			exit(EXIT_SUCCESS);
	}

	/* Cambiamos mascara */
	umask(0);

	/* Cambiamos nivel de log */
	setlogmask (LOG_UPTO (logLevel));

	/* Abrimos log */
	openlog (SERVICIO, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

	/* Creamos nueva sesion */
	if(setsid() < 0){
		syslog (LOG_ERR, "Error en llamada a setsid()");
		exit(EXIT_FAILURE);
	}

	/* Cambiamos a directorio raiz */
	if(chdir("/") < 0){
		syslog (LOG_ERR, "Error cambiando directorio de trabajo");
		exit(EXIT_FAILURE);
	}

	/* Cerramos descriptores abiertos */
	for(i=0; i < getdtablesize(); i++)
		close(i);

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Proceso convertido en daemon correctamente");

	return;
}

/**
 * @ingroup No lo se
 *
 * @page No lo se
 *
 * @brief Crea un socket TCP
 *
 * @synopsis
 * @code
 *	#include "redes2.h"
 *
 * 	int crearSocketTCP(unsigned short port, int connections);
 * @endcode
 * 
 * @description 
 * Crea un socket TCP con el protocolo TCP, lo asocia al puerto
 * correspondiente y lo pone a escuchar en el puerto a la espera 
 * de aceptar conexiones entrantes
 * 
 *
 *
 * @author
 * Pablo Marcos (pablo.marcosm@estudiante.uam.es)
 *
 *<hr>
 */
int crearSocketTCP(unsigned short port, int connections){
	int sockfd;
	struct sockaddr_in direccion;

	/* Creamos socket TCP */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a socket()");
		return ERROR;
	}

	/* Inicializamos estructura sockaddr_in */
	direccion.sin_family = AF_INET; /∗ TCP/IP ∗/
	direccion.sin_port = htons(port); /∗ Puerto asociado ∗/
	direccion.sin_addr.s_addr = htonl(INADDR_ANY); /∗ Cualquier direccion ∗/
	memset((void ∗)&(direccion.sin_zero), 0, 8);

	/* Ligamos socket al puerto correspondiente */
	if(bind(sockfd, &direccion, sizeof(direccion)) < 0){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a blind()");
		return ERROR;
	}

	if (listen(sockval, connections)<0){
		syslog(LOG_ERR, "Error creando socket TCP en llamada a listen()");
		return ERROR;
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Creado socket TCP correctamente");

	return sockfd;
}


