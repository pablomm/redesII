
#include "../includes/red_servidor.h"




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
status crearSocketTCP(unsigned short port, int connections){
	int sockfd;
	struct sockaddr_in direccion;

	/* Creamos socket TCP */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a socket()");
		return RED_ERROR;
	}

	/* Inicializamos estructura sockaddr_in */
	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(port); /* Puerto asociado */
	direccion.sin_addr.s_addr = htonl(0); /* Cualquier direccion */
	memset((void *)&(direccion.sin_zero), 0, 8);

	/* Ligamos socket al puerto correspondiente */
	if(bind(sockfd,(struct sockaddr *) &direccion, sizeof(direccion)) < 0){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a bind()");
		return RED_ERROR;
	}

	if (listen(sockfd, connections)<0){
		syslog(LOG_ERR, "Error creando socket TCP en llamada a listen()");
		return RED_ERROR;
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Creado socket TCP correctamente");

	return sockfd;
}

status aceptarConexion(int sockval){

	int desc;

	if((desc = accept(sockval, NULL, NULL)) < 0 ){
		syslog(LOG_ERR, "Error aceptando conexion en llamada a accept()");
		return RED_ERROR;
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Conexion aceptada correctamente");

	return desc;
}

status nuevaConexion(int sockval, void *(*procesarUsuario) (void *)){

	int desc = aceptarConexion(sockval);
	if(desc < 0){
		return RED_ERROR;
	}

	/* Procesar cliente */
	if (pthread_create(NULL, NULL, procesarUsuario, &desc) < 0) {
		syslog(LOG_ERR, "Error creando hilo en llamada a pthread_create()");
		return RED_ERROR;
	}

	return RED_OK;
}








