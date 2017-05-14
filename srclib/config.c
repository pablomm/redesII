/**
  @file config.c
  @brief daemonizar y funciones relacionadas con el servidor y descriptores
  @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
  @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/** 
 * @defgroup Config Config
 *
 */

#include "../includes/config.h"
#include "../includes/funciones_servidor.h"
#include "../includes/red_servidor.h"
#include "../includes/conexion_temp.h"
#include "../includes/ssl.h"

/**
 * @addtogroup Config
 * Funciones relacionadas con lanzar un servidor
 *
 * <hr>
 */

/**
 * @ingroup Config
 *
 * @brief Demoniza el proceso
 *
 * @synopsis
 * @code
 * 	void daemonizar(char *servicio, int logLevel)
 * @endcode
 *
 * @param[in] servicio identificacion para el log
 * @param[in] logLevel nivel de mascara del log
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
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

/**
 * @ingroup Config
 *
 * @brief Ciera el conjunto de descriptores
 *
 * @synopsis
 * @code
 * 	void cerrarDescriptores(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrarDescriptores(void){
	unsigned short i;

	/* Cerramos descriptores abiertos */
	for(i=0; i < getdtablesize(); i++)
		close(i);
}

/**
 * @ingroup Config
 *
 * @brief Abre el log para el envio de mensajes
 *
 * @synopsis
 * @code
 * 	void abrirLog(char * identificacion, int logLevel)
 * @endcode
 *
 * @param[in] identificacion identificacion para el log
 * @param[in] logLevel nivel de mascara del log
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void abrirLog(char * identificacion, int logLevel){

	/* CambiamologLevels nivel de log */
	setlogmask (LOG_UPTO (logLevel));

	/* Abrimos log */
	openlog (identificacion, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
}

/**
 * @ingroup Config
 *
 * @brief Manejador de la sennal SIGUSR1
 *
 * @synopsis
 * @code
 * 	void manejadorSIGUSR1(int sennal)
 * @endcode
 *
 * @param[in] sennal sennal
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejadorSIGUSR1(int sennal){

	if(signal(SIGUSR1, manejadorSIGUSR1) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGUSR1(%d)", sennal);
	}
}

/**
 * @ingroup Config
 *
 * @brief Manejador de la sennal SIGALARM
 *
 * @synopsis
 * @code
 * 	void manejadorSIGALRM(int sennal)
 * @endcode
 *
 * @param[in] sennal sennal
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejadorSIGALRM(int sennal){

	/* Llamamos a la rutina del ping pong */
	rutinaPingPong();

	if(signal(SIGALRM, manejadorSIGALRM) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGALRM(%d)", sennal);
	}

	alarm(ALARMPINGPONG);
}


/**
 * @ingroup Config
 *
 * @brief Inicializa el servidor
 *
 * @synopsis
 * @code
 * 	void manejadorSIGALRM(int sennal)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status inicializarServidor(void){

	if(signal(SIGINT, manejadorSigint) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGINT");
	}

	if(signal(SIGUSR1, manejadorSIGUSR1) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGUSR1");
	}

	if(signal(SIGALRM, manejadorSIGALRM) == SIG_ERR){
		syslog(LOG_WARNING, "No se ha podido montar manejador SIGALRM");
	}


	pid = getpid();

	/* Mutex conjunto de descriptores */
	pthread_mutex_init(&mutexDescr, NULL);

	/* Mutex lista usuarios temporales */
	pthread_mutex_init(&mutexTempUser, NULL);

	/* Inicializamos listas temporales */
	usuarioPrimero = NULL;
	usuarioUltimo = NULL;

	/* Inicializamos array de comandos */
	crea_comandos();

	return SERV_OK;
}

/**
 * @ingroup Config
 *
 * @brief Cierra el servidor
 * @synopsis
 * @code
 * 	void manejadorSIGALRM(int sennal)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrarServidor(void){
	syslog(LOG_INFO, "Cerrando el servidor...");
	cerrarDescriptores(); 	
	closelog();
	liberarEstructuras();
	/* thpool_destroy(thpool); */
	if(ssl_active){
		liberar_nivel_SSL();
	}
}

/**
 * @ingroup Config
 *
 * @brief Manejador de la sennal SIGINT
 *
 * @synopsis
 * @code
 * 	void manejadorSigint(int sennal)
 * @endcode
 *
 * @param[in] sennal sennal
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejadorSigint(int signal){

	syslog(LOG_INFO, "Terminado por recepcion de sigint(%d)", signal);
	cerrarServidor();
	exit(EXIT_SUCCESS);
}

/**
 * @ingroup Config
 *
 * @brief Inicializa descriptor del socket dado
 *
 * @synopsis
 * @code
 * 	status inicializarFD(int sckt)
 * @endcode
 *
 * @param[in] sckt el socket
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status inicializarFD(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_ZERO(&activeFD);
  	FD_SET(sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;

}

/**
 * @ingroup Config
 *
 * @brief Annade descriptor al conjunto de descriptores activos
 *
 * @synopsis
 * @code
 * 	status addFd(int sckt)
 * @endcode
 *
 * @param[in] sckt el socket
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status addFd(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_SET (sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);


	/* Enviamos señal para actualizar fd_set */
	kill(pid, SIGUSR1);

	return SERV_OK;
}

