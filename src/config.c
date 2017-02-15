
#include "../includes/config.h"

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
	abrirLog(servicio, logLevel);

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

	/* CambiamologLevels nivel de log */
	setlogmask (LOG_UPTO (logLevel));

	/* Abrimos log */
	openlog (identificacion, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

}


void inicializarServidor(void){

	pthread_mutex_init(&mutexnuevo, NULL);  
	pthread_mutex_init(&mutexdescr, NULL);

}

void cerrarServidor(void){
 	closelog();

}





