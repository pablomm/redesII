
#include "../includes/config.h"
#include "../includes/funciones_servidor.h"
#include "../includes/red_servidor.h"
#include "../includes/conexion_temp.h"

void daemonizar(char *servicio, int logLevel){

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

	cerrarDescriptores();

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

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Proceso convertido en daemon correctamente");

}

void cerrarDescriptores(void){
	unsigned short i;

	/* Cerramos descriptores abiertos */
	for(i=0; i < getdtablesize(); i++)
		close(i);
}

void abrirLog(char * identificacion, int logLevel){

	/* CambiamologLevels nivel de log */
	setlogmask (LOG_UPTO (logLevel));

	/* Abrimos log */
	openlog (identificacion, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
}


void manejadorSIGUSR1(int sennal){

	if(signal(SIGUSR1, manejadorSIGUSR1) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGUSR1");
	}
}

status inicializarServidor(void){

	if(signal(SIGINT, manejadorSigint) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGINT");
	}

	if(signal(SIGUSR1, manejadorSIGUSR1) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGUSR1");
	}

	pid = getpid();

	/* Mutex conjunto de descriptores */
	pthread_mutex_init(&mutexDescr, NULL);

	/* Mutex lista usuarios temporales */
	pthread_mutex_init(&mutexTempUser, NULL);

	usuarioPrimero = NULL;
	usuarioUltimo = NULL;


	return SERV_OK;
}




void cerrarServidor(void){

	cerrarDescriptores(); 	
	closelog();

}

void manejadorSigint(int signal){

	syslog(LOG_INFO, "Terminado por recepcion de sigint");
	cerrarServidor();
	exit(EXIT_SUCCESS);

}

status inicializarFD(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_ZERO(&activeFD);
  	FD_SET(sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;

}

status addFd(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_SET (sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);


	/* Enviamos señal para actualizar fd_set */
	kill(pid, SIGUSR1);

	return SERV_OK;
}

status deleteFd(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_CLR (sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;
}

status setReadFD(fd_set * readFD){

	pthread_mutex_lock(&mutexDescr);
	*readFD = activeFD;
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;
}



status lanzarServidor(unsigned int puerto){

	int sckt, i,desc;
	status ret;
	pthread_t auxt;
	fd_set readFD;
	struct sockaddr_in address;
	pDatosMensaje datos;
	char buffer[MAX_BUFFER] = {0};

	/* Creamos socket TCP */
	if((ret = crearSocketTCP(&sckt, puerto)) < 0){
		return ret;
	}

	/* Inicializamos conjunto de sockets */
	inicializarFD(sckt);

	running = 1;

	while(running){

		/* Igualamos sockets leidos a los activos */
		setReadFD(&readFD);

		/* LLamada select bloqueante */
		ret = select(FD_SETSIZE, &readFD, NULL, NULL, NULL);

		if(ret < 0) {
			/* Informacion de debugeo */
			syslog(LOG_DEBUG, "Restaurando set_fd por recepcion de signal");
			continue;
		}


      	/* Bucle para procesar sockets pendientes */
      	for (i = 0; i < FD_SETSIZE; ++i){

			/* Solo procesamos sockets marcados */
		    if (FD_ISSET(i, &readFD)){

				/* Caso conexion nueva */		        
				if (i == sckt){

					/* Aceptamos conexion */
		            if(aceptarConexion(sckt, &desc, &address)  < 0){
		                syslog(LOG_WARNING, "No se ha podido procesar nueva conexion");

		            } else {
						/* Creamos nueva conexion */
						if(nuevaConexion(desc, address) < 0){
							syslog(LOG_WARNING, "Error en llamada a nuevaConexion()");
						}
					}
				
				/* Caso ya aceptado */		       
				} else {

					recv(i, (void*) buffer, MAX_BUFFER, 0);

					datos = (pDatosMensaje) calloc (1, sizeof(DatosMensaje));
					if(datos == NULL){
						syslog(LOG_ERR, "Error reservando memoria para datos");
						return SERV_ERROR;
					}

					datos->sckfd = i;
					datos->len = strlen(buffer) + 1;
					datos->msg = (char *) calloc (datos->len, sizeof(char));
					strcpy(datos->msg, buffer);
					deleteFd(i);
					memset((void*)buffer, 0, datos->len);

					/* Lanzamos hilo para procesar el mensaje */
		         	if (pthread_create(&auxt , NULL, manejaMensaje, (void *) datos)  < 0) {
						liberaDatosMensaje(datos);
 						syslog(LOG_WARNING, "Error lanzando hilo para procesar mensaje");
						addFd(i);
					}
				}
			}
		}	
	}

	return SERV_OK;

}