/**
 * @ingroup Config
 *
 * @brief Elimina descriptor del socket dado 
 *
 * @synopsis
 * @code
 * 	status deleteFd(int sckt)
 * @endcode
 *
 * @param[in] sckt el socket
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status deleteFd(int sckt){

	pthread_mutex_lock(&mutexDescr);
	FD_CLR (sckt, &activeFD);
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;
}

/**
 * @ingroup Config
 *
 * @brief Pone como leido el socket dado
 *
 * @synopsis
 * @code
 * 	status setReadFD(fd_set * readFD)
 * @endcode
 *
 * @param[in] readFD el descriptor
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status setReadFD(fd_set * readFD){

	pthread_mutex_lock(&mutexDescr);
	*readFD = activeFD;
	pthread_mutex_unlock(&mutexDescr);

	return SERV_OK;
}

/**
 * @ingroup Config
 *
 * @brief Lanza el servidor
 *
 * @synopsis
 * @code
 * 	status lanzarServidor(unsigned int puerto, short pingpong)
 * @endcode
 *
 * @param[in] puerto puerto para el que crear socket
 * @param[in] pingpong para activar rutina pingpong
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status lanzarServidor(unsigned int puerto, short pingpong){

	int sckt, i, desc;
	ssize_t tam;
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

	/* Alarma para la rutina ping pong */
	if(pingpong)
		alarm(ALARMPINGPONG);

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
						if(nuevaConexion(desc, &address) < 0){
							syslog(LOG_WARNING, "Error en llamada a nuevaConexion()");
						}
					}
				
				/* Caso ya aceptado */		       
				} else {

					tam = recv(i, (void*) buffer, MAX_BUFFER, 0);
					if(tam == -1){
						syslog(LOG_ERR, "Error n: %d al recibir mensaje del socket %d", errno, i);
						continue;

					} else if(tam <= 0){ /* Caso socket cerrado */
						cerrarConexion(i);
						continue;
					}

					/* Empaquetamos el mensaje */
					datos = (pDatosMensaje) calloc (1, sizeof(DatosMensaje));
					if(datos == NULL){
						syslog(LOG_ERR, "Error reservando memoria para datos");
						return SERV_ERROR;
					}

					datos->sckfd = i;
					datos->len = tam;
					/* +1 para evitar que funciones de eloy no hagan accesos invalidos si 
						acaba en \n unicamente */
					datos->msg = (char *) calloc(tam+1, sizeof(char));
					memcpy(datos->msg, buffer, tam);
					deleteFd(i);
					memset((void*)buffer, 0, tam);

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

/**
 * @ingroup Config
 *
 * @brief Lanza el servidor con SSL
 *
 * @synopsis
 * @code
 * 	status lanzarServidorSSL(unsigned int puerto, short pingpong)
 * @endcode
 *
 * @param[in] puerto puerto para el que crear socket
 * @param[in] pingpong para activar rutina pingpong
 *
 * @return SERV_OK
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status lanzarServidorSSL(unsigned int puerto, short pingpong){

	int sckt, i, desc;
	ssize_t tam;
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

	/* Inicializamos libreria SSL */
	inicializar_nivel_SSL();
 
	if(fijar_contexto_SSL(PATH_SERV_CERT, PATH_SERV_PKEY) < 0){
		close(sckt);
		liberar_nivel_SSL();
		return SERV_ERROR;
	}

	/* Inicializamos conjunto de sockets */
	inicializarFD(sckt);

	running = 1;

	/* Alarma para la rutina ping pong */
	if(pingpong)
		alarm(ALARMPINGPONG);

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
						/* Aceptamos canal seguro */
						if(aceptar_canal_seguro_SSL(desc) < 0){
							cerrar_canal_SSL(desc);
							close(desc);
							syslog(LOG_ERR, "Error aceptando canal seguro");
							continue;
						}

						/* Creamos nueva conexion */
						if(nuevaConexion(desc, &address) < 0){
							syslog(LOG_WARNING, "Error en llamada a nuevaConexion()");
						}
					}
				
				/* Caso ya aceptado */		       
				} else {

					tam = recibir_datos_SSL(i, buffer, MAX_BUFFER);
					if(tam == -1){
						syslog(LOG_ERR, "Error n: %d al recibir mensaje del socket %d", errno, i);
						continue;

					} else if(tam == 0){ /* Caso socket cerrado */
						cerrarConexion(i);
						continue;
					}

					/* Empaquetamos el mensaje */
					datos = (pDatosMensaje) calloc (1, sizeof(DatosMensaje));
					if(datos == NULL){
						syslog(LOG_ERR, "Error reservando memoria para datos");
						return SERV_ERROR;
					}

					datos->sckfd = i;
					datos->len = tam;
					/* +1 para evitar que funciones de eloy no hagan accesos invalidos si 
						acaba en \n unicamente */
					datos->msg = (char *) calloc(tam+1, sizeof(char));
					memcpy(datos->msg, buffer, tam);
					deleteFd(i);
					memset((void*)buffer, 0, tam);

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

