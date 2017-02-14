
#include "../includes/config.h"

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
void daemonizar(char *servicio, int logLevel){

	unsigned short i;

	/* Creamos proceso hijo */
	switch(fork()){

		case -1: /* Caso de error */
			perror("Error en llamada a daemonizar()");
			exit(EXIT_FAILURE);
		
		case 0: /* Caso del hijo  */
			break;

		default: /* Caso del padre */
			exit(EXIT_SUCCESS);
	}

	/* Cambiamos mascara */
	umask(0);

	/* Abrimos log */
	abrirLog(logLevel);

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
	for(i=0; i < getdtablesize(); i++) {
		if(close(i) < 0){
			syslog(LOG_INFO, "Error en llamada close()");
		}
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Proceso convertido en daemon correctamente");

}


void abrirLog(char * identificacion, int logLevel){

	/* Cambiamos nivel de log */
	setlogmask (LOG_UPTO (logLevel));

	/* Abrimos log */
	openlog (servicio, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

}
