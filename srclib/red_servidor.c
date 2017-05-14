/**
 * @file red_servidor.c
 * @brief crear sockets, aceptar conexion y enviar
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

/**
 * @defgroup RedServidorIRC RedServidorIRC
 *
 * <hr>
 */
	
#include "../includes/red_servidor.h"
#include "../includes/ssl.h"

/**
 * @addtogroup RedServidorIRC
 * Comprende funciones para creacion de sockets, aceptar y enviar conexiones
 *
 * <hr>
 */

/**
 * @ingroup RedServidorIRC
 *
 * @brief crea un socket TCP
 *
 * @synopsis
 * @code
 * 	status crearSocketTCP(int * sckfd, unsigned short port)
 * @endcode
 *
 * @param[in] sckfd el socket
 * @param[in] port el puerto
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status crearSocketTCP(int * sckfd, unsigned short port){

	struct sockaddr_in direccion;
	int optval = 1;

	/* Creamos socket TCP */
	*sckfd = socket(AF_INET, SOCK_STREAM, 0);
	if(*sckfd == -1){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a socket()");
		return RED_ERROR;
	}

	/* Inicializamos estructura sockaddr_in */
	direccion.sin_family = AF_INET;  /* Socket de red */
	direccion.sin_port = htons(port); /* Puerto asociado */
	direccion.sin_addr.s_addr = INADDR_ANY; /* Cualquier direccion */
	memset((void *)&(direccion.sin_zero), 0, 8); /* Poner a 0 (no obligatorio) */

	/* Permitimos reutilizar el socket */
     if (setsockopt(*sckfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) { 
		syslog (LOG_ERR, "Error creando socket TCP en llamada a setsockopt()");
        return RED_ERROR;
     }  

	/* Ligamos socket al puerto correspondiente */
	if(bind(*sckfd, (struct sockaddr *) &direccion, sizeof(direccion)) < 0){
		syslog (LOG_ERR, "Error creando socket TCP en llamada a bind()");
		return RED_ERROR;
	}

	/* Ponemos a escuchar el socket */
	if (listen(*sckfd, MAX_QUEUE)<0){ /* MAX_QUEUE definida en red_servidor.h */
		syslog(LOG_ERR, "Error creando socket TCP en llamada a listen()");
		return RED_ERROR;
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Creado socket TCP correctamente");

	return RED_OK;
}

/**
 * @ingroup RedServidorIRC
 *
 * @brief Acepta conexion entrante
 *
 * @synopsis
 * @code
 * 	status aceptarConexion(int sockval,int *sckfd, struct sockaddr_in * address)
 * @endcode
 *
 * @param[in] sockval socket del que extraer la conexion
 * @param[in] sockfd nuevo socket para manejar la conexion
 * @param[in] address estructura que almacena informacion de la direccion de internet
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status aceptarConexion(int sockval,int *sckfd, struct sockaddr_in * address){

	size_t c = sizeof(struct sockaddr_in);

	/* Aceptamos conexion guardando valores en estructura Redinf */
	*sckfd = accept(sockval, (struct sockaddr *) address,(socklen_t*) &c);
	if((*sckfd) < 0 ){
		syslog(LOG_ERR, "Error aceptando conexion en llamada a accept()");
		return RED_ERROR;
	}

	/* Informacion de debugeo */
	syslog (LOG_DEBUG, "Conexion aceptada correctamente");

	return RED_OK;
}

/**
 * @ingroup RedServidorIRC
 *
 * @brief Envia mensaje desde el socket especificado
 *
 * @synopsis
 * @code
 * 	status enviar(int sockfd, char *mensaje)
 * @endcode
 *
 * @param[in] sockfd el socket 
 * @param[in] mensaje el mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status enviar(int sockfd, char *mensaje){
	if(sockfd < 0 || mensaje==NULL){
		syslog(LOG_INFO,"Error en el envio de paquete");
		return RED_ERROR;
	}

	if(ssl_active){
		enviar_datos_SSL(sockfd, mensaje, strlen(mensaje));
	} else {
		send(sockfd, mensaje, strlen(mensaje), 0);
	}
	
	return RED_OK;
}

